#include "EWEngine/Systems/SkeletonEditorScene.h"

#include "LAB/Camera.h"

#include "imgui.h"

#include <filesystem>

namespace EWE{

	VmaAllocationCreateInfo GetSkeleAllocInfo(){
		return VmaAllocationCreateInfo{

		};
	}

	SkinPush::SkinPush()
	:
		cameraBuffer{
			engine->logicalDevice,
			sizeof(CameraBufferObject), 1,
			GetSkeleAllocInfo()
		},
		lightBuffer{
			engine->logicalDevice,
			sizeof(LightBufferObject), 1,
			GetSkeleAllocInfo()
		},
		vertBuffer{
			engine->logicalDevice,
			sizeof(SplitVertexBufferObject), 1,
			GetSkeleAllocInfo()
		},
		materialBuffer{
			engine->logicalDevice,
			sizeof(MaterialBufferObject), 1,
			GetSkeleAllocInfo()
		},
		instanceBuffer{
			engine->logicalDevice,
			sizeof(SkinInstanceBufferObject), 1,
			GetSkeleAllocInfo()
		},
		sbo{
			.camera = cameraBuffer.deviceAddress,
			.lights = lightBuffer.deviceAddress,
			.vertex = vertBuffer.deviceAddress,
			.material = materialBuffer.deviceAddress,
			.instances = instanceBuffer.deviceAddress
			//vert_data needs to be initialized separately
			//initialize material separately
		}
	{

	}



	AttachmentSetInfo GetSkeleSceneAttachmentSetInfo(){
		AttachmentInfo colorInfo[1] = {
            AttachmentInfo{
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp =  VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {0.f, 0.f, 0.f, 0.f}
            }
        };
		return AttachmentSetInfo{
			1, 1, 
			VkRenderingFlags{0},
			colorInfo,
			AttachmentInfo{
				.format = VK_FORMAT_D16_UNORM,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.clearValue{
					.depthStencil{
						.depth = 1.1f,
						.stencil = 0
					}
				}
			}
		};
	}

	Sampler& GetSkeleSampler() {
		VkSamplerCreateInfo samplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			.mipLodBias = 0.f,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 0.f,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.minLod = 0.f,
			.maxLod = 1.f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE
		};

