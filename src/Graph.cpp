#include "EWEngine/NodeGraph/Graph.h"

#include "EWEngine/Global.h"

#include "EightWinds/Pipeline/Graphics.h"

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

namespace EWE{
    namespace Node{
        Graph::Graph()
            : gp_buffer{
                *Global::logicalDevice, 
                sizeof(NodeBuffer) * MaxNodeCount, 1, 
                VmaAllocationCreateInfo{
                    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                    .usage = VMA_MEMORY_USAGE_AUTO
                }
            }
        {
            gp_buffer.Map();
        }

        void Graph::Record(CommandRecord& record) {
            def_label = record.BeginLabel();
            def_pipe = record.BindPipeline();
            def_push = record.Push();
            def_vertParamPack = record.Draw();
            record.EndLabel();
        }

        void Graph::InitializeRender() {
           // printf("label addr - %zu\n"), def_label->data;
            def_push->GetRef().buffer_addr[0] = gp_buffer.deviceAddress;
            auto& vertParamPack = def_vertParamPack->GetRef();
            vertParamPack.firstInstance = 0;
            vertParamPack.firstVertex = 0;
            vertParamPack.vertexCount = 4;
            auto& labelPack = def_label->GetRef();
            labelPack.name = "node graph";
            labelPack.red = 1.f;
            labelPack.green = 0.f;
            labelPack.blue = 0.f;

            vert_shader = new Shader(*Global::logicalDevice, "examples/common/shaders/node.vert.spv");
            frag_shader = new Shader(*Global::logicalDevice, "examples/common/shaders/node.frag.spv");

            pipeLayout = new PipeLayout(*Global::logicalDevice, { vert_shader, frag_shader });
            EWE::PipelinePassConfig passConfig;
            passConfig.SetDefaults();
            passConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
            passConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
            passConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
            passConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;

            EWE::PipelineObjectConfig objectConfig;
            objectConfig.SetDefaults();
            objectConfig.cullMode = VK_CULL_MODE_NONE;
            objectConfig.depthClamp = false;
            objectConfig.rasterizerDiscard = false;
            objectConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            pipe = new GraphicsPipeline(*Global::logicalDevice, 1, pipeLayout, passConfig, objectConfig, { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

            auto& pipeParams = def_pipe->GetRef();
            pipeParams.pipe = pipe->vkPipe;
            pipeParams.layout = pipe->pipeLayout->vkLayout;
            pipeParams.bindPoint = pipe->pipeLayout->bindPoint;
        }

        void Graph::UpdateRender(Input::Mouse const& mouseData) {
            for (auto& node : nodes) {
                node.Update(mouseData);
            }
            def_vertParamPack->GetRef().instanceCount = nodes.size();
        }

        Node& Graph::AddNode() {
            const uint32_t index = nodes.size();
            return nodes.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index);
        }
        Node& Graph::AddNode(std::string_view name) {
            const uint32_t index = nodes.size();
            return nodes.emplace_back(name, reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index);
        }

#ifdef EWE_IMGUI
        void Graph::Imgui() {
            if (ImGui::Begin("node graph")) {
                for (auto& node : nodes) {
                    //do some kinda tree thing
                    node.Imgui();
                }
            }
            ImGui::End();
        }
#endif
    }
}