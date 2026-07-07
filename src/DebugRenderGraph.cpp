#include "EWEngine/Debug/RenderGraph.h"

#include "EWEngine/EWEngine.h"
#include "EWEngine/Global.h"

#include "EightWinds/Command/ParamPool.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

#include "EightWinds/Backend/Exception.h"
#include "EightWinds/Logger.h"

#include "EightWinds/Reflect/Enum.h"

#include <filesystem>
#include <chrono>
#include <ctime>
#include <cstring>

#include <fstream>

#include <unordered_map>

namespace EWE{

    using ParamMap = PerFlight<std::unordered_map<std::size_t, Command::InstructionPackage const*>>;

    struct FileWriter{
        std::filesystem::path name;
        std::ofstream& outFile;
        FileWriter(std::ofstream& _outFile) : outFile{_outFile}{}

        uint8_t tab_depth = 0;

        void Flush(){
            outFile.close();
            outFile.open(name, std::ios::app);
        }

        template<typename T>
        void operator<<(T const& obj){
            for(uint8_t i = 0; i < tab_depth; i++){
                outFile << '\t';
            }
            outFile << obj;
        }

        void endl(){
            outFile << '\n';
        }

        void AddUsage(UsageData<Image> const& usage){
            operator<<("stage : ");
            outFile << usage.stage;
            endl();
            operator<<("access mask : ");
            outFile << usage.accessMask;
            endl();
            operator<<("layout : ");
            outFile << Reflect::Enum::ToString(usage.layout);
            endl();
        }
        void AddResource(Resource<Image> const& res){
            for_each_frame{
                operator<<("image[");
                outFile << (int)frame << "] : ";

                if(res.resource[frame] != nullptr){
                    outFile << res.resource[frame]->name;
                }
                else{
                    outFile << "nullptr";
                }
                endl();
            }
            AddUsage(res.usage);
        }
        void AddUsage(UsageData<Buffer> const& usage){
            operator<<("stage : ");
            outFile << usage.stage;
            endl();
            operator<<("access mask : ");
            outFile << usage.accessMask;
            endl();
        }
        void AddResource(Resource<Buffer> const& res){
            for_each_frame{
                operator<<("buffer[");
                outFile << frame << "] : ";

                if(res.resource[frame] != nullptr){
                    outFile << res.resource[frame]->name;
                }
                else{
                    outFile << "nullptr";
                }
                endl();
            }
            AddUsage(res.usage);
        }
        
    };

    void AddHeader(std::ofstream& outFile){
        outFile << engine->logicalDevice.physicalDevice.name << '\n';
    
        {
            std::string glfw_platform{};
            switch (glfwGetPlatform()) {
                case GLFW_PLATFORM_WIN32:   glfw_platform = "Win32"; break;
                case GLFW_PLATFORM_WAYLAND: glfw_platform = "Wayland"; break;
                case GLFW_PLATFORM_X11:     glfw_platform = "X11"; break;
                default:                    glfw_platform = "Unknown"; break;
            }
            outFile << "glfw platform : " << glfw_platform << '\n';
        }
        //write swapchain data?

        if (Global::sceneManager != nullptr){
            if (Global::sceneManager->currentScenePtr != nullptr) {
                outFile << "current scene : " << Global::sceneManager->currentScenePtr->name << '\n';
            }
            else {
                outFile << "not currently in a scene\n";
            }
        }
    }
    void AddException(std::ofstream& outFile, EWEException const& except){
        outFile << Reflect::Enum::ToString(except.result) << "\n\n";
        outFile << except.msg << "\n\n";
        outFile << except.stacktrace << '\n';
    }

    void AddParamPoolInstructions(FileWriter& file, Command::ParamPool const& pp){
        auto& outFile = file.outFile;
        file << "param pool size : ";
        outFile << pp.instructions.size() << '\n';

        file.tab_depth++;
        for(std::size_t i = 0; i < pp.instructions.size(); i++){
            file << i;
            outFile << "-" << Reflect::Enum::ToString(pp.instructions[i]);
            file.endl();
        }
        file.tab_depth--;
    }

