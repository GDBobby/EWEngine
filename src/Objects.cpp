#include "EWEngine/Imgui/Objects.h"

#include "EWEngine/Global.h"
#include "EWEngine/Imgui/Params.h"

#ifdef EWE_IMGUI
#include "imgui.h"

#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"
#include "EightWinds/RenderGraph/RenderGraph.h"
#include "EightWinds/RenderGraph/RasterTask.h"
#include "EightWinds/RenderGraph/GPUTask.h"
#include "EightWinds/RenderGraph/Resources.h"
#include "EightWinds/GlobalPushConstant.h"
#include "EightWinds/Pipeline/TaskRasterConfig.h"
#include "EightWinds/Pipeline/Graphics.h"

#include <string>

#include "EightWinds/Reflect/Enum.h"


void imgui_bool_check(std::string_view name, VkBool32& vk_bool) {
    bool temp_bool = vk_bool;
    ImGui::Checkbox(name.data(), &temp_bool);
    vk_bool = temp_bool;
}

void imgui_color_components(std::string_view name, VkColorComponentFlags& components) {
    ImGui::Text(name.data());
    ImGui::SameLine();
    bool channels[4];
    channels[0] = components & VK_COLOR_COMPONENT_R_BIT;
    channels[1] = components & VK_COLOR_COMPONENT_G_BIT;
    channels[2] = components & VK_COLOR_COMPONENT_B_BIT;
    channels[3] = components & VK_COLOR_COMPONENT_A_BIT;
    ImGui::Checkbox("r", &channels[0]);
    ImGui::SameLine();
    ImGui::Checkbox("g", &channels[1]);
    ImGui::SameLine();
    ImGui::Checkbox("b", &channels[2]);
    ImGui::SameLine();
    ImGui::Checkbox("a", &channels[3]);

    components = (channels[0] * VK_COLOR_COMPONENT_R_BIT)
        + (channels[1] * VK_COLOR_COMPONENT_G_BIT)
        + (channels[2] * VK_COLOR_COMPONENT_B_BIT)
        + (channels[3] * VK_COLOR_COMPONENT_A_BIT)
        ;

}

namespace EWE{
    
#define FUNC_ENTRY(Type) template<> void ImguiExtension::Imgui(Type& obj)
    

    template<> void ImguiExtension::Imgui(Image& obj){
        ImGui::Text("name : %s", obj.name.c_str());
        if(obj.owningQueue != nullptr){
            ImGui::Text("owning Queue : %s", Reflect::Enum::ToString(Global::stcManager->GetQueueType(*obj.owningQueue)).data());
        }
        else{
            ImGui::Text("owning queue : nullptr");
            }


        ImGui::Text("extent : width[%u] height[%u] depth[%u]", obj.data.extent.width, obj.data.extent.height, obj.data.extent.depth);
        //ImGui::Text("Array Layers[%u] - Mip Levels[%u]", obj.data.arrayLayers, obj.data.mipLevels);
        ImGui::DragInt("array layers", reinterpret_cast<int*>(&obj.data.arrayLayers), 1.f, 0, 256);
        ImGui::DragInt("mip levels", reinterpret_cast<int*>(&obj.data.mipLevels), 1.f, 0, 256);
        Reflect::Enum::Imgui_Combo_Selectable("format", obj.data.format);
        ImGui::Text("layout : %s", Reflect::Enum::ToString(obj.data.layout).data());
        
        ImGui::Text("create flags : %d", obj.data.createFlags);
        //ImGui::Text("usage : %d", obj.data.usage);
        if(ImGui::BeginListBox("create flags")){
            auto temp_buffer = static_cast<VkImageCreateFlagBits>(obj.data.createFlags);
            Reflect::Enum::Imgui_ForEach_Check(temp_buffer);
            obj.data.createFlags = temp_buffer;
            ImGui::EndListBox();
        }
        if(ImGui::BeginListBox("usage")){
            VkImageUsageFlagBits temp_buffer = static_cast<VkImageUsageFlagBits>(obj.data.usage);
            Reflect::Enum::Imgui_ForEach_Check(temp_buffer);
            obj.data.usage = temp_buffer;
            ImGui::EndListBox();
        }

        Reflect::Enum::Imgui_Combo_Selectable("type", obj.data.type);
        Reflect::Enum::Imgui_Combo_Selectable("samples", obj.data.samples);
        Reflect::Enum::Imgui_Combo_Selectable("tiling", obj.data.tiling);
    }
    template<> void ImguiExtension::Imgui(ImageView& obj){
        ImGui::Text("name : %s", obj.name.c_str());
        if(ImGui::TreeNode("image")){
            ImguiExtension::Imgui(obj.image);
            ImGui::TreePop();
        }

        //this assumes aspect flags won't be combined, which I've never done
        VkImageAspectFlagBits aspect_flag_bits = static_cast<VkImageAspectFlagBits>(obj.subresource.aspectMask);
        ImGui::Text("aspect mask : %s", Reflect::Enum::ToString(aspect_flag_bits).data());
        ImGui::Text("base array layer[%u] : layer count[%u]", obj.subresource.baseArrayLayer, obj.subresource.layerCount);
        ImGui::Text("base mip level[%u] : level count[%u]", obj.subresource.baseMipLevel, obj.subresource.levelCount);
    }

