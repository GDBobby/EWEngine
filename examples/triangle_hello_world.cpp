//example
#include "EightWinds/Reflect/Enum.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Global.h"

#include "EightWinds/Pipeline/Layout.h"
#include "EightWinds/Shader.h"
#include "EightWinds/DescriptorImageInfo.h"

#include "EightWinds/Command/Record.h"
#include "EightWinds/Command/Execute.h"
#include "EightWinds/RenderGraph/GPUTask.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"

#include "EWEngine/Tools/ImguiHandler.h"

#include "EightWinds/RenderGraph/RasterTask.h"

#include "EWEngine/Reflect/ImguiReflection.h"

#include "EWEngine/NodeGraph/InstructionPackage_NG.h"
#include "EWEngine/NodeGraph/RenderGraph_NG.h"
#include "EWEngine/NodeGraph/RasterTask_NG.h"
#include "EWEngine/NodeGraph/Record_NG.h"
#include "EWEngine/NodeGraph/InstructionPackage_NG.h"

#include "EWEngine/InputData.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <array>
#include <thread>

#if EWE_DEBUG_BOOL
void PrintAllExtensions(VkPhysicalDevice physicalDevice) {
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> extProps(extCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, extProps.data());
    EWE::Logger::Print<EWE::Logger::Debug>("available extensions --\n");
    for (auto& prop : extProps) {
        EWE::Logger::Print<EWE::Logger::Debug>("\t%s\n", prop.extensionName);
    }
}
#endif


void DemoWindowInputs();

template<std::meta::info T>
std::string GetMetaInfo_Temp(){
    if constexpr(std::meta::is_complete_type(T)){
        return "complete type";
        if constexpr(std::meta::is_class_type(T)){
            return "complete class type";
        }
    }
    else if constexpr(std::meta::is_type(T)){
        return "incomplete type";
        if constexpr(std::meta::is_class_type(T)){
            return "incomplete class";
        }
    }
    return "none";
}