		return EWE::Global::assetManager->sampler.Get(samplerCreateInfo);
	}

	TaskRasterConfig GetSkeleSceneTaskConfig() {
		TaskRasterConfig ret;
		ret.SetDefaults();
		ret.depthStencilInfo.depthTestEnable = VK_TRUE;
		ret.depthStencilInfo.depthWriteEnable = VK_TRUE;

		ret.attachment_info = GetSkeleSceneAttachmentSetInfo();

		return ret;
	}


    SkeletonScene::SkeletonScene()
    : 
        SceneBase{"skeleton scene"},

        explorer{std::filesystem::current_path() / "resources/models"},

        rasterPkg{
			"mmscene",
			engine->logicalDevice, engine->renderQueue,
			GetSkeleSceneTaskConfig()
		},
		pkgRecord{"mmscene", engine->renderQueue},
		gpuTask{"mmscene", engine->logicalDevice, pkgRecord, false},
		subTask{"mmscene", engine->logicalDevice, engine->renderQueue},
		renderGraph{
			"mmscene",
			engine->logicalDevice, engine->swapchain,
			engine->renderQueue, engine->computeQueue,
			engine->graphics_stc_task, engine->compute_stc_task
		},
		fri{
			"mmscenefri",
			engine->logicalDevice, engine->renderQueue,
			GetSkeleSceneAttachmentSetInfo(),
			engine->window.screenDimensions.width, engine->window.screenDimensions.height
		},
		attachment_diis{
			ArgumentPack_ConstructionHelper<4>{},
			GetSkeleSampler(),
			*fri.full.color_views[0][0],
			DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

			GetSkeleSampler(),
			*fri.full.color_views[0][1],
			DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		}
    {
        root_path.fill('\0');
		skeletonPackage.payload.shaders.fill(nullptr);
        skeletonPackage.payload.shaders[ShaderStage::Vertex] = Global::assetManager->shader.Get("skin_uber.vert.spv");
        skeletonPackage.payload.shaders[ShaderStage::Fragment] = Global::assetManager->shader.Get("skin_uber.frag.spv");
        skeletonPackage.payload.config.SetDefaults();

        skeletonPackage.paramPool.PushBack(Inst::If);
        skeletonPackage.paramPool.PushBack(Inst::Push);
        skeletonPackage.paramPool.PushBack(Inst::DrawIndexed);
        skeletonPackage.paramPool.PushBack(Inst::EndIf);

		ifPack = skeletonPackage.paramPool.CastParam<Inst::If>(0);
        pushPack = skeletonPackage.paramPool.CastParam<Inst::Push>(1);
        drawPack = skeletonPackage.paramPool.CastParam<Inst::DrawIndexed>(2);

		for_each_frame{
			ifPack->GetRef(frame).enabled = false;
			pushPack->GetRef(frame).buffer_count = 5;
			pushPack->GetRef(frame).texture_count = 0;
			pushPack->GetRef(frame).size = pushPack->GetRef(frame).Size();

			drawPack->GetRef(frame).indexBuffer.buffer = VK_NULL_HANDLE;
			drawPack->GetRef(frame).indexBuffer.offset = 0;
			drawPack->GetRef(frame).indexBuffer.indexType = VK_INDEX_TYPE_UINT16;
			
			drawPack->GetRef(frame).indexCount = 0;
			drawPack->GetRef(frame).instanceCount = 0;
			drawPack->GetRef(frame).firstIndex = 0;
			drawPack->GetRef(frame).vertexOffset = 0;
			drawPack->GetRef(frame).firstInstance = 0;
		}

		pkgRecord.packages.push_back(&rasterPkg);
		subTask.tasks.push_back(&gpuTask);
        PerFlight<Image*> temp_att_res{
            &fri.full.color_views[0][0]->image,
            &fri.full.color_views[0][1]->image
        };
        const UsageData<Image> initial_acquire_usage{
            .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
		color_attachment_index = gpuTask.resources.AddResource<Image>(temp_att_res, initial_acquire_usage);
    }

    SkeletonScene::~SkeletonScene(){

    }

    void SkeletonScene::Load(){
    }

    void SkeletonScene::Entry(){
		//inScene = true;

        rasterPkg.objectPackages.clear();

		rasterPkg.objectPackages.push_back(&skeletonPackage);

		rasterPkg.scissor = EWE::engine->window.screenDimensions;
		rasterPkg.viewport.x = 0.f;
		rasterPkg.viewport.y = static_cast<float>(EWE::engine->window.screenDimensions.height);
		rasterPkg.viewport.width = static_cast<float>(EWE::engine->window.screenDimensions.width);
		rasterPkg.viewport.height = -static_cast<float>(EWE::engine->window.screenDimensions.height);
		rasterPkg.viewport.minDepth = 0.0f;
		rasterPkg.viewport.maxDepth = 1.f;
		rasterPkg.Compile();
		rasterPkg.Undefer(fri);

		gpuTask.GenerateWorkload();
		subTask.packaged_tasks.push_back(gpuTask.workload);

		Global::mergeTask->textures_to_be_merged.ClearAndResize(2);
		Global::mergeTask->textures_to_be_merged[0].lh.task = &gpuTask;
		Global::mergeTask->textures_to_be_merged[0].lh.index = color_attachment_index;
		for_each_frame{
			Global::mergeTask->textures_to_be_merged[0].texIndex[frame] = attachment_diis[frame].index;
		}

		Global::mergeTask->textures_to_be_merged[1].lh.task = &Global::imguiTask->gpuTask;
		Global::mergeTask->textures_to_be_merged[1].lh.index = Global::imguiTask->color_attachment_index; //imgui task attachment is guaranteed to be 0
		for_each_frame{
			Global::mergeTask->textures_to_be_merged[1].texIndex[frame] = Global::imguiTask->attachment_diis[frame].index;
		}

		Global::mergeTask->AddToRenderGraph(renderGraph);
		Global::imguiTask->AddToRenderGraph(renderGraph);

		renderGraph.tasks.push_back(&gpuTask);

		renderGraph.execution_order = {
			std::vector<EWE::SubmissionTask*>{&subTask, &Global::imguiTask->subTask},
			std::vector<EWE::SubmissionTask*>{&Global::mergeTask->subTask}
		};
		renderGraph.InitializeSemaphores();

		engine->current_renderGraph = &renderGraph;

		EWE::Global::imguiHandler->viewports.resize(1);
        {
            auto& vp_back = EWE::Global::imguiHandler->viewports.emplace_back();
            vp_back.exec_funcs.clear();
            vp_back.context = EWE::Global::imguiHandler->InitializeContext();
            vp_back.current_viewport.extent.width = EWE::engine->window.screenDimensions.width;
            vp_back.current_viewport.extent.height = EWE::engine->window.screenDimensions.height;

            vp_back.exec_funcs.push_back(
                [&] (ImguiViewport& vp) {

                    this->Imgui();
                }
            );
        }

		skinPush.cam_forward = lab::vec3(1.f, 0.f, 0.f);
		skinPush.camera.proj = lab::ProjectionMatrix(
			skinPush.fov_radians, 
			static_cast<float>(engine->window.screenDimensions.width) / static_cast<float>(engine->window.screenDimensions.height), 
			0.f, 100.f
		);
		skinPush.camera.view = lab::ViewDirection<
			CoordinateSystem{}
		>(
			lab::vec3{skinPush.camera.camPos}, 
			skinPush.cam_forward
		);
		skinPush.camera.projView = skinPush.camera.proj * skinPush.camera.view;

		skinPush.lights.ambientColor = Color_RGBA<flaot>{1.f, 1.f, 1.f, 1.f};
		skinPush.lights.sunDir = lab::vec3(0.f, 0.f, 1.f);
		skinPush.lights.sunColor = Color_RGBA<float>{1.f, 0.f, 0.f, 1.f};
		skinPush.point_count = 0;

		skinPush.material.albedo = Color_RGB<float>{0.04f, 0.04f, 0.04f};
		skinPush.material.rough_val = 0.5f;
		skinPush.material.metal_val = 0.5f;

		skinPush.instance_data.transform.translation = lab::vec3{0.f};
		skinPush.instance_data.transform.scale = lab::vec3{1.f};
		skinPush.instance_data.transform.rotation = lab::vec3{0.f};
	}

    bool SkeletonScene::RenderUpdate(double dt){
		dt = lab::Min(dt, 1.0 / 10.0);

		return true;
    }

    void SkeletonScene::Exit(){
		//inScene = false;
    }

	template<typename T>
	struct is_vector : std::false_type {};

	template<typename T, typename Alloc>
	struct is_vector<std::vector<T, Alloc>> : std::true_type {
		using Type = T;
	};

	template<typename T>
	inline constexpr bool is_vector_v = is_vector<T>::value;

	template<std::meta::info T>
	consteval std::string_view RecursivelyGetTemplateName(){
		if constexpr (std::meta::has_template_arguments(T)) {
			static constexpr auto template_id = std::meta::identifier_of(std::meta::template_of(T));
			static constexpr auto args = std::define_static_array(std::meta::template_arguments_of(T));

			std::string name{template_id};
			name += "<";

			template for (constexpr auto arg : args) {
				if constexpr (std::meta::is_type(arg)) {
					// could be a plain type OR another template specialization (or alias
					// to one) - let the recursive call sort out which
					name += RecursivelyGetTemplateName<arg>();
				}
				else if constexpr (std::meta::has_identifier(arg)) {
					name += std::meta::identifier_of(arg);
				}
				else {
					name += std::meta::display_string_of(arg);
				}
				name += ", ";
			}

			if (args.size() > 0) {
				name.pop_back();
				name.pop_back(); // trailing ", "
			}
			name += ">";

			return std::define_static_string(name);
		}
		else if constexpr (std::meta::has_identifier(T)) {
			return std::meta::identifier_of(T);
		}
		else {
			return std::meta::display_string_of(T);
		}
	}

	template<typename T>
	void MetaImgui(T& obj){

		static constexpr auto is_class_type = std::meta::is_class_type(^^T);
		static constexpr auto base_name = std::meta::display_string_of(^^T);
		//Log::Debug("is class type - %s : %d\n", base_name.data(), is_class_type);

		if constexpr(std::meta::is_class_type(^^T)){

			static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
			template for(constexpr auto mem : members){
				auto& member = obj.[:mem:];
				static constexpr auto mem_name = std::meta::has_identifier(mem) ? std::meta::identifier_of(mem) : std::meta::display_string_of(mem);
				//Log::Debug("member is vector - %s : %d\n", mem_name.data(), is_vector_v<std::remove_cvref_t<decltype(member)>>);

				using StrippedMemberType = std::remove_cvref_t<decltype(member)>;

				if constexpr(is_vector_v<StrippedMemberType>){
					constexpr auto ValueTypeRefl = std::meta::dealias(^^typename StrippedMemberType::value_type);
					static constexpr auto value_name = std::meta::has_identifier(ValueTypeRefl) ? std::meta::identifier_of(ValueTypeRefl) : std::meta::display_string_of(ValueTypeRefl);

					std::string tree_name = std::string("std::vector<") + value_name.data()
						+ "> : " + mem_name.data() + " - " + std::to_string(member.size());
					if(ImGui::TreeNode(tree_name.c_str())) {
						ImGui::PushID(reinterpret_cast<std::size_t>(member.data()));
						for(std::size_t i = 0; i < member.size(); i++){
							if(ImGui::TreeNode(std::to_string(i).c_str())){
								MetaImgui(member[i]);
								ImGui::TreePop();
							}
						}
						ImGui::PopID();
						ImGui::TreePop();
					}
					continue;
				}
				else{
					constexpr auto DealiasedReflType = std::meta::dealias(^^StrippedMemberType);
					if constexpr(std::meta::has_identifier(DealiasedReflType)){
						static constexpr auto identifier = std::meta::identifier_of(DealiasedReflType);
						std::string typed_name = identifier.data();
						typed_name += ":";
						typed_name += mem_name.data();
						if(ImGui::TreeNode(typed_name.c_str())){
							MetaImgui(member);
							ImGui::TreePop();
						}
					}
					else if constexpr(std::meta::has_template_arguments(DealiasedReflType)){
						static constexpr auto display_string = std::meta::display_string_of(DealiasedReflType);
						static constexpr auto template_of_id = std::meta::identifier_of(std::meta::template_of(DealiasedReflType));

						std::string template_name = std::string(RecursivelyGetTemplateName<DealiasedReflType>()) + ":";
						template_name += mem_name.data();

						if(ImGui::TreeNode(template_name.c_str())){
							MetaImgui(member);
							ImGui::TreePop();
						}

					}
					else{
						static constexpr auto display_string = std::meta::display_string_of(DealiasedReflType);
						static constexpr auto is_template = std::meta::is_template(DealiasedReflType);
						Log::Debug("no identifier : %s : %d\n", display_string.data(), is_template);

						if constexpr (std::meta::is_template(DealiasedReflType)){
							std::string typed_name = display_string.data();
							typed_name += ":";
							typed_name += mem_name.data();
							if(ImGui::TreeNode(typed_name.c_str())){
								MetaImgui(member);
								ImGui::TreePop();
							}
						}
						else{
							std::string typed_name = "invalid type reflection:";
							typed_name += mem_name.data();
							if(ImGui::TreeNode(typed_name.c_str())){
								MetaImgui(member);
								ImGui::TreePop();
							}
						}
					}
				}
			}
		}
		else if constexpr(std::is_floating_point_v<T>){
			ImGui::DragFloat(base_name.data(), &obj);
		}
		else if constexpr(std::integral<T>){
			ImGui::DragInt(base_name.data(), reinterpret_cast<int*>(&obj));
		}
		else if constexpr(std::is_same_v<T, std::string>){
			ImGui::Text(obj.c_str());
		}
		else{
			ImGui::Text(base_name.data());
		}

	}

    void SkeletonScene::Imgui(){
        if(ImGui::Begin("skeleton")){
            ImGui::Checkbox("load", &load_open);
        }
		ImGui::End();
		if(load_open){
			LoadFile();
		}
		if(ImGui::Begin("meta")){
			bool root_has_val = root.has_value();
			ImGui::Checkbox("root has val", &root_has_val);
			if(root.has_value()){
				MetaImgui(*root);

			}
		}
		ImGui::End();
    }

    void SkeletonScene::LoadFile(){
        explorer.enabled = load_open;
        explorer.state = ExplorerContext::State::Load;
        if(ImGui::Begin("file load")){
            explorer.Imgui();
            if(explorer.selected_file.has_value()){
                const std::filesystem::path load_path = *explorer.selected_file;

				Log::Debug("gltf parsing file : %s\n", load_path.string().c_str());
                root = gltf2cpp::parse(load_path.string().c_str());
                
                //const auto localized_path = std::filesystem::proximate(load_path, Global::worldMap->files.root_directory);
                //auto* wmd = Global::worldMap->Get(localized_path);
                //InitFromObject(*wmd);

                explorer.enabled = false;
                explorer.selected_file.reset();

				load_open = false;
            }
            if(!ImGui::IsWindowFocused()){
                //auto imwin = ImGui::GetCurrentWindow();
                explorer.enabled = false;
            }
        }
        ImGui::End();
    }
}