    template<> void ImguiExtension::Imgui(RenderGraph& obj){
        for (auto& task : obj.tasks) {
            if(ImGui::TreeNode("Task name : %s", task.name.c_str())){
                ImguiExtension::Imgui(task);
                ImGui::TreePop();
            }
        }
        for (auto& sub : obj.submissions) {
            ImGui::Text("submission name : %s", sub.name.c_str());
        }
        for (std::size_t i = 0; i < obj.execution_order.size(); i++) {
            const std::string sub_group_name = std::string("submission group[") + std::to_string(i) + ']';
            if (ImGui::TreeNode(sub_group_name.c_str())) {
                for (auto* sub : obj.execution_order[i]) {
                    ImGui::Text("sub : %s", sub->name.c_str());
                }
                ImGui::TreePop();
            }
        }

        ImGui::Text("present result : %s", Reflect::Enum::ToString(obj.presentResult).data());
	}
    
    
    
	FUNC_ENTRY(RasterTask){
        int raster_task_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
		ImGui::PushID(raster_task_id);
		ImGui::Text("Raster Task : %s", obj.name.c_str());
        if (ImGui::TreeNode("config")) {
            ImguiExtension::Imgui(obj.config);
            ImGui::TreePop();
        }
        ImGui::Checkbox("owns attachment lifetime", &obj.ownsAttachmentLifetime);

        for (auto& def_pipe : obj.deferred_pipelines) {
            auto* graphics_pipe = reinterpret_cast<GraphicsPipeline*>(def_pipe.pipeline);
        }
        
        ImGui::PopID();
    }
    
    FUNC_ENTRY(TaskRasterConfig){
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        ImGui::DragInt("viewport count", reinterpret_cast<int*>(&obj.viewportCount), 0, 8); // i have some arbitrary limit somewhere i need to find
        ImGui::DragInt("scissor count", reinterpret_cast<int*>(&obj.scissorCount), 0, 8);
        //imgui_enum("rast samples", obj.rastSamples, 0, 128);
        Reflect::Enum::Imgui_Combo_Selectable("rast samples", obj.rastSamples);

        ImGui::Checkbox("sample shading", &obj.enable_sampleShading);
        if (obj.minSampleShading) {
            ImGui::DragFloat("min sample shading", &obj.minSampleShading, 0.f, 100.f);
        }
        ImGui::Checkbox("alpha to coverage", &obj.alphaToCoverageEnable);
        ImGui::Checkbox("alpha to one enable", &obj.alphaToOneEnable);

        for (auto& dynS : obj.dynamicState) {
            ImGui::Text("dyn state - %s", Reflect::Enum::ToString(dynS).data());
        }

        ImguiExtension::Imgui(obj.attachment_set_info);

        ImGui::PopID();
    }


    FUNC_ENTRY(ObjectRasterConfig) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        ImGui::Checkbox("depth clamp", &obj.depthClamp);
        ImGui::Checkbox("rasterizer discard", &obj.rasterizerDiscard);
        //imgui_enum("polygon mode", obj.polygonMode, 0, 3);
        Reflect::Enum::Imgui_Combo_Selectable("polygon mode", obj.polygonMode);

