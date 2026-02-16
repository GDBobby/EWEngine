#include "EWEngine/Imgui/Framework_Imgui.h"


#ifdef EWE_IMGUI
#include "imgui.h"


#include "EightWinds/RenderGraph/RenderGraph.h"
#include "EightWinds/RenderGraph/RasterTask.h"
#include "EightWinds/RenderGraph/GPUTask.h"
#include "EightWinds/RenderGraph/Resources.h"
#include "EightWinds/GlobalPushConstant.h"
#include "EightWinds/Pipeline/TaskRasterConfig.h"

#include "EightWinds/Pipeline/Graphics.h"

#include "magic_enum/magic_enum.hpp"

#include <string>


template<typename T>
void imgui_enum(std::string_view name, T& val, int min, int max) {

    ImGui::SliderInt(name.data(), reinterpret_cast<int*>(&val), min, max, magic_enum::enum_name(val).data());
}

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
    
    template<> void ImguiExtension::Imgui(RenderGraph& obj){
		
        for (auto& task : obj.tasks) {
            ImGui::Text("Task name : %s", task.name.c_str());
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

        ImGui::Text("present result : %s", magic_enum::enum_name(obj.presentResult).data());
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
        imgui_enum("rast samples", obj.rastSamples, 0, 128);
        ImGui::Checkbox("sample shading", &obj.enable_sampleShading);
        if (obj.minSampleShading) {
            ImGui::DragFloat("min sample shading", &obj.minSampleShading, 0.f, 100.f);
        }
        ImGui::Checkbox("alpha to coverage", &obj.alphaToCoverageEnable);
        ImGui::Checkbox("alpha to one enable", &obj.alphaToOneEnable);

        for (auto& dynS : obj.dynamicState) {
            ImGui::Text("dyn state - %s", magic_enum::enum_name(dynS).data());
        }

        ImguiExtension::Imgui(obj.attachment_set_info);

        ImGui::PopID();
    }


    FUNC_ENTRY(ObjectRasterConfig) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        ImGui::Checkbox("depth clamp", &obj.depthClamp);
        ImGui::Checkbox("rasterizer discard", &obj.rasterizerDiscard);
        imgui_enum("polygon mode", obj.polygonMode, 0, 3);

        ImGui::RadioButton("none", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_NONE); ImGui::SameLine();
        ImGui::RadioButton("front", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_FRONT_BIT); ImGui::SameLine();
        ImGui::RadioButton("back", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_BACK_BIT); ImGui::SameLine();
        ImGui::RadioButton("front and back", reinterpret_cast<int*>(&obj.cullMode), VK_CULL_MODE_FRONT_AND_BACK);

        imgui_enum("front face", obj.frontFace, 0, 2);

        ImguiExtension::Imgui(obj.depthBias);

        imgui_enum("topology", obj.topology, 0, 12);
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
    FUNC_ENTRY(GPUTask) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        ImGui::Text("gpu task - %s", obj.name.c_str());
        ImGui::Text("queue index : %d", obj.queue.FamilyIndex());

        ImguiExtension::Imgui(obj.resources);

        ImGui::PopID();
    }
    FUNC_ENTRY(TaskResourceUsage) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);

        ImGui::Text("image count[%d] : buffer count[%d]");
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
        //imgui_enum("rendering flags", obj.renderingFlags, 0, 64); //were just gonna assume this is 0 til reflection comes in

        int col_count = obj.colors.size();
        if (ImGui::DragInt("color count", &col_count, 0, 16)) {
            obj.colors.resize(col_count);
        }
        for (auto& col : obj.colors) {
            //ImguiExtension::Imgui(col.format);
            imgui_enum("format", col.format, 0, 256);
        }
        ImguiExtension::Imgui(obj.depth);

        ImGui::PopID();
    }
    FUNC_ENTRY(VkPipelineDepthStencilStateCreateInfo) {
        int temp_id = static_cast<int>(reinterpret_cast<std::size_t>(&obj)); //im fine with the inaccuracy imposed by the reduction in bits
        ImGui::PushID(temp_id);
        imgui_bool_check("depth test enable", obj.depthTestEnable);
        imgui_bool_check("depth write enable", obj.depthWriteEnable);
        imgui_enum("depth compare op", obj.depthCompareOp, 0, 8);
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
        imgui_enum("src color blend factor", obj.srcColorBlendFactor, 0, 20);
        imgui_enum("dst color blend factor", obj.dstColorBlendFactor, 0, 20);
        imgui_enum("color blend op", obj.colorBlendOp, 0, 5);

        imgui_enum("src alpha blend factor", obj.srcAlphaBlendFactor, 0, 20);
        imgui_enum("dst alpha blend factor", obj.dstAlphaBlendFactor, 0, 20);
        imgui_enum("alpha blend op", obj.alphaBlendOp, 0, 5);

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

        ImGui::DragInt("stage", reinterpret_cast<int*>(&obj.stage), 0, 65536);
        ImGui::DragInt("access mask", reinterpret_cast<int*>(&obj.accessMask), 0, 65536);

        imgui_enum("layout", obj.layout, 0, 9);

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
                ImGui::Text(magic_enum::enum_name(temp_val.value).data());
                ImGui::SameLine();
                ImGui::Text(" : %s", obj.shaders[i]->name.c_str());
                ImguiExtension::Imgui(*obj.shaders[i]);
            }

            ImGui::Text("Pipeline Type : %s", magic_enum::enum_name(obj.pipelineType).data());
            ImGui::Text("Bind Point : %s", magic_enum::enum_name(obj.bindPoint).data());
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
        imgui_enum("format", obj.format, 0, 256);
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

    template<> void ImguiExtension::Imgui(VkStencilOpState& obj) {
        imgui_enum("fail op", obj.failOp, 0, 8);
        imgui_enum("pass op", obj.passOp, 0, 8);
        imgui_enum("depth fail op", obj.depthFailOp, 0, 8);
        imgui_enum("compare op", obj.compareOp, 0, 8);
        ImGui::DragInt("compare mask", reinterpret_cast<int*>(&obj.compareMask), 0, INT32_MAX);
        ImGui::DragInt("write mask", reinterpret_cast<int*>(&obj.compareMask), 0, INT32_MAX);
        ImGui::DragInt("reference", reinterpret_cast<int*>(&obj.reference), 0, INT32_MAX);

    }

    template<> void ImguiExtension::Imgui(Command::Record& obj) {
        for (std::size_t i = 0; i < obj.records.size(); i++) {
            ImGui::Text("%zu\t:%s:%zu", i, magic_enum::enum_name(obj.records[i].type).data(), obj.records[i].paramOffset);
        }
    }

#undef FUNC_ENTRY
} //namespace EWE
#endif