    void AddRenderAttachments(FileWriter& file, AttachmentSetInfo const& att_info){
        auto& outFile = file.outFile;
        file << "relative size : ";
        outFile << att_info.relative_size << '\n';

        file << "dimensions - ";
        outFile << att_info.width << ": " << att_info.height << '\n';

        file << "render flags : ";
        outFile << att_info.renderingFlags << '\n';
        file.endl();

        file << "color att count : ";
        outFile << att_info.colors.Size() << '\n';

        file << "using depth : ";
        outFile << att_info.using_depth << '\n';
    }

    void AddInstPackage(FileWriter& file, Command::InstructionPackage const* pkg, ParamMap& packages);
    void AddRasterPackage(FileWriter& file, RasterPackage const* pkg, ParamMap& packages){
        auto& outFile = file.outFile;
        file << "Raster Package : ";
        outFile << pkg->name << '\n';

        file.tab_depth++;
        AddRenderAttachments(file, pkg->task_config.attachment_info);
        file.tab_depth--;
        
        file << "obj count : ";
        outFile << pkg->objectPackages.size() << '\n';
        file.tab_depth++;
        for(std::size_t i = 0; i < pkg->objectPackages.size(); i++){
            file << i;
            file.endl();
            if(pkg->objectPackages[i] == nullptr){
                file << "nullptr";
                file.endl();
            }
            AddInstPackage(file, reinterpret_cast<Command::InstructionPackage const*>(pkg->objectPackages[i]), packages);
        }
        file.tab_depth--;
    }

    void AddInstPackage(FileWriter& file, Command::InstructionPackage const* pkg, ParamMap& paramMap){
        auto& outFile = file.outFile;

        if(pkg == nullptr){
            file << "InstructionPackage : nullptr";
            file.endl();
            return;
        }

        for_each_frame{
            const std::size_t param_starting_addr = reinterpret_cast<std::size_t>(pkg->paramPool.params[frame].memory);
            paramMap[frame][param_starting_addr] = pkg;
        }

        switch(pkg->type){
            case Command::InstructionPackage::Type::Base:{
                file << "InstructionPackage : ";
                outFile << pkg->name;
                file.endl();
                file.tab_depth++;
                AddParamPoolInstructions(file, pkg->paramPool);
                file.tab_depth--;
               
                break;
            }
            case Command::InstructionPackage::Type::Object:{
                auto const& objPkg = *reinterpret_cast<Command::ObjectPackage const*>(pkg);
                file << "Object Package : ";
                outFile << pkg->name;
                file.endl();
                file << "shaders-";
                file.endl();
                file.tab_depth++;
                for(std::size_t i = 0; i < objPkg.payload.shaders.size(); i++){
                    if(objPkg.payload.shaders[i] != nullptr){
                        file << Reflect::Enum::ToString((ShaderStage::Bits)i);
                        outFile << " : " << objPkg.payload.shaders[i]->name << '\n';
                    }
                }
                file.tab_depth--;
                AddParamPoolInstructions(file, objPkg.paramPool);
                break;
            }
            case Command::InstructionPackage::Type::Raster:{
                AddRasterPackage(file, reinterpret_cast<RasterPackage const*>(pkg), paramMap);
                
                break;
            }
            case Command::InstructionPackage::Type::Compute:{
                file << "Command Package : ";
                outFile << pkg->name;
                file.endl();

                break;
            }
        }
    }

    void AddPackageRecord(FileWriter& file, Command::PackageRecord const* pkgRecord, ParamMap& packages){
        auto& outFile = file.outFile;
        file << "package record : ";
        if(pkgRecord == nullptr){
            outFile << "nullptr\n";
            return;
        }
        else{
            outFile << pkgRecord->name;
            file.endl();

            file << "package count : ";
            outFile << pkgRecord->packages.size() << '\n';
            file.tab_depth++;
            for(auto const& pkg : pkgRecord->packages){
                AddInstPackage(file, pkg, packages);
            }
            file.tab_depth--;
        }
    }

