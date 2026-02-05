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

#include <vector>

namespace EWE{
    namespace Node{
        struct Graph{
            static constexpr std::size_t graph_version = 0;

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

            //Shader* pin_vert_shader;
            //Shader* pin_frag_shader;
            //PipeLayout* pin_pipeLayout;

            //how do I robustly connect this with GPUTask?
            void Record(RasterTask& rasterTask);
            void Undefer();

            void UpdateRender(Input::Mouse const& mouseData, uint8_t frameIndex);

#ifdef EWE_IMGUI
            void Imgui();
#endif

            void Serialize();
            void Deserialize(std::string_view name);

            bool selecting_file = false;
            void SaveFile();
            void SaveFileAs(std::string_view dstName);
            void CloseFile();
            void OpenFile(std::string_view name);

#ifdef EWE_IMGUI
            ExplorerContext fileExplorer;
#endif
            //drawing configurations
        };
    }
}//namespace EWE