int main() {


#if EWE_DEBUG_BOOL
    EWE::Logger::Print<EWE::Logger::Debug>("starting working dir - %s\n", std::filesystem::current_path().string().c_str());
#endif

#ifdef USING_NVIDIA_AFTERMATH
    EWE::Logger::Print<EWE::Logger::Debug>("using nvidia aftermath - %s\n", std::filesystem::current_path().string().c_str());
#endif

#if defined(__SANITIZE_ADDRESS__)
    EWE::Logger::Print<EWE::Logger::Debug>("compiled with asan\n");
#endif

    std::hash<std::filesystem::path> hash{};
    hash(std::filesystem::current_path());

    //need to fix htis. its something with my windows debugger
    auto current_working_directory = std::filesystem::current_path();
    if (current_working_directory.parent_path().parent_path().stem() == "build") {
        current_working_directory = current_working_directory.parent_path().parent_path().parent_path();
#if EWE_DEBUG_BOOL
        EWE::Logger::Print<EWE::Logger::Normal>("build redacted working dir - %s\n", current_working_directory.string().c_str());
#endif
    }
    else if (current_working_directory.parent_path().stem() == "build") {
        current_working_directory = current_working_directory.parent_path().parent_path();
    }
    if (current_working_directory.stem() == "EWEngine") {
        current_working_directory = current_working_directory / "examples";
    }
    std::filesystem::current_path(current_working_directory);
#if EWE_DEBUG_BOOL
    EWE::Logger::Print<EWE::Logger::Normal>("current dir - %s\n", std::filesystem::current_path().string().c_str());
#endif

    uint32_t extensionCount = 0;
    EWE::EWE_VK(vkEnumerateInstanceExtensionProperties, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    EWE::EWE_VK(vkEnumerateInstanceExtensionProperties, nullptr, &extensionCount, extensions.data());

    EWE::EWEngine engine{ "triangle hello world" };
    EWE::LogicalDevice& logicalDevice = engine.logicalDevice;

    EWE::Queue* renderQueue = nullptr;
    for (auto& queue : logicalDevice.queues) {
        if (queue.family.SupportsSurfacePresent() && queue.family.SupportsGraphics()) {
            renderQueue = &queue;
            break;
        }
    }
    if (renderQueue == nullptr) {
#if EWE_DEBUG_BOOL
        EWE::Logger::Print<EWE::Logger::Error>("failed to find a render queue, exiting\n");
#endif
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return -1;
    }
#if EWE_DEBUG_NAMING
    renderQueue->SetName("render queue");
#endif

    EWE::Input::Mouse mouseData{};
    mouseData.TakeCallbackControl();

    glfwSetMouseButtonCallback(EWE::Global::window->window, EWE::Input::Mouse::MouseCallback);
    EWE::ImguiHandler imguiHandler{ *renderQueue, 3, VK_SAMPLE_COUNT_1_BIT };

    //pipeline
    EWE::Shader* triangle_vert = EWE::Global::shaders->Get("basic.vert.spv");
    EWE::Shader* triangle_frag = EWE::Global::shaders->Get("basic.frag.spv");

    EWE::PipeLayout triangle_layout(logicalDevice, std::initializer_list<EWE::Shader*>{ triangle_vert, triangle_frag });
    //passconfig should be using a full rendergraph setup
    EWE::TaskRasterConfig passConfig;
    passConfig.SetDefaults();
    passConfig.pipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D16_UNORM;
    passConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
    passConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
    passConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    passConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    passConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;

    passConfig.attachment_set_info.colors.clear();
    passConfig.attachment_set_info.width = EWE::Global::window->screenDimensions.width;
    passConfig.attachment_set_info.height = EWE::Global::window->screenDimensions.height;
    passConfig.attachment_set_info.renderingFlags = 0;

    auto& color_back = passConfig.attachment_set_info.colors.emplace_back();
    color_back.format = VK_FORMAT_R8G8B8A8_UNORM;
    color_back.clearValue.color.float32[0] = 0.f;
    color_back.clearValue.color.float32[1] = 0.f;
    color_back.clearValue.color.float32[2] = 0.f;
    color_back.clearValue.color.float32[3] = 0.f;
    color_back.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_back.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    auto& depth_temp = passConfig.attachment_set_info.depth;
    depth_temp.format = VK_FORMAT_D16_UNORM;
    depth_temp.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_temp.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_temp.clearValue.depthStencil.depth = 0.f;
    depth_temp.clearValue.depthStencil.stencil = 0; //idk what to set this to tbh

    EWE::FullRenderInfo triangle_render_info{
        "triangle",
        logicalDevice, *renderQueue,
        passConfig.attachment_set_info
    };

    EWE::RasterTask triangle_raster_task{"triangle raster task", logicalDevice, *renderQueue, passConfig, &triangle_render_info};

    EWE::ObjectRasterData triangle_rasterObj;
    triangle_rasterObj.layout = &triangle_layout;
    triangle_rasterObj.config.SetDefaults();
    triangle_rasterObj.config.cullMode = VK_CULL_MODE_NONE;
    triangle_rasterObj.config.depthClamp = false;
    triangle_rasterObj.config.rasterizerDiscard = false;
    triangle_rasterObj.config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
     
    //VertexDrawData has a copy of a deferred<VertexParamPack> that will be automatically adjusted
    //also has a push constant, which will be automatically applied
    EWE::VertexDrawData triangle_drawData{};
    triangle_drawData.use_labelPack = true;
    triangle_raster_task.AddDraw(triangle_rasterObj, &triangle_drawData);


    //EWE::Node::Graph nodeGraph{};
    //nodeGraph.Record(triangle_raster_task);

    EWE::Command::Record triangleRecord{};
    triangle_raster_task.Record(triangleRecord, true);

    EWE::RenderGraph& renderGraph = engine.renderGraph;
    EWE::GPUTask& renderTask = renderGraph.tasks.AddElement("main render", logicalDevice, *renderQueue, triangleRecord);

    triangle_raster_task.scissor = EWE::Global::window->screenDimensions;
    triangle_raster_task.viewport.x = 0.f;
    triangle_raster_task.viewport.y = static_cast<float>(EWE::Global::window->screenDimensions.height);
    triangle_raster_task.viewport.width = static_cast<float>(EWE::Global::window->screenDimensions.width);
    triangle_raster_task.viewport.height = -static_cast<float>(EWE::Global::window->screenDimensions.height);
    triangle_raster_task.viewport.minDepth = 0.0f;
    triangle_raster_task.viewport.maxDepth = 1.f;
    triangle_raster_task.AdjustPipelines();
    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        if (triangle_drawData.deferred_label != nullptr) {
            auto& triangle_label = triangle_drawData.deferred_label->GetRef(i);
            triangle_label.name = "triangle";
            triangle_label.red = 1.f;
            triangle_label.green = 0.f;
            triangle_label.blue = 0.f;
        }
    }

    //nodeGraph.Undefer();


    VmaAllocationCreateInfo vmaAllocInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = 0,
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .memoryTypeBits = 0,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
        .priority = 1.f
    };

    struct TriangleVertex {
        lab::vec2 pos; //xy
        lab::vec3 color; //rgb, the 4th element isnt read, but i need it for alignment
    };
    for (auto& str : triangle_vert->BDA_data) {
        if (str.name == "Vertex") {

#if EWE_DEBUG_BOOL
            EWE::Logger::Print<EWE::Logger::Debug>("size comparison - %zu : %zu\n", str.size, sizeof(TriangleVertex));
            //im getting fucked by spv. its forcing 32bit alignment, even tho spirvcross says 20bit
            //EWE_ASSERT(str.size == sizeof(TriangleVertex));
#endif
        }
    }
    EWE::Buffer vertex_buffer{ logicalDevice, sizeof(TriangleVertex) * 3, 1, vmaAllocInfo, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT };
#if EWE_DEBUG_NAMING
    vertex_buffer.SetName("vertex buffer");
#endif
    {
        TriangleVertex* mappedData = reinterpret_cast<TriangleVertex*>(vertex_buffer.Map());

        mappedData[0].pos[0] = -0.5f;
        mappedData[0].pos[1] = -0.5f;

        mappedData[0].color[0] = 1.f;
        mappedData[0].color[1] = 0.f;
        mappedData[0].color[2] = 0.f;

        mappedData[1].pos[0] = 0.f;
        mappedData[1].pos[1] = 0.5f;

        mappedData[1].color[0] = 0.f;
        mappedData[1].color[1] = 1.f;
        mappedData[1].color[2] = 0.f;

        mappedData[2].pos[0] = 0.5f;
        mappedData[2].pos[1] = -0.5f;

        mappedData[2].color[0] = 0.f;
        mappedData[2].color[1] = 0.f;
        mappedData[2].color[2] = 1.f;

        vertex_buffer.Flush();
        vertex_buffer.Unmap();
    }

    triangle_drawData.buffers[0] = &vertex_buffer;
    if (triangle_drawData.deferred_push != nullptr) {
        triangle_drawData.UpdateBuffer();
    }
    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        if (triangle_drawData.paramPack != nullptr) {
            triangle_drawData.paramPack->GetRef(i).firstInstance = 0;
            triangle_drawData.paramPack->GetRef(i).firstVertex = 0;
            triangle_drawData.paramPack->GetRef(i).instanceCount = 1;
            triangle_drawData.paramPack->GetRef(i).vertexCount = 3;
        }
    }

    passConfig.pipelineRenderingCreateInfo.pColorAttachmentFormats = &engine.swapchain.swapCreateInfo.imageFormat;
    EWE::Shader* merge_vert = EWE::Global::shaders->Get("merge.vert.spv");
    EWE::Shader* merge_frag = EWE::Global::shaders->Get("merge.frag.spv");
    EWE::PipeLayout merge_layout(logicalDevice, std::initializer_list<EWE::Shader*>{ merge_vert, merge_frag });

    EWE::Command::Record mergeRecord{};

    auto color_temp = passConfig.attachment_set_info.colors[0];
    //by poppin the color and then putting it back, the image won't be constructed with the raster task
    //then putting it back in after construction, the pipelines will be constructed correctly
    passConfig.attachment_set_info.colors.clear(); 
    passConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    
    EWE::FullRenderInfo merge_render_info{
        "merge",
        logicalDevice, *renderQueue,
        passConfig.attachment_set_info
    };
    EWE::RasterTask mergeRaster{ "merge raster", logicalDevice, *renderQueue, passConfig, &merge_render_info };
    color_temp.format = engine.swapchain.swapCreateInfo.imageFormat;
    mergeRaster.task_config.attachment_set_info.colors.push_back(color_temp);

    mergeRaster.scissor = triangle_raster_task.scissor;
    mergeRaster.viewport = triangle_raster_task.viewport;

    EWE::ObjectRasterData merge_rasterObj{
        .layout = &merge_layout,
        .config = triangle_rasterObj.config
    };
    EWE::VertexDrawData merge_drawData{};
    mergeRaster.AddDraw(merge_rasterObj, &merge_drawData);
    mergeRaster.Record(mergeRecord);

    EWE::GPUTask& mergeTask = renderGraph.tasks.AddElement(
        "merge task",
        logicalDevice, *renderQueue,
        mergeRecord
    );
    mergeRaster.AdjustPipelines();

    for (uint8_t i = 0; i < EWE::max_frames_in_flight; i++) {
        merge_drawData.paramPack->GetRef(i).firstInstance = 0;
        merge_drawData.paramPack->GetRef(i).firstVertex = 0;
        merge_drawData.paramPack->GetRef(i).vertexCount = 4;
        merge_drawData.paramPack->GetRef(i).instanceCount = 1;
    }

    EWE::UsageData<EWE::Image> initial_acquire_usage{
        .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    uint32_t render_att_index = renderTask.resources.AddResource(triangle_raster_task.renderInfo->full.color_images[0], initial_acquire_usage);
    renderGraph.syncManager.AddAcquisition_Image(renderTask, render_att_index);
    uint32_t present_img_att_index = mergeTask.resources.AddResource<EWE::Image>(initial_acquire_usage);
    renderGraph.syncManager.AddAcquisition_Image(mergeTask, present_img_att_index);

    EWE::GPUTask& imguiTask{ renderGraph.tasks.AddElement("imgui", logicalDevice, *renderQueue) };

    uint32_t imgui_att_index = imguiTask.resources.AddResource(imguiHandler.renderInfo.full.color_images[0], initial_acquire_usage);
    renderGraph.syncManager.AddAcquisition_Image(imguiTask, imgui_att_index);

    EWE::UsageData<EWE::Image> merge_acquire_usage{
        .stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .accessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    uint32_t acquire_render_output_index = mergeTask.resources.AddResource(triangle_raster_task.renderInfo->full.color_images[0], merge_acquire_usage);
    renderGraph.syncManager.AddTransition_Image(renderTask, render_att_index, mergeTask, acquire_render_output_index);
    uint32_t acquire_imgui_output_index = mergeTask.resources.AddResource(imguiHandler.renderInfo.full.color_images[0], merge_acquire_usage);
    renderGraph.syncManager.AddTransition_Image(imguiTask, imgui_att_index, mergeTask, acquire_imgui_output_index);
    //renderGraph.syncManager.PopulateAffixes(); //from here, no more resources can be added

    renderGraph.presentBridge.SetSubresource(
        VkImageSubresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    );

    renderTask.GenerateWorkload();
    mergeTask.GenerateWorkload();

    EWE::SubmissionTask& world_render_submission = renderGraph.submissions.AddElement(*EWE::Global::logicalDevice, *renderQueue, "world render");
    world_render_submission.packaged_tasks.push_back(renderTask.workload);
    EWE::SubmissionTask& imgui_submission = renderGraph.submissions.AddElement(*EWE::Global::logicalDevice, *renderQueue, "imgui");

    imguiHandler.Begin();
    EWE::Node::RenderGraphNodeGraph rgng{renderGraph};
    //EWE::Node::RasterTaskNodeGraph rtng{};
    EWE::Node::RecordNodeGraph rng{};
    EWE::Node::InstructionPackageNodeGraph ipng{};
    imguiHandler.End();

    auto imgui_port_info = [&](EWE::ImguiViewport& vp){
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        //ImGui::SetNextWindowPos(viewport->Pos);
        //ImGui::SetNextWindowSize(viewport->Size);
        //if(ImGui::Begin("##", nullptr, main_window_flags)){

            const std::string extension_string = std::string("##") + std::to_string(reinterpret_cast<std::size_t>(&vp));

            //ImGui::SetNextWindowCollapsed(false);
            //DemoWindowInputs();

            ImGui::Separator();

            ImGui::BulletText("context address : %zu", reinterpret_cast<std::size_t>(ImGui::GetCurrentContext()));
            ImGui::BulletText("HoveredWindow : %zu", ImGui::GetCurrentContext()->HoveredWindow);
            ImGui::BulletText("current window : %zu", ImGui::GetCurrentContext()->CurrentWindow);

            ImGui::BulletText("pos : %.2f:%.2f", ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
            ImGui::BulletText("size : %.2f:%.2f", ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
            
            lab::ivec2 difference{vp.current_viewport.offset.x, vp.current_viewport.offset.y};
            if(ImGui::DragInt2((std::string("offset") + extension_string).c_str(), &vp.current_viewport.offset.x, 1.f, 0, EWE::Global::window->screenDimensions.width)){
                difference.x -= vp.current_viewport.offset.x;
                difference.y -= vp.current_viewport.offset.y;

                vp.current_viewport.extent.width += difference.x;
                vp.current_viewport.extent.height += difference.y;
            }
            else{
                vp.current_viewport.offset.x = ImGui::GetWindowPos().x;
                vp.current_viewport.offset.y = ImGui::GetWindowPos().y;

                vp.current_viewport.extent.width = ImGui::GetWindowSize().x;
                vp.current_viewport.extent.height = ImGui::GetWindowSize().y;
            }
            auto current_im_vp = ImGui::GetWindowViewport();
            current_im_vp->Pos.x = vp.current_viewport.offset.x;
            current_im_vp->Pos.y = vp.current_viewport.offset.y;
            
            ImGui::DragInt2("extent", reinterpret_cast<int*>(&vp.current_viewport.extent.width), 1.f, 0.f, EWE::Global::window->screenDimensions.width - vp.current_viewport.offset.x);

            std::string butt_str = std::string("split right##") + extension_string;
            if(ImGui::Button(butt_str.c_str())){
                vp.current_viewport.extent.width /= 2;
                //vp.current_viewport.extent.height /= 2;

                //this reference will most likely be invalidated
                auto exec_func_copy = vp.exec_func;
                auto rect_copy = vp.current_viewport;
                auto& back_vp = imguiHandler.viewports.emplace_back();

                back_vp.context = imguiHandler.InitializeContext();
                back_vp.current_viewport = rect_copy;
                back_vp.current_viewport.offset.x += rect_copy.extent.width;
                //back_vp.current_viewport.offset.y += rect_copy.extent.height;
                back_vp.exec_func = exec_func_copy;
            }

            if(ImGui::Button("bring next to front")){
                ImGui::SetNextWindowFocus();
            }
        //}
        //ImGui::End();
        if(ImGui::Begin("test mover")){
            ImGui::BulletText("pos : %.2f:%.2f", ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
            ImGui::BulletText("size : %.2f:%.2f", ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        }
        ImGui::End();
    };

    auto node_imgui_vp = [&](EWE::ImguiViewport& vp){
        //ImGui::Checkbox("Use work area instead of main area", &use_work_area);

        auto dragdrop_payload = ImGui::GetDragDropPayload();
        if (dragdrop_payload == NULL){
            ImGui::Text("payload null");
        }
        else{
            const int payload_count = (int)dragdrop_payload->DataSize / (int)sizeof(ImGuiID);
            ImGui::Text("payload count : %d : %s", payload_count, reinterpret_cast<const char*>(dragdrop_payload->Data));
        }

        if(ImGui::BeginTabBar("node editors")){
        
            if(ImGui::BeginTabItem("render graph")){
                rgng.RenderNodes();

                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("raster task")){
                //rtng.editor.RenderNodes();
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("record")){
                rng.RenderNodes();
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("inst pckg")){
                ipng.RenderNodes();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    };

    auto assets_imgui_window = [&](EWE::ImguiViewport& vp){
        //EWE::ImguiExtension::Imgui(triangle_raster_task);
        //EWE::ImguiExtension::Imgui(mergeRaster);
        if(ImGui::TreeNode("resources")){
            if(ImGui::TreeNode("buffers")){
                logicalDevice.buffers.mut.lock();
                for(auto res : logicalDevice.buffers){
                    const auto string_addr = std::to_string(reinterpret_cast<std::size_t>(res));
                    std::string res_name = res->name + "##" + string_addr;
                    if(res->name == ""){
                        res_name = string_addr;
                    }
                    if(ImGui::TreeNode(res_name.data())){
                        for(std::size_t i = 0; i < res->creation_trace.size(); i++){
                            if(res->creation_trace[i].source_file().size() == 0){
                                break;
                            }
                            ImGui::Text("%zu:%s", i, res->creation_trace[i].source_file().c_str());
                        }
                        ImGui::TreePop();
                    }
                }
                logicalDevice.buffers.mut.unlock();
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("images")){
                logicalDevice.images.mut.lock();
                for(auto res : logicalDevice.images){
                    const auto string_addr = std::to_string(reinterpret_cast<std::size_t>(res));
                    std::string res_name = res->name + "##" + string_addr;
                    if(res->name == ""){
                        res_name = string_addr;
                    }
                    if(ImGui::TreeNode(res_name.data())){
                        for(std::size_t i = 0; i < res->creation_trace.size(); i++){
                            if(res->creation_trace[i].source_file().size() == 0){
                                break;
                            }
                            ImGui::Text("%zu:%s", i, res->creation_trace[i].source_file().c_str());
                        }
                        ImGui::TreePop();
                    }
                }
                logicalDevice.images.mut.unlock();
                ImGui::TreePop();
            }
            for(auto& q : logicalDevice.queues){
                const std::string pool_name = std::string("queue[") + q.debugName + "] pools";
                if(ImGui::TreeNode(pool_name.c_str())) {
                    q.commandPools.mut.lock();
                    for(auto cmdPool : q.commandPools){
                        const std::string cmdpoolname = std::to_string(reinterpret_cast<std::size_t>(cmdPool));
                        if(ImGui::TreeNode(cmdpoolname.c_str())){
                            ImGui::TreePop();
                        }
                    }
                    q.commandPools.mut.unlock();
                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }

        if(ImGui::TreeNode("shaders")){
            EWE::Global::shaders->Imgui();

            ImGui::TreePop();
        }
        if(ImGui::TreeNode("dii")){
            EWE::Global::diis->Imgui();

            ImGui::TreePop();
        }
        if(ImGui::TreeNode("images")){
            EWE::Global::images->Imgui();

            ImGui::TreePop();
        }
        if(ImGui::TreeNode("samplers")){
            EWE::Global::samplers->Imgui();

            ImGui::TreePop();
        }
        if(ImGui::TreeNode("records")){
            EWE::Global::records->Imgui();
            ImGui::TreePop();
        }

    };

    auto engine_imgui_window = [&](EWE::ImguiViewport& vp){
        engine.Imgui();
        ImGui::BulletText("asset root directory : %s", EWE::Global::records->files.root_directory.string().c_str());
        ImGui::BulletText("mouse pos : %.2f:%.2f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
        static EWE::Inst::Type inst_type = EWE::Inst::Type::BindPipeline;
        Reflect::Enum::Imgui_Combo_Selectable("inst dd", inst_type);
        EWE::DragDropPtr::Source(inst_type);

        if(ImGui::TreeNode("reflect TEST")){
            static constexpr auto hb_i_info = ^^EWE::HeapBlock<EWE::Image>;
            union TestUnion{
                int x;
                float y;
            };
            Reflect::ImguiReflect<^^TestUnion>();

            Reflect::ImguiReflect<hb_i_info>();
            Reflect::ImguiReflect<^^EWE::HeapBlock>();
            ImGui::TreePop();
        }
    };
    {
        auto& vp_back = imguiHandler.viewports[0];
        //imguiHandler.viewports.emplace_back();
        vp_back.exec_func = node_imgui_vp;
        vp_back.current_viewport.offset.y = EWE::Global::window->screenDimensions.height * 0.2f;
        vp_back.current_viewport.extent.height = EWE::Global::window->screenDimensions.height * 0.5f;
    }
    {
        auto& vp_back = imguiHandler.viewports.emplace_back();
        vp_back.exec_func = assets_imgui_window;
        vp_back.context = imguiHandler.InitializeContext();
        vp_back.current_viewport.offset.y = EWE::Global::window->screenDimensions.height * 0.7f;
        vp_back.current_viewport.extent.height = EWE::Global::window->screenDimensions.height * 0.3f;
    }
    {
        auto& vp_back = imguiHandler.viewports.emplace_back();
        vp_back.exec_func = engine_imgui_window;
        vp_back.context = imguiHandler.InitializeContext();
        vp_back.current_viewport.extent.height = EWE::Global::window->screenDimensions.height * 0.2f;
    }

    //imguiHandler.SetSubmissionData(imgui_submission.submitInfo);
    imgui_submission.packaged_tasks.push_back([&](EWE::CommandBuffer& cmdBuf, uint8_t frameIndex) {
#ifdef EWE_IMGUI
        //imguiHandler.BeginCommandBuffer();
        imguiTask.prefix.Execute(cmdBuf, frameIndex);
        imguiHandler.Render(cmdBuf);
        return true;
#endif
        return false;
    }
    );
    EWE::SubmissionTask& attachment_blit_submission = renderGraph.submissions.AddElement(*EWE::Global::logicalDevice, *renderQueue, "attachment blit");

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

    EWE::Sampler& attachmentSampler = EWE::Global::samplers->Get(samplerCreateInfo);


    EWE::PerFlight<EWE::DescriptorImageInfo> world_attachment_descriptor(
        EWE::DescriptorImageInfo{attachmentSampler, triangle_raster_task.renderInfo->full.color_views[0][0], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        EWE::DescriptorImageInfo{attachmentSampler, triangle_raster_task.renderInfo->full.color_views[0][1], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    );
    EWE::PerFlight<EWE::DescriptorImageInfo> imgui_attachment_descriptor(
        EWE::DescriptorImageInfo{attachmentSampler, imguiHandler.renderInfo.full.color_views[0][0], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        EWE::DescriptorImageInfo{attachmentSampler, imguiHandler.renderInfo.full.color_views[0][1], EWE::DescriptorType::Combined, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    );

    std::size_t currentMergeTaskIndex = 0;

    attachment_blit_submission.packaged_tasks.push_back([&](EWE::CommandBuffer& cmdBuf, uint8_t frameIndex) {
        //i dont really know how to bridge tasks in different submisisons yet
        //fully explicit right now

        merge_drawData.textures[0] = &world_attachment_descriptor[frameIndex];
        merge_drawData.textures[1] = &imgui_attachment_descriptor[frameIndex];

        merge_drawData.UpdateBuffer();

        VkRenderingAttachmentInfo presentAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = engine.swapchain.GetCurrentImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {0.f, 0.f, 0.f, 0.f}
        };
        mergeRaster.deferred_vk_render_info->GetRef(frameIndex).colorAttachmentCount = 1;
        mergeRaster.deferred_vk_render_info->GetRef(frameIndex).pColorAttachments = &presentAttachmentInfo;

        mergeTask.workload(cmdBuf, frameIndex);
        renderGraph.presentBridge.Execute(cmdBuf);
        return true;
    });

    

    renderGraph.execution_order = {
        std::vector<EWE::SubmissionTask*>{&imgui_submission, &world_render_submission},
        std::vector<EWE::SubmissionTask*>{&attachment_blit_submission}
    };
    attachment_blit_submission.uses_present_image = true;

    for (auto iter = renderGraph.tasks.begin(); iter != renderGraph.tasks.end(); iter.operator++()) { //why am i manually calling the operator
        //testing hive iterator
        auto& task = *iter;
        EWE::Logger::Print<EWE::Logger::Debug>("Task name - %s\n", task.name.c_str());
    }

    renderGraph.InitializeSemaphores();
    try { //beginning of render loop
        auto timeBegin = std::chrono::high_resolution_clock::now();
        VkDescriptorImageInfo descImg;
        std::chrono::nanoseconds elapsedTime = std::chrono::nanoseconds(0);
        constexpr auto frameDuration = std::chrono::duration<double>(1.0 / 60.0); // seconds per frame



        while (!glfwWindowShouldClose(EWE::Global::window->window)) {
            const auto timeEnd = std::chrono::high_resolution_clock::now();
            elapsedTime += timeEnd - timeBegin;
            timeBegin = timeEnd;
            if (elapsedTime >= frameDuration) {

                glfwPollEvents();

                if (renderGraph.Acquire(EWE::Global::frameIndex)) {
                    auto& swapImage = engine.swapchain.GetCurrentImage();
                    swapImage.data.layout = VK_IMAGE_LAYOUT_UNDEFINED;

                    mouseData.UpdatePosition(EWE::Global::window->window);
                    //nodeGraph.UpdateRender(mouseData, EWE::Global::frameIndex);
                    renderGraph.ChangeResource(mergeTask, present_img_att_index, &swapImage, EWE::Global::frameIndex);
                    renderGraph.presentBridge.UpdateSrcData(&mergeTask.queue, &mergeTask.resources.images[present_img_att_index], EWE::Global::frameIndex);
                    renderGraph.RecreateBarriers(EWE::Global::frameIndex);

                    renderGraph.Execute(EWE::Global::frameIndex);
                    //renderGraph.presentSubmission.Present(EWE::Global::frameIndex);

                    EWE::Global::frameIndex = (EWE::Global::frameIndex + 1) % EWE::max_frames_in_flight;
                    engine.totalFramesSubmitted++;
                }
                else {
                }
                elapsedTime = std::chrono::nanoseconds(0);
            }
        }
    }
    catch (EWE::EWEException& except) {
        logicalDevice.HandleVulkanException(except);
        EWE_ASSERT(false && "caught exception");
    }


#if EWE_DEBUG_BOOL
    EWE::Logger::Print<EWE::Logger::Debug>("returning successfully\n");
#endif

    std::this_thread::sleep_for(std::chrono::seconds(2)); 
    return 0;
}