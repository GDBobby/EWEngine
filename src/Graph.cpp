#include "EWEngine/NodeGraph/Graph.h"

#include "EWEngine/Global.h"

#include "EightWinds/Pipeline/Graphics.h"

#include <filesystem>
#include <fstream>

#ifdef EWE_IMGUI
#include "imgui.h"
#include "magic_enum/magic_enum.hpp"

//template<typename T>
//void imgui_enum(std::string_view name, T& val, int min, int max) {
//
//    ImGui::SliderInt(name.data(), reinterpret_cast<int*>(&val), min, max, magic_enum::enum_name(val).data());
//}
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
            static Input::Button lmb;

            lmb.Update(mouseData.buttons[GLFW_MOUSE_BUTTON_LEFT]);

            lab::ivec2 mousePos{ static_cast<int>(mouseData.current_pos.x), static_cast<int>(mouseData.current_pos.y) };
            current_mouse_coverage.mouse_pos = mousePos;
            static lab::ivec2 old_mouse_pos;

            if (lmb.current == Input::Button::Pressed) {
                current_mouse_coverage.index = -1;


                for (auto& node : nodes) {
                    if (node.Update(mouseData)) {
                        const lab::vec2 inner_coord = UI::CoordWithinQuad(mousePos, node.buffer->position, node.buffer->scale, Global::window->screenDimensions.width, Global::window->screenDimensions.height);
                        if (inner_coord.x >= 0.0f && inner_coord.x <= 1.0f && inner_coord.y <= 1.0f && inner_coord.y >= 0.0f) {
                            const lab::vec2 adjusted_center_coords{
                                lab::Abs(inner_coord.x - 0.5f) * 2.f,
                                lab::Abs(inner_coord.y - 0.5f) * 2.f
                            };
                            if ((adjusted_center_coords.x > node.buffer->foregroundScale) && (adjusted_center_coords.y > node.buffer->foregroundScale)) {
                                current_mouse_coverage.selection = MouseCoveragePackage::Selection::Resize;
                            }
                            else if (inner_coord.y < (1.0f - node.buffer->titleScale)) {
                                current_mouse_coverage.selection = MouseCoveragePackage::Selection::Translate;
                            }

                            current_mouse_coverage.index = node.index;
                            current_mouse_coverage.coord_within_node = inner_coord;
                        }
                    }
                }
            }
            else if(lmb.current == Input::Button::Released) {
                current_mouse_coverage.selection = MouseCoveragePackage::Selection::None;
            }

            switch (current_mouse_coverage.selection) {
                case MouseCoveragePackage::Selection::None: break;
                case MouseCoveragePackage::Selection::Translate: {
                    const lab::ivec2 mouse_diff{
                        mousePos.x - old_mouse_pos.x,
                        mousePos.y - old_mouse_pos.y
                    };
                    const lab::vec2 transDiff = UI::Position_Difference(mouse_diff, Global::window->screenDimensions.width, Global::window->screenDimensions.height);
                    printf("trans diff : %.2f:%.2f\n", transDiff.x, transDiff.y);
                    nodes[current_mouse_coverage.index].buffer->position.x += transDiff.x;
                    nodes[current_mouse_coverage.index].buffer->position.y += transDiff.y;
                    break;
                }
                case MouseCoveragePackage::Selection::Resize: {

                    const lab::ivec2 mouse_diff{
                        mousePos.x - old_mouse_pos.x,
                        mousePos.y - old_mouse_pos.y
                    };
                    const lab::vec2 transDiff = UI::Position_Difference(mouse_diff, Global::window->screenDimensions.width, Global::window->screenDimensions.height);
                    printf("scale diff : %.2f:%.2f\n", transDiff.x, transDiff.y);
                    nodes[current_mouse_coverage.index].buffer->scale += transDiff;
                    break;
                }
                case MouseCoveragePackage::Selection::Link: {

                    break;
                }
            }

            old_mouse_pos = mousePos;
           
            drawData.paramPack->GetRef(frameIndex).instanceCount = nodes.size() + pins.size();
        }

        Pin& Graph::AddPin(NodeID node_index) {
            const uint32_t index = nodes.size() + pins.size();
            auto& parent_node = nodes[node_index];
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index, node_index);
        }
        Pin& Graph::AddPin(std::string_view name, NodeID node_index) {
            const uint32_t index = nodes.size() + pins.size();
            auto& parent_node = nodes[node_index];
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index, node_index);
        }
        Pin& Graph::AddPin(Node& parent_node) {
            const uint32_t index = nodes.size() + pins.size();
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index, parent_node.index);
        }
        Pin& Graph::AddPin(std::string_view name, Node& parent_node) {
            const uint32_t index = nodes.size() + pins.size();
            parent_node.pins.emplace_back(pins.size());
            return pins.emplace_back(reinterpret_cast<NodeBuffer*>(gp_buffer.GetMapped()) + index, index, parent_node.index);
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

        void Graph::Serialize() {
            std::ofstream outFile{ name };
            ProcessStream(outFile);
            outFile.close();
        }
        void Graph::Deserialize(std::string_view name) {
            std::ifstream inFile{ name.data(), std::ios::binary };
            if (!inFile.is_open()) {
                inFile.open(name.data());
            }
            if (!inFile.is_open()) {
                throw std::runtime_error("failed to load graph file");
            }
            ProcessStream(inFile);
            inFile.close();
        }

#ifdef EWE_IMGUI

        void Graph::ImguiMenuBar() {

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New", "")) {
                    }
                    if (ImGui::MenuItem("Save", "Ctrl + S")) {
                        Serialize();
                        //LevelManager::saveLevel()
                    }
                    if (ImGui::MenuItem("Save As", "")) {
                        //Serialize();
                        //open some kinda saving screen, maybe the file explorer again
                    }

                    if (ImGui::MenuItem("Load", "Ctrl + L")) {
                        selecting_file = true;
                    }
                    //if (ImGui::MenuItem("Exit", "")) {
                    //    closePlease = true;
                    //}
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Settings")) {
                    ImGui::Checkbox("nodes resizable [Ctrl + R]", &settings.nodes_resizable);
                    if (ImGui::SliderFloat("global background scale", &settings.global_background_scale, 0.5f, 1.f)) {
                        for (auto& node : nodes) {
                            node.buffer->foregroundScale = settings.global_background_scale;
                        }
                    }
                    if (ImGui::SliderFloat("global title scale", &settings.global_title_scale, 0.5f, 1.f)) {
                        for (auto& node : nodes) {
                            node.buffer->titleScale = settings.global_title_scale;
                        }
                    }
                    if (ImGui::ColorEdit3("global background color", &settings.global_background_color.x)) {
                        for (auto& node : nodes) {
                            node.buffer->backgroundColor = settings.global_background_color;
                        }
                    }
                    if (ImGui::ColorEdit3("global foreground color", &settings.global_foreground_color.x)) {
                        for (auto& node : nodes) {
                            node.buffer->foregroundColor = settings.global_foreground_color;
                        }
                    }
                    if (ImGui::ColorEdit3("global title color", &settings.global_title_color.x)) {
                        for (auto& node : nodes) {
                            node.buffer->titleColor = settings.global_title_color;
                        }
                    }
                    ImGui::EndMenu();
                }
                //printf("after ?? \n");
                ImGui::EndMainMenuBar();
            }
        }

        void Graph::Imgui() {
            ImguiMenuBar();

            if (selecting_file) {
                if (ImGui::Begin(fileExplorer.current_path.string().c_str(), &selecting_file)) {
                    fileExplorer.Imgui();

                }
                ImGui::End();
                if (fileExplorer.selected_file.has_value()) {
                    CloseFile();
                    OpenFile(fileExplorer.selected_file.value().string());
                    fileExplorer.selected_file = std::nullopt;
                }
            }


            if (ImGui::Begin("node graph")) {

                constexpr std::size_t name_size = 128;
                char name_buffer[name_size];
                strncpy(name_buffer, name.c_str(), name_size);
                if (ImGui::InputText("graph name", name_buffer, name_size)) {
                    name = name_buffer;
                }
                ImGui::Text("current_mouse_coverage : %d", current_mouse_coverage.index);
                ImGui::Text("mouse pos : {%d:%d}", current_mouse_coverage.mouse_pos.x, current_mouse_coverage.mouse_pos.y);
                if (current_mouse_coverage.index >= 0) {
                    ImGui::Text("node : {%d} - innner coord : {%.2f:%.2f} : %s", current_mouse_coverage.index, current_mouse_coverage.coord_within_node.x, current_mouse_coverage.coord_within_node.y, magic_enum::enum_name(current_mouse_coverage.selection).data());

                }

                ImGui::Text("node count : %zu", nodes.size());
                if(ImGui::Button("Add Node")){
                    AddNode();
                }
                
                for (auto& node : nodes) {
                    //do some kinda tree thing
                    ImGui::PushID(node.index); 
                    if (ImGui::TreeNodeEx((void*)(intptr_t)node.index, 0, "%s", node.name.c_str())) {
                        node.Imgui();
                        if (ImGui::Button("add pin")) {
                            auto& pin = AddPin(node);
                            pin.buffer->InitPin();
                        }
                        if (ImGui::TreeNode("pins")) {
                            for (auto& pin_index : node.pins) {
                                pins[pin_index].Imgui();
                            }
                            ImGui::TreePop();
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::End();
        }
#endif

    } //namespace Node
} //namespace EWE