    void AddGPUTask(FileWriter& file, GPUTask const* gpuTask, ParamMap& packages){
        auto& outFile = file.outFile;

        file << "GPUTask : ";
        if(gpuTask == nullptr) {
            outFile << "nullptr";
            file.endl();
            return;
        }
        else{
            outFile << gpuTask->name;
            file.endl();

            {
                file << "resources";
                file.endl();
                
                {
                    file.tab_depth++;
                    file << "resource count : images[";
                    outFile << gpuTask->resources.images.size() << "] buffers[" << gpuTask->resources.buffers.size() << "]\n";

                    for(std::size_t i = 0; i < gpuTask->resources.images.size(); i++){
                        file << "image resource[";
                        outFile << i << "]\n";
                        file.tab_depth++;
                        file.AddResource(gpuTask->resources.images[i]);
                        file.tab_depth--;
                    }
                    for(std::size_t i = 0; i < gpuTask->resources.buffers.size(); i++){
                        file << "buffer resource[";
                        outFile << i << "]\n";
                        file.tab_depth++;
                        file.AddResource(gpuTask->resources.buffers[i]);
                        file.tab_depth--;
                    }

                    file.tab_depth--;
                }
                file.endl();
                file.tab_depth++;
                AddPackageRecord(file, gpuTask->pkgRecord, packages);
                file.tab_depth--;
            }
        }
    }

    void AddSubmissionTask(FileWriter& file, SubmissionTask const* subTask, ParamMap& packages){
        auto& outFile = file.outFile;
        file << "Submission Task : ";
        if(subTask == nullptr){
            outFile << "nullptr";
        }
        else{
            outFile << subTask->name;
            if(subTask->specializedSubmission){
                outFile << " : specialized";
                file.endl();
            }
            else{
                file.endl();
                file.tab_depth++;
                for(auto const& task : subTask->tasks){
                    AddGPUTask(file, task, packages);
                }
                file.tab_depth--;
            }
        }
            
    }

    void AddRenderGraph(FileWriter& file, RenderGraph const& renderGraph, uint8_t frameIndex, ParamMap& packages){
        auto& outFile = file.outFile;

        outFile << "Render Graph : " << renderGraph.name.string() << '\n';
        outFile << "Execution order:\n";
        
        file.tab_depth++;
        for(std::size_t i = 0; i < renderGraph.execution_order.size(); i++){
            file << "execution stage : ";
            outFile << i << '\n';
            file.tab_depth++;
            for(auto const& ind_sub : renderGraph.execution_order[i]) {
                AddSubmissionTask(file, ind_sub, packages);
            }
            file.tab_depth--;
        }
        file.tab_depth--;
    }

