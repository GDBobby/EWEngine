#include "EWEngine/Systems/LeafSystem.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Assets/Image/Loader.h"

#include "EWEngine/Systems/TinyObjLoader.h"

#include "LAB/Support/Simple.h"
#include <vulkan/vulkan_core.h>


namespace EWE {

	LeafSystem::RandomSubObject::RandomSubObject()
	: dev{}, gen{ dev() }, 
		ellipseRatio{ 1.f,2.f }, 
		rotRatio{ 1.f, 4.f },
		motion{ 0, 100 }, 
		ellipseOsc{ 0.75f, 1.25f }, 
		angularFrequency{ lab::PI<float>, lab::GetPI(2.f) }, 
		initTime{ 0.f, 20.f },
		depthVariance{ -5.f, 5.f },
		widthVariance{ -80.f, 60.f }, 
		fallSwingVariance{ 5.f, 10.f }, 
		initHeightVariance{ 2.f, 40.f }, 
		variance{ -.5f, .5f },
		rockDist{ 1.75f, 2.25f }
	{}

	VmaAllocationCreateInfo GetVMAInfo(){
		return VmaAllocationCreateInfo {
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO,
			.requiredFlags = 0,
			.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.memoryTypeBits = 0,
			.pool = VK_NULL_HANDLE,
			.pUserData = nullptr,
			.priority = 1.f
		};
	}

	TaskRasterConfig GetTaskConfig(){
		TaskRasterConfig ret;
		ret.SetDefaults();
		ret.attachment_info.colors[0].format = engine->swapchain.swapCreateInfo.imageFormat;
		return ret;
	}

	//id like to move some of the random generation components to local scope on leaf generation, not sure which ones yet
	//id also like to attempt to move this entire calculation to the GPU on compute shaders
	LeafSystem::LeafSystem(std::filesystem::path const& filepath)
	: random{},
		vertices{engine->logicalDevice},
		indices{engine->logicalDevice},
		sceneBuffer{engine->logicalDevice, sizeof(SceneBufferObject), 1, GetVMAInfo()}, //per frame buffer should be mapped into 1 double wide buffer
		vertShader{engine->logicalDevice, engine->root_directory / "shaders/leaf.vert.spv"},
		fragShader{engine->logicalDevice, engine->root_directory / "shaders/leaf.frag.spv"},
		renderGraph{
			"leafsystem",
			engine->logicalDevice, engine->swapchain,
			engine->renderQueue, engine->computeQueue,
			engine->graphics_stc_task, engine->compute_stc_task
		},
		raster_pkg{"leaf raster task", engine->logicalDevice, engine->renderQueue, GetTaskConfig()},
		gpuTask{"leaf gpu task", engine->logicalDevice, engine->renderQueue},
		subTask{"leaf sub task", engine->logicalDevice, engine->renderQueue},
		renderInfo{ 
			"leaf render info", 
			engine->logicalDevice, engine->renderQueue, 
			raster_pkg.task_config.attachment_info, 
			engine->window.screenDimensions.width, engine->window.screenDimensions.height
		}
	{
		InitData(filepath);
		Record();

		engine->current_renderGraph = &renderGraph;
	}

	LeafSystem::~LeafSystem() {
	}

