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

        void Graph::Record(RasterTask& rasterTask) {
           // printf("label addr - %zu\n"), def_label->data;

            vert_shader = new Shader(*Global::logicalDevice, "examples/common/shaders/node.vert.spv");
            frag_shader = new Shader(*Global::logicalDevice, "examples/common/shaders/node.frag.spv");
            pipeLayout = new PipeLayout(*Global::logicalDevice, { vert_shader, frag_shader });

            EWE::ObjectRasterConfig objectConfig;
            objectConfig.SetDefaults();
            objectConfig.cullMode = VK_CULL_MODE_NONE;
            objectConfig.depthClamp = false;
            objectConfig.rasterizerDiscard = false;
            objectConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            objectConfig.polygonMode = VK_POLYGON_MODE_FILL;
            //pipe = new GraphicsPipeline(*Global::logicalDevice, 1, pipeLayout, passConfig, objectConfig, { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

            ObjectRasterData config{
                .layout = pipeLayout,
                .config = objectConfig
            };

            rasterTask.AddDraw(config, drawData);

            //task raster config isn't gonna pla ynicely with labels, unless I allow the user to input arbitrary commands
        }

        void Graph::Undefer() {
            for (uint8_t i = 0; i < GlobalPushConstant_Raw::buffer_count; i++) {
                drawData.buffers[i] = nullptr;
            }

            drawData.buffers[0] = &gp_buffer;
            drawData.UpdateBuffer();
            for (uint8_t i = 0; i < max_frames_in_flight; i++) {
                auto& vertParamPack = drawData.paramPack->GetRef(i);
                vertParamPack.firstInstance = 0;
                vertParamPack.firstVertex = 0;
                vertParamPack.vertexCount = 4;
                vertParamPack.instanceCount = 0;
            }
        }

        void Graph::UpdateRender(Input::Mouse const& mouseData, uint8_t frameIndex) {
            for (auto& node : nodes) {
                node.Update(mouseData);
            }

            drawData.paramPack->GetRef(frameIndex).instanceCount = nodes.size();
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
                ImGui::Text("node count : %zu", nodes.size());
                for (auto& node : nodes) {
                    //do some kinda tree thing
                    if (ImGui::TreeNode(node.name.c_str())) {
                        node.Imgui();
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::End();
        }
#endif
    }
}