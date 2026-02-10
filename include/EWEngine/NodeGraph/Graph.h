#pragma once

#include "EightWinds/Data/PerFlight.h"
#include "EightWinds/Buffer.h"

#include "EWEngine/NodeGraph/Node.h"
#include "EWEngine/NodeGraph/ContiguousContainer.h"

#include "EightWinds/RenderGraph/Command/Record.h"
#include "EightWinds/RenderGraph/RasterTask.h"

#include "EightWinds/Pipeline/PipelineBase.h"

#include "EWEngine/Tools/ImguiFileExplorer.h"

#include "EWEngine/Preprocessor.h"
#include "EightWinds/Data/StreamHelper.h"

#include "EWEngine/Tools/UIHelper.h"

#include <vector>

namespace EWE{
    namespace Node{
        struct MouseCoveragePackage {
            NodeID index;
            lab::vec2 coord_within_node;
            lab::ivec2 mouse_pos;

            enum class Selection {
                None,
                Resize,
                Translate,
                Link,
            };
            Selection selection = Selection::None;
        };

        struct Graph{
            static constexpr std::size_t graph_version = 1;

            std::string name = "new file";
            static constexpr uint32_t MaxNodeCount = 4096;
            //std::array<NodeBuffer, MaxNodeCount> nodeBuffers{};
            Buffer gp_buffer;

            [[nodiscard]] explicit Graph();
        
            ContiguousContainer<Node> nodes{};
            ContiguousContainer<Pin> pins; //removing a pin is going to invaidate larger indexes

            //the references are getting invalidated, be careful with lfietime and switch to the index immediately
            Node& AddNode(std::string_view name);
            Node& AddNode();
            Pin& AddPin(Node& parent_node);
            Pin& AddPin(std::string_view name, Node& parent_node);
            Pin& AddPin(NodeID node_index);
            Pin& AddPin(std::string_view name, NodeID node_index);


            VertexDrawData drawData;

            Shader* node_vert_shader;
            Shader* node_frag_shader;
            PipeLayout* node_pipeLayout;

            struct Settings {
                bool nodes_resizable = false;
                float global_background_scale = 0.95f;
                float global_title_scale = 0.9f;
                lab::vec3 global_background_color{ 0.f, 0.2f, 0.2f };
                lab::vec3 global_foreground_color{ 0.5f, 0.5f, 0.f };
                lab::vec3 global_title_color{ 1.f, 1.f, 0.8f };
            };
            Settings settings;
            MouseCoveragePackage current_mouse_coverage;

            //Shader* pin_vert_shader;
            //Shader* pin_frag_shader;
            //PipeLayout* pin_pipeLayout;

            //how do I robustly connect this with GPUTask?
            void Record(RasterTask& rasterTask);
            void Undefer();

            void UpdateRender(Input::Mouse const& mouseData, uint8_t frameIndex);

#ifdef EWE_IMGUI
            void ImguiMenuBar();
            void Imgui();
#endif

            void Serialize();
            void Deserialize(std::string_view name);

            bool selecting_file = false;
            void SaveFile();
            void SaveFileAs(std::string_view dstName);
            void CloseFile();
            void OpenFile(std::string_view name);

            template<typename StreamObj>
            void ProcessStream(StreamObj& streamObj) {
                Stream::Operator<StreamObj> stream{ streamObj };
                std::size_t temp_buffer = graph_version;
                stream.Process(temp_buffer);
#if EWE_DEBUG
                assert(temp_buffer == graph_version);
#endif
                stream.Process(settings);
                temp_buffer = nodes.size();
                stream.Process(temp_buffer);
                temp_buffer = pins.size();
                stream.Process(temp_buffer);
                //outFile << links.size(); //idk yet


                for (auto& node : nodes) {
                    temp_buffer = node.name.size();
                    stream.Process(temp_buffer);
                    stream.Process(node.name.data(), node.name.size());
                    node.buffer->ProcessStream(streamObj);
                    //link count? or separate?
                }
                for (auto& pin : pins) {
                    temp_buffer = pin.name.size();
                    stream.Process(temp_buffer);
                    stream.Process(pin.name.data(), pin.name.size());
                    stream.Process(pin.parentNode);
                    pin.buffer->ProcessStream(streamObj);
                }
            }

#ifdef EWE_IMGUI
            ExplorerContext fileExplorer;
#endif
            //drawing configurations
        };
    }
}//namespace EWE