	void LeafSystem::Record(){
		obj_pkg.payload.config.SetDefaults();
		obj_pkg.payload.shaders[ShaderStage::Vertex] = &vertShader;
		obj_pkg.payload.shaders[ShaderStage::Fragment] = &fragShader;

		raster_pkg.objectPackages.push_back(&obj_pkg);
		raster_pkg.scissor = EWE::engine->window.screenDimensions;
		raster_pkg.viewport.x = 0.f;
		raster_pkg.viewport.y = static_cast<float>(EWE::engine->window.screenDimensions.height);
		raster_pkg.viewport.width = static_cast<float>(EWE::engine->window.screenDimensions.width);
		raster_pkg.viewport.height = -static_cast<float>(EWE::engine->window.screenDimensions.height);
		raster_pkg.viewport.minDepth = 0.0f;
		raster_pkg.viewport.maxDepth = 1.f;
		raster_pkg.Compile();
		raster_pkg.Undefer(renderInfo);

		pkg_record.name = "leaf pkg record";
		pkg_record.queue = &engine->renderQueue;
		pkg_record.packages.push_back(&raster_pkg);

		gpuTask.pkgRecord = &pkg_record;
		//gpuTask.paramPool.emplace(pkg_record.Compile());
		gpuTask.GenerateWorkload();

		subTask.tasks.push_back(&gpuTask);
		subTask.uses_present_image = true;
		//subTask.CollectTaskWorkloads();
		{
			auto& sub_group_back = renderGraph.execution_order.emplace_back();
			sub_group_back.push_back(&subTask);
		}

		const EWE::UsageData<EWE::Image> initial_acquire_usage{ //this is the usage after acquirement
			.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		subTask.packaged_tasks.push_back(
			[&](EWE::CommandBuffer& cmdBuf, uint8_t frameIndex) {

                VkRenderingAttachmentInfo presentAttachmentInfo{
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .pNext = nullptr,
                    .imageView = engine->swapchain.GetCurrentImageView(),
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .resolveImageView = VK_NULL_HANDLE,
                    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = {0.f, 0.f, 0.f, 0.f}
                };
                auto& vri = raster_pkg.deferred_vk_render_info->GetRef(frameIndex);
                vri.colorAttachmentCount = 1;
                vri.pColorAttachments = &presentAttachmentInfo;

                gpuTask.workload(cmdBuf, frameIndex);
                renderGraph.presentBridge.Execute(cmdBuf);
                return true;
            }
		);

		renderGraph.tasks.push_back(&gpuTask);

		present_img_att_index = gpuTask.resources.AddResource<EWE::Image>(initial_acquire_usage);
		renderGraph.syncManager.AddAcquisition_Image(gpuTask, present_img_att_index);

    	renderGraph.InitializeSemaphores();
		renderGraph.presentBridge.final_swap_img_usage = &gpuTask.resources.images[present_img_att_index];
		renderGraph.swap_image_instances.emplace_back(&gpuTask, present_img_att_index);
		//renderInfo.render_data
	}

	void LeafSystem::InitData(std::filesystem::path const& filepath) {

		LoadLeafModel(filepath);
		//LoadLeafTexture();
		sceneBuffer[0].SetName("leaf instance[0]");
		sceneBuffer[1].SetName("leaf instance[1]");

		LeafPhysicsInitialization();
	}

	////this should be a graphics queue command buffer
	void LeafSystem::LoadLeafModel(std::filesystem::path const& filepath) {
		static constexpr VertexProperty LeafVertProperties{
			.position = true,
			.normal = true,
			.tangent = false,
			.uv = false
		};
		using LeafIndexType = uint16_t;
		using LeafModelType = Model<LeafVertProperties, LeafIndexType>;
		LeafModelType leafModel = LoadModelTinyObj<LeafVertProperties, LeafIndexType>(engine->root_directory / filepath);
		vertices.Init(sizeof(Vertex<LeafVertProperties>) * leafModel.vertices.size(), 1, GetVMAInfo());
		indices.Init(sizeof(LeafIndexType) * leafModel.indices.size(), 1, GetVMAInfo(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

		{
			obj_pkg.paramPool.PushBack(Inst::Push);
			ParamPack<Inst::DrawIndexed> indexPack{
				.indexBuffer = IndexBuffer{
					.buffer = indices.buffer_info.buffer,
					.offset = 0,
					.indexType = VK_INDEX_TYPE_UINT16
				},
				.indexCount = leafModel.indices.size(),
				.instanceCount = LEAF_COUNT,
				.firstIndex = 0,
				.vertexOffset = 0,
				.firstInstance = 0
			};
			obj_pkg.paramPool.PushBack(indexPack);

			pushPacks = obj_pkg.paramPool.param_data[0].CastTo<ParamPack<Inst::Push>>();
			for_each_frame{
				auto& pushPack = pushPacks->GetRef(frame);
				pushPack.size = 0;
				pushPack.buffer_count = 2;
				pushPack.texture_count = 0;
				pushPack.size = pushPack.Size();
				pushPack.GetDeviceAddress(0) = vertices.deviceAddress;
				pushPack.GetDeviceAddress(1) = sceneBuffer[frame].deviceAddress;
			}

		}
	}
	void LeafSystem::LoadLeafTexture() {
		/*
		const std::string fullLeafTexturePath = "textures/leaf.jpg";
		//Image::CreateImage(&leafImageInfo, fullLeafTexturePath, false);

		leafImgID = Image_Manager::GetCreateImageID(fullLeafTexturePath, false);

		const std::string leafTexturePath = "leaf.jpg";
		*/
	}


	void LeafSystem::RenderLoop(std::function<bool()> cancel_func) {
		
		for_each_frame{
			sceneBuffer[frame].Map();
		}

		while(!cancel_func()){
			if(engine->render_loop_timer.ReadyForRenderUpdate()){

                glfwPollEvents();

                if (renderGraph.Acquire(EWE::engine->frameIndex)) {
                    //mouseData.UpdatePosition(EWE::engine->window.window);
                    renderGraph.UpdateSwapImage(EWE::engine->frameIndex);

					//it seems like the semaphore is created correctly
                    renderGraph.RecreateBarriers(EWE::engine->frameIndex);

                    renderGraph.Execute(EWE::engine->frameIndex);

                    EWE::engine->frameIndex = (EWE::engine->frameIndex + 1) % EWE::max_frames_in_flight;
                    engine->totalFramesSubmitted++;
                }
			}
		}
		for_each_frame{
			sceneBuffer[frame].Unmap();
		}

		//leafModel->BindAndDrawInstanceNoBuffer(LEAF_COUNT);
	}


	void LeafSystem::LeafPhysicsInitialization(){
		leafs.resize(LEAF_COUNT);
		
		for (uint16_t i = 0; i < LEAF_COUNT; i++) {
			auto& leaf = leafs[i];
			//0.7071067811865475
			//origin -= average velocity * randomTime; 

			//average velocity

			//give a starting position in a box, then subtract position by an inverse amount of time


			leaf.angF = random.angularFrequency(random.gen);
			leaf.ellRatio = random.ellipseRatio(random.gen);
			leaf.rotRatio = random.rotRatio(random.gen);
			leaf.ellOsc = random.ellipseOsc(random.gen);
			leaf.fallAmplitude = random.fallSwingVariance(random.gen);
			leaf.swingAmplitude = random.fallSwingVariance(random.gen);
			leaf.rotSpeed = random.angularFrequency(random.gen);


			float depth = random.depthVariance(random.gen);
			float width = random.widthVariance(random.gen);
			static constexpr float half_sqrt_2 = lab::Sqrt(2.f) / 2.f;
			leaf.origin = lab::vec3{ half_sqrt_2 * (width - depth) - 3.f, 0.f, half_sqrt_2 * (depth - width) - 3.f };

			int motDis = random.motion(random.gen);
			if (motDis < 10) {
				leaf.fallMotion = LF_Steady;
				leaf.transform.rotation.x = -lab::GetPI_DividedBy(2.f) * .9f;
				leaf.averageVelocity = lab::vec3(WIND_SPEED, gravity * lab::PI<float> * 9.f / 4.f, -WIND_SPEED);
				leaf.transform.translation = leaf.origin - leaf.averageVelocity * (20.f - random.initTime(random.gen) * 0.666f);
			}
			else if (motDis < 43) {
				leaf.fallMotion = LF_Fluttering;
				//velocity.y = (gravity + (leaf.swingAmplitude * lab::Sin(2.f * leaf.angF * leaf.time)) / leaf.rotRatio) * leaf.ellOsc
				leaf.averageVelocity = lab::vec3(WIND_SPEED, gravity * 3.f, -WIND_SPEED);
				leaf.transform.translation = leaf.origin - leaf.averageVelocity * (20.f - random.initTime(random.gen));
			}
			else if (motDis < 75) {
				leaf.fallMotion = LF_Chaotic;
				leaf.time += random.initTime(random.gen);
				leaf.averageVelocity = lab::vec3(WIND_SPEED, gravity * 3.f, -WIND_SPEED);
				leaf.transform.translation = leaf.origin - leaf.averageVelocity * (20.f - random.initTime(random.gen));
				//velocity.y = (gravity + (leaf.swingAmplitude * lab::Sin(2.f * leaf.angF * leaf.time)) / leaf.rotRatio) * leaf.ellOsc;
			}
			else {
				leaf.fallMotion = LF_Spiral;
				leaf.averageVelocity = lab::vec3(WIND_SPEED, gravity * 3.f, -WIND_SPEED);
				leaf.origin -= leaf.averageVelocity * 20.f;
				leaf.time = random.initTime(random.gen);
			}
			//leaf.fallMotion = (LeafFallMotions)motion(random.gen);

			//leaf.transform.translation = leaf.origin;
			//fallSwingVariance(random.gen);

			for_each_frame{
				sceneData[frame].transforms[i] = leaf.transform.GetMatrix();
			}
		}

		//do i need each buffer initialized on the GPU? i think i can skip that
		Log::Warning("i think i can skip initialization on gpu, not sure\n");
		//if i was doing a compute shader update, i should initialize on gpu
		//for_each_frame{
		//	memcpy(sceneBuffer[frame].GetMapped(), &sceneData[frame], sizeof(SceneBufferObject));
		//	sceneBuffer[frame].Flush();
		//}

	}

	void LeafSystem::FallCalculation(float timeStep) {
		//angF = angularFrequency * timestep
		//EllRatio = ratio of minor to major axis in ellipse
		//rotRatio = ratio of elliptical oscillation to rotation of leaf itself
		for(uint16_t i = 0; i < LEAF_COUNT; i++){
			auto& leaf = leafs[i];

			leaf.time += timeStep;
			//leaf.time = lab::clamp(leaf.time, 0.f, lab::GetPI(2.f));

			//dw/dt = change in leaf angF
			//theta = angle with xy plane
			//	  ^ = sin(transform.rotation.x);
			//a = angle with xz plane
			//^ = dot(normalize(velocity), lab::vec3{0.f,-1.f,0.f});
			//V is velocity
			//p = density of leaf

			//Ka = friction in the direction of the fall
			lab::vec3 velocity{ 0.f };
			float angFT = leaf.angF * leaf.time;

			switch (leaf.fallMotion) {
				case LF_Steady: {
					//leaf.transform.translation.x += variance(random.gen);
					//leaf.transform.translation.y = leaf.origin.y + gravity * leaf.time;
					//leaf.transform.translation.z += variance(random.gen);

					//velocity = lab::vec3(0.f);

					velocity.x = random.variance(random.gen) + (WIND_SPEED * leaf.ellOsc); //ellOsc for wind variance
					velocity.y = gravity * 1.5f * leaf.rotSpeed; //ellOsc isnt related but im plugging it for gravity variance
					velocity.z = random.variance(random.gen) - (WIND_SPEED * leaf.ellOsc);

					leaf.transform.translation += velocity * timeStep;
					leaf.transform.rotation.y += leaf.rotSpeed * 2.f * timeStep;
					break;
				}
				case LF_Tumbling:
				case LF_Fluttering:
					leaf.transform.rotation.y -= lab::GetPI_DividedBy(2.f) * timeStep * leaf.ellOsc; //ellOsc isnt related but im plugging it cause it fits
					velocity.x = -leaf.fallAmplitude * lab::Cos(angFT) * lab::Sin(leaf.transform.rotation.y)
						+ (WIND_SPEED * leaf.ellOsc); //wind, ellOsc for variance
					velocity.y = (gravity + (leaf.swingAmplitude * lab::Sin(2.f * leaf.angF * leaf.time)) / leaf.rotRatio) * leaf.ellOsc * 2.f; //JUST USING ROTRATIO AND ELLOSC FOR GRAVITY VARIANCE
					velocity.z = -leaf.fallAmplitude * lab::Cos(angFT) * lab::Sin(leaf.transform.rotation.y)
						- (WIND_SPEED * leaf.ellOsc); //wind, ellOsc for variance
					leaf.transform.translation += velocity * timeStep;
					leaf.transform.rotation.z = -leaf.swingAmplitude * lab::Sin(leaf.angF * (leaf.time - leaf.angF * 1.f / 16.f)) / (lab::GetPI(2.f)) * lab::Sin(leaf.transform.rotation.y) * .75f;
					break;


				case LF_Chaotic:
					//velocity.z = -leaf.fallAmplitude * lab::Sin(leaf.angF * leaf.time);
					//all going clockwise

					leaf.transform.rotation.y += lab::GetPI_DividedBy(2.f) * timeStep * leaf.ellOsc;
					velocity.x = -leaf.fallAmplitude * lab::Cos(angFT) * lab::Sin(leaf.transform.rotation.y)
						+ (WIND_SPEED * leaf.ellOsc); //wind, ellOsc for variance
					velocity.y = (gravity + (leaf.swingAmplitude * lab::Sin(2.f * leaf.angF * leaf.time)) / leaf.rotRatio) * leaf.ellOsc * 2.f;//JUST USING ROTRATIO AND ELLOSC FOR GRAVITY VARIANCE
					velocity.z = -leaf.fallAmplitude * lab::Cos(angFT) * lab::Sin(leaf.transform.rotation.y)
						- (WIND_SPEED * leaf.ellOsc); //wind, ellOsc for variance
					leaf.transform.translation += velocity * timeStep;
					leaf.transform.rotation.z = -leaf.swingAmplitude * lab::Sin(leaf.angF * (leaf.time - leaf.angF * 1.f / 16.f)) / (lab::GetPI(2.f)) * lab::Sin(leaf.transform.rotation.y) * .75f;
					//leaf.transform.rotation.x = -leaf.swingAmplitude * lab::Sin(leaf.angF * (leaf.time - leaf.angF * 1.f / 16.f)) / (lab::GetPI(2.f)) * lab::Cos(leaf.transform.rotation.y) / 2.f;

					break;
					/*
				case LF_Tumbling:
				case LF_Fluttering: {
					//leaf.transform.translation.x = leaf.origin.x - (leaf.fallAmplitude / leaf.angF) * lab::Sin(leaf.angF * leaf.time);
					//leaf.transform.translation.y = leaf.origin.y + (gravity * leaf.time) - ((leaf.swingAmplitude/(2.f * leaf.angF)) * lab::Cos(2.f * leaf.angF * leaf.time));
					//velocity = lab::vec3(0.f);

					velocity.x = -leaf.fallAmplitude * lab::Cos(angFT);
					velocity.y = gravity + leaf.swingAmplitude * lab::Sin(2.f * angFT);
					//velocity.z = variance(random.gen);


					leaf.transform.translation += velocity * timeStep;
					leaf.transform.rotation.z = -leaf.swingAmplitude * lab::Sin(leaf.angF * (leaf.time - leaf.angF * 1.f / 16.f)) / (lab::GetPI(2.f));

					break;
				}
				*/
				case LF_Helix:
				case LF_Spiral: {
					//velocity = lab::vec3(0.f);
					const lab::vec3 oldPos = leaf.transform.translation;

					leaf.transform.translation.x = leaf.origin.x + leaf.ellOsc * cos(angFT / 2.f) * (10.f + leaf.ellRatio * sin(leaf.rotRatio * angFT))
						+ (WIND_SPEED * leaf.time * leaf.ellOsc); //wind, ellOsc for variance							
					leaf.transform.translation.y = leaf.origin.y + gravity * leaf.time * leaf.ellOsc * 1.9f; //maybe multiply gravity by some value between 0.8 and 0.9
					leaf.transform.translation.z = leaf.origin.z + leaf.ellOsc * sin(angFT / 2.f) * (10.f + leaf.ellRatio * sin(leaf.rotRatio * angFT))
						- (WIND_SPEED * leaf.time * leaf.ellOsc); //wind, ellOsc for variance

					velocity = (leaf.transform.translation - oldPos) / timeStep;
					//leaf.rotRef = lab::mod(leaf.rotRef + , lab::GetPI_DividedBy(2.f) / 2.f);
					//leaf.transform.rotation.z = leaf.rotRef - lab::GetPI_DividedBy(2.f) / 4.f;

					//leaf.transform.rotation.z = lab::Sin(leaf.rotRef);
					const float horiPerc = 1.f - (velocity.y / lab::Magnitude(velocity));

					//leaf.transform.rotation.x = -lab::Cos(horiPerc) * lab::GetPI_DividedBy(4.f);
					leaf.transform.rotation.x = -lab::Sin(horiPerc * lab::GetPI_DividedBy(4.f));
					leaf.transform.rotation.y = lab::ArcTan2(velocity.x, velocity.z) + (lab::GetPI(3.f / 4.f));
					leaf.transform.rotation.x = -lab::Cos(horiPerc * lab::GetPI_DividedBy(4.f));
					//leaf.transform.rotation.z = -lab::atan(velocity.x, velocity.y) - (lab::GetPI_DividedBy(4.f) * 3.f);

					// if velocity.y == gravity, i want rotation.y to be half pi
					//if velocity.y == 0, i want rotation.y to be 0
					//or the other way around?

					break;
				}
			}

			if (leaf.transform.translation.y <= 0.f) {
				//roll a new motion type?
				//leaf.transform.translation = leaf.origin;
				leaf.transform.translation = leaf.origin - (leaf.averageVelocity * 20.f);

				//leaf.origin = leaf.transform.translation;
				leaf.time = 0.f;

				/*
				if (leaf.fallMotion == LF_Chaotic) {
					leaf.time += random.initTime(random.gen);
				}
				*/
				//leaf.transform.rotation = lab::vec3(0.f, 0.f, -lab::GetPI_DividedBy(4.f));
				//leaf.transform.rotation.x = lab::GetPI_DividedBy(2.f);
			}
			else {

				//forwardDir = { sin(player.FollowCamera->transform.rotation.y), -sin(player.FollowCamera->transform.rotation.x), cos(player.FollowCamera->transform.rotation.y) };
				//^ following that logic
				//leaf.transform.rotation = lab::vec3(lab::asin(velNorm.y), lab::asin(velNorm.x), lab::acos(velNorm.y));


				//get direction of velocity, reverse that into rotation


				//leaf.transform.rotation.z += (-4 * leaf.transform.rotation.z - (3.f * lab::pi<float>() * leafDensity * (velocityXMag + velocityYMag) * lab::Cos(beta) * lab::Sin(beta))) * timeStep;
				//(-3.f * lab::pi<float>() * leafDensity * (velocityXMag + velocityYMag) * lab::Cos(beta) * lab::Sin(beta)) dt


				//lab::mod(leaf.transform.rotation.z, lab::GetPI(2.f));
			}



			//leaf.transform.mat4(mappedTransformData[engine->frameIndex] + (sizeof(lab::mat4) / sizeof(float) * i));
			sceneData[engine->frameIndex].transforms[i] = leaf.transform.GetMatrix();

		}
		memcpy(sceneBuffer[engine->frameIndex].GetMapped(), &sceneData[engine->frameIndex], sizeof(SceneBufferObject));
		sceneBuffer[engine->frameIndex].Flush();

	}
} //namespace EWE