//example
#include "EWEngine/Assets/DII.h"
#include "EWEngine/Assets/Hash.h"
#include "EWEngine/Systems/Sound_Engine.h"
#include "EightWinds/Data/ForwardArgConstructionHelper.h"
#include "EightWinds/Preprocessor.h"
#include "EightWinds/VulkanHeader.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Global.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"

#include "EightWinds/Pipeline/Layout.h"
#include "EightWinds/Shader.h"
#include "EightWinds/DescriptorImageInfo.h"

#include "EightWinds/Command/Execute.h"
#include "EightWinds/RenderGraph/GPUTask.h"
#include "EightWinds/RenderGraph/RasterPackage.h"
#include "EightWinds/Backend/RenderInfo.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/InstructionPointer.h"

#include "EWEngine/Default/Models.h"

#include "EightWinds/Reflect/Enum.h"
#include "EWEngine/Imgui/ImguiHandler.h"
#include "EWEngine/Reflect/ImguiReflection.h"
#include "EWEngine/Imgui/ImNodes/NodeGraph_Manager.h"
#include "EWEngine/Imgui/DragDrop.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "EWEngine/Data/Timing.h"

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
    EWE::Log::Debug("available extensions --\n");
    for (auto& prop : extProps) {
        EWE::Log::Debug("\t%s\n", prop.extensionName);
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

int main(int argc, char* argv[]) {

    if(argc < 2){
        EWE::Log::Warning("working directory wasn't set via cmd line\n");

    //need to fix htis. its something with my windows debugger
        auto current_working_directory = std::filesystem::current_path();
        if (current_working_directory.parent_path().parent_path().stem() == "build") {
            current_working_directory = current_working_directory.parent_path().parent_path().parent_path();
#if EWE_DEBUG_BOOL
            EWE::Log::Normal("build redacted working dir - %s\n", current_working_directory.string().c_str());
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
        EWE::Log::Normal("current dir (not set from cmd line) - %s\n", std::filesystem::current_path().string().c_str());
#endif
        EWE_Debug_Breakpoint();
    }
    else{
        auto argv1_length = strlen(argv[1]);
        if(argv1_length <= 5){
            EWE::Log::Error("im scared to fuck my system, not allowing working directories less than 5 characters\n");
            return -1;
        }
        const char* second_arg = argv[1];
        std::filesystem::current_path(second_arg);
        EWE::Log::Normal("current dir (set from cmd line) - %s\n", std::filesystem::current_path().string().c_str());
    }


#if EWE_DEBUG_BOOL
    EWE::Log::Debug("starting working dir - %s\n", std::filesystem::current_path().string().c_str());
#endif

#ifdef USING_NVIDIA_AFTERMATH
    EWE::Log::Debug("using nvidia aftermath - %s\n", std::filesystem::current_path().string().c_str());
#endif

#if defined(__SANITIZE_ADDRESS__)
    EWE::Log::Debug("compiled with asan\n");
#endif

    uint32_t extensionCount = 0;
    EWE::EWE_VK(vkEnumerateInstanceExtensionProperties, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    EWE::EWE_VK(vkEnumerateInstanceExtensionProperties, nullptr, &extensionCount, extensions.data());

    EWE::EWEngine engine{ "triangle hello world", std::filesystem::current_path()};

    EWE::Global::InitEngineGlobal(std::filesystem::current_path());

    EWE::NG_Manager ng_manager{};
    ng_manager.DefaultFillGraphs();
    ng_manager.SetOpenGraphFunc();

    //i want to move or condense these funcs somehow. fine for now
    auto imgui_port_info = [&](EWE::ImguiViewport& vp){
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        //ImGui::SetNextWindowPos(viewport->Pos);
        //ImGui::SetNextWindowSize(viewport->Size);
        //if(ImGui::Begin("##", nullptr, main_window_flags)){

            const std::string extension_string = std::string("##") + std::to_string(reinterpret_cast<std::size_t>(&vp));

            ImGui::Separator();

            ImGui::BulletText("context address : %zu", reinterpret_cast<std::size_t>(ImGui::GetCurrentContext()));
            ImGui::BulletText("HoveredWindow : %zu", ImGui::GetCurrentContext()->HoveredWindow);
            ImGui::BulletText("current window : %zu", ImGui::GetCurrentContext()->CurrentWindow);

            ImGui::BulletText("pos : %.2f:%.2f", ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
            ImGui::BulletText("size : %.2f:%.2f", ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
            
            lab::ivec2 difference{vp.current_viewport.offset.x, vp.current_viewport.offset.y};
            if(ImGui::DragInt2((std::string("offset") + extension_string).c_str(), &vp.current_viewport.offset.x, 1.f, 0, EWE::engine->window.screenDimensions.width)){
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
            
            ImGui::DragInt2("extent", reinterpret_cast<int*>(&vp.current_viewport.extent.width), 1.f, 0.f, EWE::engine->window.screenDimensions.width - vp.current_viewport.offset.x);
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

        ng_manager.Render();
    };

    auto assets_imgui_window = [&](EWE::ImguiViewport& vp){

        EWE::Global::assetManager->ApplyGLFWDrops();

        if(ImGui::TreeNode("resources")){
            if(ImGui::TreeNode("buffers")){
                engine.logicalDevice.buffers.mut.lock();
                for(auto res : engine.logicalDevice.buffers){
                    const auto string_addr = std::to_string(reinterpret_cast<std::size_t>(res));
                    std::string res_name = res->name.string() + "##" + string_addr;
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
                engine.logicalDevice.buffers.mut.unlock();
                ImGui::TreePop();
            }
            if(ImGui::TreeNode("images")){
                engine.logicalDevice.images.mut.lock();
                for(auto res : engine.logicalDevice.images){
                    const auto string_addr = std::to_string(reinterpret_cast<std::size_t>(res));
                    std::string res_name = res->name.string() + "##" + string_addr;
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
                engine.logicalDevice.images.mut.unlock();
                ImGui::TreePop();
            }
            for(auto& q : engine.logicalDevice.queues){
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
        EWE::Global::assetManager->Imgui();
    };

    auto engine_imgui_window = [&](EWE::ImguiViewport& vp){
        engine.Imgui();
        ImGui::BulletText("asset root directory : %s", EWE::Global::assetManager->root_directory.string().c_str());
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
        auto dragdrop_payload = ImGui::GetDragDropPayload();
        if (dragdrop_payload == NULL){
            ImGui::Text("payload null");
        }
        else{
            const int payload_count = (int)dragdrop_payload->DataSize / (int)sizeof(ImGuiID);
            ImGui::Text("payload count : %d : %s", payload_count, reinterpret_cast<const char*>(dragdrop_payload->Data));
        }

        if(ImGui::Button("start music")){
            EWE::Global::soundEngine->PlayMusic(EWE::SoundEngine::howling_wind_index, true);
        }
        ImGui::SameLine();
        if(ImGui::Button("stop music")){
            EWE::Global::soundEngine->StopMusic();
        }
    };
    
    bool finished_loading = false;
    auto prepare_rendergraph_func = [&](){
        EWE::LogicalDevice& logicalDevice = engine.logicalDevice;

        //pipeline
        EWE::Shader* triangle_vert = EWE::Global::assetManager->shader.Get("basic.vert.spv");
        EWE::Shader* triangle_frag = EWE::Global::assetManager->shader.Get("basic.frag.spv");

        EWE::AttachmentSetInfo mainSetInfo{};
        {
            mainSetInfo.relative_size = true;
            mainSetInfo.width = 1.f;
            mainSetInfo.height = 1.f;
            mainSetInfo.renderingFlags = 0;
            mainSetInfo.colors.ClearAndResize(1);
            
            auto& color_back = mainSetInfo.colors[0];
            color_back.format = VK_FORMAT_R8G8B8A8_UNORM;
            color_back.clearValue.color.float32[0] = 0.f;
            color_back.clearValue.color.float32[1] = 0.f;
            color_back.clearValue.color.float32[2] = 0.f;
            color_back.clearValue.color.float32[3] = 0.f;
            color_back.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_back.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            auto& depth_temp = mainSetInfo.depth;
            depth_temp.format = VK_FORMAT_D16_UNORM;
            depth_temp.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_temp.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth_temp.clearValue.depthStencil.depth = 0.f;
            depth_temp.clearValue.depthStencil.stencil = 0; //idk what to set this to tbh
        }


        EWE::PipeLayout triangle_layout(logicalDevice, { triangle_vert, triangle_frag });
        //passconfig should be using a full rendergraph setup
        EWE::TaskRasterConfig passConfig;
        EWE::FullRenderInfo& renderInfo = EWE::Global::assetManager->attachment_info.ConstructInto("main ri", logicalDevice, engine.renderQueue, mainSetInfo, EWE::engine->window.screenDimensions.width, EWE::engine->window.screenDimensions.height);
        {
            passConfig.SetDefaults();
            passConfig.attachment_info = renderInfo.full.setInfo;
            passConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
            passConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
            passConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            passConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
            passConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;
        }

        EWE::ObjectRasterData triangle_rasterObj;
        triangle_rasterObj.layout = &triangle_layout;
        triangle_rasterObj.config.SetDefaults();
        triangle_rasterObj.config.cullMode = VK_CULL_MODE_NONE;
        triangle_rasterObj.config.depthClamp = false;
        triangle_rasterObj.config.rasterizerDiscard = false;
        triangle_rasterObj.config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

        EWE::RenderGraph& renderGraph = EWE::Global::assetManager->renderGraph.ConstructInto(
            "triangle-rendergraph", 
            engine.logicalDevice, engine.swapchain, 
            engine.renderQueue, engine.computeQueue, 
            engine.graphics_stc_task, engine.compute_stc_task
        );
        engine.current_renderGraph = &renderGraph;
        EWE::GPUTask& renderTask = EWE::Global::assetManager->gpuTask.ConstructInto("main-render-task", engine.logicalDevice, engine.renderQueue);

        //just generates the buffer so I can pull from it later
        EWE::Basic::Quad(false);

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
        auto& vertex_buffer = EWE::Global::assetManager->buffer.ConstructInto(EWE::Asset::CrossPlatformPathHash("triangle vertex buffer"), sizeof(TriangleVertex) * 3, 1, vmaAllocInfo, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        
        {
            for (auto& str : triangle_vert->variables) {
                if (str.name == "Vertex") {

        #if EWE_DEBUG_BOOL
                    EWE::Log::Debug("size comparison - %zu : %zu\n", str.size, sizeof(TriangleVertex));
        #endif
                }
            }

            vertex_buffer.name = "triangle vertex buffer";
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

        {
            auto& vp_back = EWE::Global::imguiHandler->viewports.emplace_back();
            vp_back.exec_funcs = {engine_imgui_window};
            vp_back.context = EWE::Global::imguiHandler->InitializeContext();
            vp_back.current_viewport.extent.width = EWE::engine->window.screenDimensions.width * 0.8f;
            vp_back.current_viewport.extent.height = EWE::engine->window.screenDimensions.height * 0.2f;
        }
        {
            auto& vp_back = EWE::Global::imguiHandler->viewports[0];
            vp_back.exec_funcs = {node_imgui_vp};
            vp_back.current_viewport.extent.width = EWE::engine->window.screenDimensions.width * 0.8f;
            vp_back.current_viewport.offset.y = EWE::engine->window.screenDimensions.height * 0.2f;
            vp_back.current_viewport.extent.height = EWE::engine->window.screenDimensions.height * 0.8f;
        }
        {
            auto& vp_back = EWE::Global::imguiHandler->viewports.emplace_back();
            vp_back.exec_funcs = {assets_imgui_window};
            vp_back.context = EWE::Global::imguiHandler->InitializeContext();
            vp_back.current_viewport.offset.x = EWE::engine->window.screenDimensions.width * 0.8f;
            vp_back.current_viewport.extent.width = EWE::engine->window.screenDimensions.width * 0.2f;
        }

        EWE::Global::imguiTask->AddToRenderGraph(renderGraph);
        EWE::Global::mergeTask->AddToRenderGraph(renderGraph);

        renderGraph.execution_order = {
            std::vector<EWE::SubmissionTask*>{&EWE::Global::imguiTask->subTask},//, &world_render_submission},
            std::vector<EWE::SubmissionTask*>{&EWE::Global::mergeTask->subTask}
        };

        renderGraph.InitializeSemaphores();

        finished_loading = true;
    };

    auto starting_time = std::chrono::high_resolution_clock::now();
    std::function<bool()> loading_timer_func = [&](){
        auto const current_time = std::chrono::high_resolution_clock::now();
        auto const duration = current_time - starting_time;
        return finished_loading && (duration > std::chrono::seconds(2));
    };

    std::thread async_load_thread{prepare_rendergraph_func};

    engine.leafSystem.RenderLoop(loading_timer_func);

    EWE::RenderGraph& main_render_graph = *EWE::Global::assetManager->renderGraph.Get("triangle-rendergraph");

    try { //beginning of render loop
        auto& render_loop = engine.render_loop_timer;
        render_loop.last_time = std::chrono::high_resolution_clock::now();
        render_loop.duration = std::chrono::duration<double>(1.0 / 60.0);
		render_loop.SetLoopDuration();

        while (!glfwWindowShouldClose(EWE::engine->window.window)) {
            if (render_loop.ReadyForRenderUpdate()) {

                glfwPollEvents();

                if (main_render_graph.Acquire(EWE::engine->frameIndex)) {
                    //mouseData.UpdatePosition(EWE::engine->window.window);
                    main_render_graph.UpdateSwapImage(EWE::engine->frameIndex);
                    main_render_graph.RecreateBarriers(EWE::engine->frameIndex);

                    main_render_graph.Execute(EWE::engine->frameIndex);

                    EWE::engine->frameIndex = (EWE::engine->frameIndex + 1) % EWE::max_frames_in_flight;
                    engine.totalFramesSubmitted++;
                }
                else {
                }
            }
        }
    }
    catch (EWE::EWEException& except) {
        engine.logicalDevice.HandleVulkanException(except);
        EWE_ASSERT(false && "caught exception");
    }


#if EWE_DEBUG_BOOL
    EWE::Log::Debug("returning successfully\n");
#endif

    std::this_thread::sleep_for(std::chrono::seconds(2)); 
    return 0;
}