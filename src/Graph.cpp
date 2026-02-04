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

            node_vert_shader = new Shader(*Global::logicalDevice, "examples/common/shaders/node.vert.spv");
            node_frag_shader = new Shader(*Global::logicalDevice, "examples/common/shaders/node.frag.spv");
            node_pipeLayout = new PipeLayout(*Global::logicalDevice, { node_vert_shader, node_frag_shader });

            EWE::ObjectRasterConfig objectConfig;
            objectConfig.SetDefaults();
            objectConfig.cullMode = VK_CULL_MODE_NONE;
            objectConfig.depthClamp = false;
            objectConfig.rasterizerDiscard = false;
            objectConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            objectConfig.polygonMode = VK_POLYGON_MODE_FILL;
            objectConfig.blendAttachment.blendEnable = VK_TRUE;
            objectConfig.blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            objectConfig.blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            objectConfig.blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            objectConfig.blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            objectConfig.blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            objectConfig.blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            objectConfig.blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            //pipe = new GraphicsPipeline(*Global::logicalDevice, 1, pipeLayout, passConfig, objectConfig, { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });

            ObjectRasterData node_config{
                .layout = node_pipeLayout,
                .config = objectConfig
            };

            rasterTask.AddDraw(node_config, drawData);

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
            drawData.paramPack->GetRef(frameIndex).instanceCount = nodes.size() + pins.size();
        }

        Pin& Graph::AddPin(NodeID node_index) {
            const uint32_t index = nodes.size() + pins.size();
            auto& parent_node = nodes[node_index];
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index);
        }
        Pin& Graph::AddPin(std::string_view name, NodeID node_index) {
            const uint32_t index = nodes.size() + pins.size();
            auto& parent_node = nodes[node_index];
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index);
        }
        Pin& Graph::AddPin(Node& parent_node) {
            const uint32_t index = nodes.size() + pins.size();
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index);
        }
        Pin& Graph::AddPin(std::string_view name, Node& parent_node) {
            const uint32_t index = nodes.size() + pins.size();
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index);
        }

        Node& Graph::AddNode() {
            const uint32_t index = nodes.size() + pins.size();
            return nodes.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index, pins);
        }
        Node& Graph::AddNode(std::string_view name) {
            const uint32_t index = nodes.size();
            return nodes.emplace_back(name, reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index, pins);
        }

#ifdef EWE_IMGUI
        void Graph::Imgui() {
            if (ImGui::Begin("node graph")) {
                ImGui::Text("node count : %zu", nodes.size());
                
                for (auto& node : nodes) {
                    //do some kinda tree thing
                    if (ImGui::TreeNode(node.name.c_str())) {
                        node.Imgui();
                        const std::string add_pin_index = "add pin##" + std::to_string(node.index);
                        if (ImGui::Button(add_pin_index.c_str())) {
                            auto& pin = AddPin(node);
                            pin.buffer->InitPin();
                        }
                        const std::string pin_tree = "pins##" + std::to_string(node.index);
                        if (ImGui::TreeNode(pin_tree.c_str())) {
                            for (auto& pin_index : node.pins) {
                                pins[pin_index].Imgui();
                            }
                            ImGui::TreePop();
                        }
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::End();
        }
#endif
    }
}