        ImGui::RadioButton("none", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_NONE); ImGui::SameLine();
        ImGui::RadioButton("front", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_FRONT_BIT); ImGui::SameLine();
        ImGui::RadioButton("back", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_BACK_BIT); ImGui::SameLine();
        ImGui::RadioButton("front and back", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_FRONT_AND_BACK);

        //imgui_enum("front face", obj.frontFace, 0, 2);
        Reflect::Enum::Imgui_Combo_Selectable("front face", obj.frontFace);

        ImguiExtension::Imgui(obj.depthBias);

        //imgui_enum("topology", obj.topology, 0, 12);
        Reflect::Enum::Imgui_Combo_Selectable("topology", obj.topology);
        ImGui::Checkbox("primitive restart", &obj.primitiveRestart);

        ImguiExtension::Imgui(obj.blendAttachment);
            
        ImGui::PopID();
    }
    FUNC_ENTRY(ObjectRasterData) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        ImguiExtension::Imgui(*obj.layout);
        ImguiExtension::Imgui(obj.config);
        ImGui::PopID();
    }
    FUNC_ENTRY(GlobalPushConstant_Abstract) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        for (std::size_t i = 0; i < GlobalPushConstant_Raw::buffer_count; i++) {
            if (obj.buffers[i]) {
                const std::string temp_name{ std::string("[") + std::to_string(i) + ']' + obj.buffers[i]->name };
                ImGui::Text(temp_name.c_str());
            }
        }
        for (std::size_t i = 0; i < GlobalPushConstant_Raw::texture_count; i++) {
            if (obj.textures[i]) {
                const std::string temp_name{ std::string("[") + std::to_string(i) + ']' + obj.textures[i]->view.image.name };
                ImGui::Text(temp_name.c_str());
            }
        }
        ImGui::PopID();
    }

    template<> void ImguiExtension::Imgui(Command::Executor& obj) {
        
        std::size_t current_memory_offset = 0;
        for(auto& inst : obj.record.records){
            

            current_memory_offset += Instruction::GetParamSize(inst.type);
        }
        /*
        for (std::size_t i = 0; i < obj.records.size(); i++) {
            ImGui::Text("%zu\t:%s", i, Reflect::Enum::ToString(obj.records[i].type).data());
        }
        */
    }

    FUNC_ENTRY(GPUTask) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        ImGui::Text("gpu task - %s", obj.name.c_str());
        ImGui::Text("queue index : %d", obj.queue.FamilyIndex());

        if(ImGui::TreeNode("resources")){
            ImguiExtension::Imgui(obj.resources);
            ImGui::TreePop();
        }

        if(obj.commandExecutor.has_value()){
            if(ImGui::TreeNode("executor")){
                ImguiExtension::Imgui(obj.commandExecutor.value());
                ImGui::TreePop();
            }
        }
        else{
            ImGui::Text("executor has no value");
        }

        ImGui::PopID();
    }
    FUNC_ENTRY(TaskResourceUsage) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        ImGui::Text("image count[%zu] : buffer count[%zu]", obj.images.size(), obj.buffers.size());
        for (auto& img : obj.images) {
            ImguiExtension::Imgui(img);
        }
        for (auto& buf : obj.buffers) {
            ImguiExtension::Imgui(buf);
        }

        ImGui::PopID();
    }
    //FUNC_ENTRY(TaskAffix); //no valuable info i think, rebuilt every frame
    FUNC_ENTRY(AttachmentSetInfo) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        ImGui::DragInt("width", reinterpret_cast<int*>(&obj.width), 0, 16536);
        ImGui::DragInt("height", reinterpret_cast<int*>(&obj.height), 0, 16536);
        //come back to this
        //Reflect::Enum::Imgui_Combo_Selectable("rendering flags", obj.renderingFlags);

        int col_count = obj.colors.size();
        if (ImGui::DragInt("color count", &col_count, 0, 16)) {
            obj.colors.resize(col_count);
        }
        for (auto& col : obj.colors) {
            //imgui_enum("format", col.format, 0, 256);
            Reflect::Enum::Imgui_Combo_Selectable("format", col.format);
        }
        ImguiExtension::Imgui(obj.depth);

        ImGui::PopID();
    }
    FUNC_ENTRY(VkPipelineDepthStencilStateCreateInfo) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        imgui_bool_check("depth test enable", obj.depthTestEnable);
        imgui_bool_check("depth write enable", obj.depthWriteEnable);
        Reflect::Enum::Imgui_Combo_Selectable("depth compare op", obj.depthCompareOp);
        imgui_bool_check("depth bounds test enable", obj.depthBoundsTestEnable);
        imgui_bool_check("stencil test enable", obj.stencilTestEnable);

        ImguiExtension::Imgui(obj.front); //vkstencilopstate
        ImguiExtension::Imgui(obj.back); //vkstencilopstate

        ImGui::DragFloat("min depth bounds", &obj.minDepthBounds, -1000.f, 1000.f);
        ImGui::DragFloat("max depth bounds", &obj.maxDepthBounds, -1000.f, 1000.f);
        ImGui::PopID();
    }
    FUNC_ENTRY(DepthBias) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        ImGui::Checkbox("enable", &obj.enable);
        ImGui::DragFloat("constant factor", &obj.constantFactor, -1000.f, 1000.f);
        ImGui::DragFloat("clamp", &obj.clamp, -1000.f, 1000.f);
        ImGui::DragFloat("slope factor", &obj.slopeFactor, -1000.f, 1000.f);

        ImGui::PopID();
    }
    FUNC_ENTRY(VkPipelineColorBlendAttachmentState) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        imgui_bool_check("blend enable", obj.blendEnable);
        //imgui_enum("src color blend factor", obj.srcColorBlendFactor, 0, 20);
        //imgui_enum("dst color blend factor", obj.dstColorBlendFactor, 0, 20);
        //imgui_enum("color blend op", obj.colorBlendOp, 0, 5);

        //imgui_enum("src alpha blend factor", obj.srcAlphaBlendFactor, 0, 20);
        //imgui_enum("dst alpha blend factor", obj.dstAlphaBlendFactor, 0, 20);
        //imgui_enum("alpha blend op", obj.alphaBlendOp, 0, 5);
        Reflect::Enum::Imgui_Combo_Selectable("src color blend factor", obj.srcColorBlendFactor);
        Reflect::Enum::Imgui_Combo_Selectable("dst color blend factor", obj.dstColorBlendFactor);
        Reflect::Enum::Imgui_Combo_Selectable("color blend op", obj.colorBlendOp);

        Reflect::Enum::Imgui_Combo_Selectable("src alpha blend factor", obj.srcAlphaBlendFactor);
        Reflect::Enum::Imgui_Combo_Selectable("dst alpha blend factor", obj.dstAlphaBlendFactor);
        Reflect::Enum::Imgui_Combo_Selectable("alpha blend op", obj.alphaBlendOp);

        imgui_color_components("color write mask", obj.colorWriteMask);

        ImGui::PopID();
    }
    template<> void ImguiExtension::Imgui(Resource<Image>& obj) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        ImGui::Text(obj.resource[0]->name.c_str());
        if (obj.resource[1]) {
            ImGui::Text(obj.resource[1]->name.c_str());
        }

        ImguiExtension::Imgui(obj.usage);

        ImGui::PopID();
    }
    template<> void ImguiExtension::Imgui(Resource<Buffer>& obj) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        ImGui::Text(obj.resource[0]->name.c_str());
        if (obj.resource[1]) {
            ImGui::Text(obj.resource[1]->name.c_str());
        }

        ImguiExtension::Imgui(obj.usage);

        ImGui::PopID();
    }

    template<> void ImguiExtension::Imgui(UsageData<Image>& obj) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        //Reflect::Enum::Imgui_Combo_Selectable("stage", obj.stage);
        //Reflect::Enum::Imgui_Combo_Selectable("access mask", obj.accessMask);

        //these aren't enums
        ImGui::DragInt("stage", reinterpret_cast<int*>(&obj.stage), 1, 0, 65536);
        ImGui::DragInt("access mask", reinterpret_cast<int*>(&obj.accessMask), 1, 0, 65536);


        //imgui_enum("layout", obj.layout, 0, 9);
        Reflect::Enum::Imgui_Combo_Selectable("layout", obj.layout);

        ImGui::PopID();
    }
    template<> void ImguiExtension::Imgui(UsageData<Buffer>& obj) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        ImGui::DragInt("stage", reinterpret_cast<int*>(&obj.stage), 0, 65536);
        ImGui::DragInt("access mask", reinterpret_cast<int*>(&obj.accessMask), 0, 65536);

        ImGui::PopID();
    }

    FUNC_ENTRY(PipeLayout) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        for (std::size_t i = 0; i < Shader::Stage::COUNT; i++) {
            if (obj.shaders[i]) {
                Shader::Stage temp_val{ static_cast<Shader::Stage::Bits>(i) };
                ImGui::Text(Reflect::Enum::ToString(temp_val.value).data());
                ImGui::SameLine();
                ImGui::Text(" : %s", obj.shaders[i]->name.c_str());
                ImguiExtension::Imgui(*obj.shaders[i]);
            }

            ImGui::Text("Pipeline Type : %s", Reflect::Enum::ToString(obj.pipelineType).data());
            ImGui::Text("Bind Point : %s", Reflect::Enum::ToString(obj.bindPoint).data());
        }
        ImGui::PopID();
    }
    FUNC_ENTRY(Shader) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);



        ImGui::PopID();
    }

    FUNC_ENTRY(AttachmentInfo) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        //imgui_enum("format", obj.format, 0, 256);
        Reflect::Enum::Imgui_Combo_Selectable("format", obj.format);
        ImguiExtension::Imgui(obj.loadOp);
        ImguiExtension::Imgui(obj.storeOp);
        ImGui::DragFloat4("clear value", reinterpret_cast<float*>(obj.clearValue.color.float32), 0.f, 1.f);

        ImGui::PopID();
    }

    template<> void ImguiExtension::Imgui(VkAttachmentLoadOp& obj) {
        static constexpr std::array<const char*, 4> s_vs{
            "VK_ATTACHMENT_LOAD_OP_LOAD",
            "VK_ATTACHMENT_LOAD_OP_CLEAR",
            "VK_ATTACHMENT_LOAD_OP_DONT_CARE",
            "VK_ATTACHMENT_LOAD_OP_NONE"
        };

        int temp = obj;
        if (temp > 3) {
            temp = 3;
        }
        ImGui::SliderInt("load op", reinterpret_cast<int*>(&obj), 0, s_vs.size(), s_vs[temp]);
    }

    template<> void ImguiExtension::Imgui(VkAttachmentStoreOp& obj) {
        static constexpr std::array<const char*, 4> s_vs{
            "VK_ATTACHMENT_STORE_OP_STORE",
            "VK_ATTACHMENT_STORE_OP_DONT_CARE",
            "VK_ATTACHMENT_STORE_OP_NONE"
        };

        int temp = obj;
        if (temp > 2) {
            temp = 2;
        }
        ImGui::SliderInt("store op", reinterpret_cast<int*>(&obj), 0, s_vs.size(), s_vs[temp]);
    }

    template<> void ImguiExtension::Imgui(VkSamplerCreateInfo& obj) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        
        Reflect::Enum::Imgui_Combo_Selectable("mag filter", obj.magFilter);
        Reflect::Enum::Imgui_Combo_Selectable("min filter", obj.minFilter);
        Reflect::Enum::Imgui_Combo_Selectable("mipmap mode", obj.mipmapMode);
        Reflect::Enum::Imgui_Combo_Selectable("address mode U", obj.addressModeU);
        Reflect::Enum::Imgui_Combo_Selectable("address mode V", obj.addressModeV);
        Reflect::Enum::Imgui_Combo_Selectable("address mode W", obj.addressModeW);
        ImGui::DragFloat("mip lod bias", &obj.mipLodBias, 1.f, obj.minLod, obj.maxLod);
        bool vk_bool = obj.anisotropyEnable;
        ImGui::Checkbox("enable anisotropy", &vk_bool);
        obj.anisotropyEnable = vk_bool;
        if(obj.anisotropyEnable){
            ImGui::DragFloat("max anisotropy", &obj.maxAnisotropy, 1.f, 0.f, 100.f);
        }
        vk_bool = obj.compareEnable;
        ImGui::Checkbox("enable compare", &vk_bool);
        obj.compareEnable = vk_bool;
        if(obj.compareEnable){
            Reflect::Enum::Imgui_Combo_Selectable("compare op", obj.compareOp);
        }
        ImGui::DragFloat("min lod", &obj.minLod, 1.f, 0.f, obj.maxLod);
        ImGui::DragFloat("max lod", &obj.maxLod, 1.f, obj.minLod, 100.f);
        Reflect::Enum::Imgui_Combo_Selectable("border color", obj.borderColor);
        vk_bool = obj.unnormalizedCoordinates;
        ImGui::Checkbox("unnormalized coordinates", &vk_bool);
        obj.unnormalizedCoordinates = vk_bool;
    


        ImGui::PopID();
    }

    template<> void ImguiExtension::Imgui(VkStencilOpState& obj) {
        //imgui_enum("fail op", obj.failOp, 0, 8);
        //imgui_enum("pass op", obj.passOp, 0, 8);
        //imgui_enum("depth fail op", obj.depthFailOp, 0, 8);
        //imgui_enum("compare op", obj.compareOp, 0, 8);

        Reflect::Enum::Imgui_Combo_Selectable("fail op", obj.failOp);
        Reflect::Enum::Imgui_Combo_Selectable("pass op", obj.passOp);
        Reflect::Enum::Imgui_Combo_Selectable("depth fail op", obj.depthFailOp);
        Reflect::Enum::Imgui_Combo_Selectable("compare op", obj.compareOp);
        ImGui::DragInt("compare mask", reinterpret_cast<int*>(&obj.compareMask), 0, INT32_MAX);
        ImGui::DragInt("write mask", reinterpret_cast<int*>(&obj.compareMask), 0, INT32_MAX);
        ImGui::DragInt("reference", reinterpret_cast<int*>(&obj.reference), 0, INT32_MAX);

    }

    template<> void ImguiExtension::Imgui(Command::Record& obj) {
        for (std::size_t i = 0; i < obj.records.size(); i++) {
            ImGui::Text("%zu\t:%s", i, Reflect::Enum::ToString(obj.records[i].type).data());
        }
    }


    void ImguiExpandInstruction(void* mem_addr, Instruction::Type itype){
        switch (itype) {
            case Instruction::Type::BindPipeline: ImguiReflectParamStruct(reinterpret_cast<ParamPack::Pipeline*>(mem_addr)); break;
            /*
            case Type::BindDescriptor: return sizeof(VkDescriptorSet);
            case Type::Push: return sizeof(GlobalPushConstant_Raw);
            case Type::BeginRender: return sizeof(VkRenderingInfo);
            case Type::EndRender: return 0;
            
            case Type::Draw: return sizeof(ParamPack::VertexDraw);
            case Type::DrawIndexed: return sizeof(ParamPack::IndexDraw);
            case Type::Dispatch: return sizeof(ParamPack::Dispatch);
            case Type::DrawMeshTasks: return sizeof(ParamPack::DrawMeshTasks);
            //raytracing here
            
            case Type::DrawIndirect: return sizeof(ParamPack::DrawIndirect);
            case Type::DrawIndexedIndirect: return sizeof(ParamPack::DrawIndirect);
            case Type::DispatchIndirect: return sizeof(ParamPack::DispatchIndirect);
            case Type::DrawMeshTasksIndirect: return sizeof(ParamPack::DrawIndirect);
            
            case Type::DrawIndirectCount: return sizeof(ParamPack::DrawIndirectCount);
            case Type::DrawIndexedIndirectCount: return sizeof(ParamPack::DrawIndirectCount);
            case Type::DrawMeshTasksIndirectCount: return sizeof(ParamPack::DrawIndirectCount);
            //case Type::PipelineBarrier: return 0;
            case Type::DS_Viewport: return sizeof(ParamPack::Viewport);
            case Type::DS_Scissor: return sizeof(ParamPack::Scissor);
            case Type::DS_ViewportCount: return sizeof(ParamPack::ViewportCount);
            case Type::DS_ScissorCount: return sizeof(ParamPack::ScissorCount);
            case Type::BeginLabel: return sizeof(ParamPack::Label);
            case Type::EndLabel: return 0;
            case Type::If: return sizeof(bool);
            //other loop controls
            case Type::EndIf: return 0;
            */

            //bunch of dynamic state that i havent got to yet
        }
        //EWE_UNREACHABLE;
        
    }

#undef FUNC_ENTRY
} //namespace EWE
#endif