#include "EWEngine/NodeGraph/Graph.h"

#include "EWEngine/Global.h"

#include "EightWinds/Pipeline/Graphics.h"

#include "EightWinds/Data/StreamHelper.h"

#include <filesystem>

#ifdef EWE_IMGUI
#include "imgui.h"
#endif

#include <fstream>

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
            },
            fileExplorer{ std::filesystem::current_path() }
        {
            fileExplorer.acceptable_extensions.push_back("ewng");
            gp_buffer.Map();
        }

        void Graph::Record(RasterTask& rasterTask) {
           // printf("label addr - %zu\n"), def_label->data;

            node_vert_shader = new Shader(*Global::logicalDevice, "common/shaders/node.vert.spv");
            node_frag_shader = new Shader(*Global::logicalDevice, "common/shaders/node.frag.spv");
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

        void Graph::SaveFile() {
            SaveFileAs(name);
        }
        void Graph::SaveFileAs(std::string_view dstName) {

        }

        void Graph::CloseFile() {
            nodes.clear();
            pins.clear();
            //nothing else? links?
        }
        void Graph::OpenFile(std::string_view name) {
            Deserialize(name);
        }

#ifdef EWE_IMGUI
        void Graph::Imgui() {
            if (selecting_file) {
                if (ImGui::Begin(fileExplorer.current_path.string().c_str(), &selecting_file)) {
                    fileExplorer.Imgui();

                    ImGui::End();
                }
                if (fileExplorer.selected_file.has_value()) {
                    CloseFile();
                    OpenFile(fileExplorer.selected_file.value().string());
                }
            }


            if (ImGui::Begin("node graph")) {
                
                constexpr std::size_t name_size = 128;
                char name_buffer[name_size];
                strncpy(name_buffer, name.c_str(), name_size);
                if (ImGui::InputText("graph name", name_buffer, name_size)) {
                    name = name_buffer;
                }
                if (ImGui::Button("Serialize")) {
                    Serialize();
                }
                
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

        void Graph::Serialize() {
            //compress the data first? shift down nodes and pins if they weren't already compressed?

            printf("current filepath : %s\n", std::filesystem::current_path().string().c_str());

            const std::string file_name = name + ".ewng";
            std::ofstream outFile{ file_name.c_str(), std::ios::binary };
            //header
            //outFile << name.size(); //the file is named after the name
            Stream::Helper(outFile, graph_version);
            std::size_t temp_buffer = nodes.size();
            Stream::Helper(outFile, temp_buffer);
            temp_buffer = pins.size();
            Stream::Helper(outFile, temp_buffer);
            //outFile << links.size(); //idk yet


            for (auto& node : nodes) {
                temp_buffer = node.name.size();
                Stream::Helper(outFile, temp_buffer);
                outFile.write(node.name.c_str(), node.name.size());
                node.buffer->Serialize(outFile);
                //link count? or separate?
            }
            for (auto& pin : pins) {
                temp_buffer = pin.name.size();
                Stream::Helper(outFile, temp_buffer);
                outFile.write(pin.name.c_str(), pin.name.size());
                outFile.write(reinterpret_cast<const char*>(&pin.parentNode), sizeof(pin.parentNode));
                pin.buffer->Serialize(outFile);
            }

            outFile.close();
        }

        void Graph::Deserialize(std::string_view name) {
            std::ifstream inFile{ name.data(), std::ios::binary};

            //header
            std::size_t version;
            inFile >> version;
            std::size_t nodeCount;
            inFile >> nodeCount;
            std::size_t pinCount;
            inFile >> pinCount;

            std::size_t current_buffer_index = 0;

            nodes.reserve(nodeCount);

            for (std::size_t i = 0; i < nodeCount; i++) {

                std::size_t nameSize;
                inFile >> nameSize;
                std::string nameBuffer;
                nameBuffer.resize(nameSize);
                inFile.read(nameBuffer.data(), nameSize);
                Node& temp_node = AddNode(nameBuffer);

                //std::size_t node_pin_count;
                //inFile >> node_pin_count; //idk how to do this yet, do i even care or do i just trust the pins to sort it out

                temp_node.buffer->Deserialize(inFile);
            }
            for (std::size_t i = 0; i < pinCount; i++){

                std::size_t nameSize;
                inFile >> nameSize;

                std::string nameBuffer;
                nameBuffer.resize(nameSize);
                inFile.read(nameBuffer.data(), nameSize);

                NodeID parentId;
                inFile >> parentId;
                Pin& temp_pin = AddPin(nameBuffer, parentId);

                temp_pin.buffer->Deserialize(inFile);
                nodes[parentId].pins.emplace_back(temp_pin.globalPinID);
            }
        }
    } //namespace Node
} //namespace EWE