    template <typename T>
    void PrintMemberAndData(FileWriter& file, T const& pack){
        auto& outFile = file.outFile;
        static constexpr auto type_mems = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));
        template for(constexpr auto type_mem : type_mems){
            constexpr std::string_view name = std::meta::identifier_of(type_mem);
            file << name;

            auto const& value = pack.[:type_mem:];
            using MemberType = std::remove_cvref_t<decltype(value)>;
            if constexpr(std::is_same_v<MemberType, VkBuffer>){
                outFile << " : " << reinterpret_cast<std::size_t>(value) << " : ";
                if(value == VK_NULL_HANDLE){
                    outFile << "VK_NULL_HANDLE\n";
                }
                else{
                    Buffer const* reverted_buffer = engine->logicalDevice.RevertVkBuffer(value);
                    if(reverted_buffer == nullptr){
                        outFile << "nullptr (failed to revert)\n";
                    }
                    else{
                        outFile << reverted_buffer->name << '\n';
                    }
                }
            }
            else if constexpr(std::is_pointer_v<T>){
                outFile << " : " << reinterpret_cast<std::size_t>(value) << '\n';
            }
            else if constexpr(std::meta::is_class_type(^^MemberType)){
                file.endl();
                file.tab_depth++;
                PrintMemberAndData(file, value);
                file.tab_depth--;
            }
            else if constexpr(std::is_same_v<MemberType, uint8_t>){
                outFile << " : " << (int)value << '\n';
            }
            else{
                outFile << " : " << value << '\n';
            }
        }
    }

    void AddParamData(FileWriter& file, Command::ParamPool const& pp, uint8_t current_frame, ParamMap const& paramMap);
    void AddInstruction(FileWriter& file, InstructionPointerAdjuster const& ip, Inst::Type iType, uint8_t frameIndex, ParamMap const& paramMap){
        auto& outFile = file.outFile;

        if(iType == Inst::Ext_Pool){
            //file.Flush();

            auto const* poolPack = ip.CastTo<ParamPack<Inst::Ext_Pool>>();
            Command::ParamPool const& child_pool = *poolPack->GetCRef(frameIndex).pool;
            AddParamData(file, child_pool, frameIndex, paramMap);
        }
        else if (iType == Inst::BeginRender){
            auto const* pack = ip.CastTo<ParamPack<Inst::BeginRender>>();
            auto const& renderInfo = reinterpret_cast<VkRenderingInfo const&>(pack->GetCRef(frameIndex));
            PrintMemberAndData(file, renderInfo);
        }
#if 1//EWE_DEBUG_BOOL
        else if (iType == Inst::Push){
            auto const* pack = ip.CastTo<ParamPack<Inst::Push>>();
            auto const& push = pack->GetCRef(frameIndex);
            for(std::size_t i = 0; i < push.buffer_count; i++){
                file << "buffer[";
                outFile << i << "] : ";
                auto const& da = push.GetDeviceAddress(i);
                if(da == null_buffer){
                    outFile << "null buffer\n";
                }
                else{
                    auto const* binded_buffer = engine->logicalDevice.RevertDA(da);
                    if(binded_buffer == nullptr){
                        outFile << da << " : " << "nullptr (couldnt be reversed)\n";
                    }
                    else{
                        outFile << da << " : " << binded_buffer->name << '\n';
                    }
                }
            }
            for(std::size_t i = 0; i < push.texture_count; i++){
                auto const& ti = push.GetTextureIndex(i);
                if(ti == null_texture){
                    outFile << "null texture\n";
                }
                else{
                    auto const* binded_tex = engine->logicalDevice.RevertTI(ti);
                    if(binded_tex != nullptr){
                        outFile << ti << " : " << "nullptr (couldnt be reversed)\n";
                    }
                    else{
                        outFile << ti << " : " << binded_tex->view.image.name << '\n';
                    }
                }
            }
        }
#endif
        else{
            static constexpr auto type_mems = std::define_static_array(std::meta::enumerators_of(^^Inst::Type));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
            template for(constexpr auto enum_mem : type_mems){
                if ([:enum_mem:] == iType) {
                    if constexpr(std::meta::is_complete_type(^^ParamPack<([:enum_mem:])>)) {
                        auto const* instPtr = ip.CastTo<ParamPack<([:enum_mem:])>>();
                        PrintMemberAndData(file, instPtr->GetCRef(frameIndex));
                    }
                }
            }
#pragma GCC diagnostic pop
        }
    }

    void AddParamData(FileWriter& file, Command::ParamPool const& pp, uint8_t current_frame, ParamMap const& paramMap){
        auto& outFile = file.outFile;
        const std::size_t param_starting_addr = reinterpret_cast<std::size_t>(pp.params[current_frame].memory);
        auto found = paramMap[current_frame].find(param_starting_addr);
        if(found == paramMap[current_frame].end()){
            file << "failed to find pp's owner name\n";
        }
        else{
            file << found->second->name;
            outFile << " : " << Reflect::Enum::ToString(found->second->type) << '\n';
        }
        file.tab_depth++;

        std::size_t param_offset = 0;

        for(std::size_t i = 0; i < pp.instructions.size(); i++){
            auto const& inst = pp.instructions[i];
            const bool has_param_data = Inst::GetParamSize(inst) > 0;
            file << i;
            outFile << "." << Reflect::Enum::ToString(inst) << '\n';
            if(has_param_data){
                file.tab_depth++;
                //template for each member
                AddInstruction(file, pp.param_data[param_offset], inst, current_frame, paramMap);
                file.tab_depth--;
                param_offset++;
            }
        }
        file.tab_depth--;
    }

    void CrackIntoParamData(FileWriter& file, RenderGraph const& renderGraph, uint8_t current_frame, ParamMap& paramMap){
        auto& outFile = file.outFile;
        file.tab_depth++;
        for(std::size_t sub_group_index = 0; sub_group_index < renderGraph.execution_order.size(); sub_group_index++){
            auto const& sub_group = renderGraph.execution_order[sub_group_index];
            file << "sub group[";
            outFile << sub_group_index;
            outFile << "]\n";
            
            file.tab_depth++;
            for(std::size_t ind_sub_index = 0; ind_sub_index < sub_group.size(); ind_sub_index++){
                auto const& ind_sub = sub_group[ind_sub_index];
                file << "sub task[";
                outFile << ind_sub->name << "]\n";
                if(!ind_sub->specializedSubmission){


                    file.tab_depth++;
                    for(auto const& gpuTask : ind_sub->tasks){
                        file << "GPUTask[";
                        outFile << gpuTask->name << "]\n";
                        auto const& pkgRecord = gpuTask->pkgRecord;

                        //now we actually getinto the pp
                        //file.tab_depth++;
                        for(auto& pkg : pkgRecord->packages){
                            AddParamData(file, pkg->paramPool, current_frame, paramMap);
                        }
                        //file.tab_depth--;
                        
                    }
                    file.tab_depth--;
                }
                else{
                    file.tab_depth++;
                    file << "specialized\n";
                    file.tab_depth--;
                }
            }
            file.tab_depth--;
        }
        file.tab_depth--;
    }

    void DumpBuffers(FileWriter& file){
        auto& outFile = file.outFile;

        file << "dumped buffers-\n";
        file.tab_depth++;

        for(auto& buf_addr : engine->logicalDevice.buffers.resources){
            Buffer const& buf = buf_addr.CastToRef<Buffer>();
            file << buf.name;
            outFile << " : ";
            if(buf.usageFlags == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT){
                if(buf.deviceAddress == null_buffer){
                    outFile << "null_buffer addr\n";
                }
                else{
                    outFile << static_cast<std::size_t>(buf.deviceAddress) << '\n';
                }
            }
            else{

                static constexpr auto usage_data = Reflect::Enum::enum_data<VkBufferUsageFlagBits>;

                bool already_have_one = false;
                for(std::size_t i = 0; i < usage_data.size(); i++){
                    if(buf.usageFlags & usage_data[i].value){
                        if(already_have_one){
                            outFile << " | ";
                        }
                        already_have_one = true;
                        outFile << usage_data[i].name;
                    }
                }

                outFile << '\n';
            }
        }
        file.tab_depth--;
    }
    void DumpTextures(FileWriter& file){
        auto& outFile = file.outFile;

        file << "dumped textures-\n";
        file.tab_depth++;

        for(auto& kvp : engine->logicalDevice.bindlessDescriptor.tracker){
            DescriptorImageInfo const& dii = *kvp.key;

            if(dii.name == ""){
                file << dii.view.image.name;
            }
            else{
                file << dii.name;
            }

            if(kvp.value == null_texture){
                outFile << " : null_texture\n";
            }
            else{
                outFile << " : " << kvp.value << '\n';
            }
        }
        file.tab_depth--;
    }

    void Debug_RenderGraph_DEVICE_LOST(RenderGraph const& renderGraph, EWEException const& except){

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::localtime(&t);

        char time_buf[64];
        memset(time_buf, 0, 64);
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d_%H-%M-%S", tm);

        std::filesystem::path file_path = "Error" / std::filesystem::path(time_buf);
        file_path.replace_extension(".log");

        if(!std::filesystem::exists(std::filesystem::current_path() / "Error")){
            std::filesystem::create_directory(std::filesystem::current_path() / "Error");
        }
        std::ofstream outFile{file_path, std::ios::binary};
        if(!outFile.is_open()){

            outFile.open(file_path, std::ios::binary);

            if(!outFile.is_open()){
                Log::Error("failed to create error file\n");
                return;
            }
        }
        Log::Debug("dumping render graph to : %s / %s\n", std::filesystem::current_path().string().c_str(), file_path.string().c_str());

        AddHeader(outFile);
        outFile << '\n';
        AddException(outFile, except);
        outFile << '\n';

        ParamMap packageMap{};
        FileWriter file{outFile};
        file.name = file_path;

        uint8_t previous_frame = engine->frameIndex;
        if(previous_frame == 0){
            previous_frame = max_frames_in_flight - 1;
        }
        else{
            previous_frame--;
        }

        DumpBuffers(file);
        DumpTextures(file);

        AddRenderGraph(file, renderGraph, previous_frame, packageMap);

        file.endl();
        file << "Param Data - previous frame[";
        outFile << (int)previous_frame << "] : total frames[" << (int)engine->totalFramesSubmitted << "]\n";

        file.Flush();

        CrackIntoParamData(file, renderGraph, previous_frame, packageMap);

        file.Flush();

        file.endl();
        file << "Param Data - current frame[";
        outFile << (int)engine->frameIndex << "]\n";
        CrackIntoParamData(file, renderGraph, engine->frameIndex, packageMap);

        outFile.close();
    }
} //namespace EWE