#pragma once

#include "EightWinds/Data/PerFlight.h"
#include "EightWinds/Buffer.h"

#include "EWEngine/NodeGraph/Node.h"
#include "EWEngine/NodeGraph/ContiguousContainer.h"

#include "EightWinds/RenderGraph/Command/Record.h"
#include "EightWinds/RenderGraph/RasterTask.h"

#include "EightWinds/Pipeline/PipelineBase.h"


#include <vector>

namespace EWE{
    namespace Node{
        struct Graph{
            static constexpr uint32_t MaxNodeCount = 1024;
            //std::array<NodeBuffer, MaxNodeCount> nodeBuffers{};
            Buffer gp_buffer;

            [[nodiscard]] explicit Graph();
        
            ContiguousContainer<Node> nodes{};

            Node& AddNode(std::string_view name);
            Node& AddNode();

            VertexDrawData drawData;

            Shader* vert_shader;
            Shader* frag_shader;
            PipeLayout* pipeLayout;

            //how do I robustly connect this with GPUTask?
            void Record(RasterTask& rasterTask);
            void Undefer();

            void UpdateRender(Input::Mouse const& mouseData, uint8_t frameIndex);

#ifdef EWE_IMGUI
            void Imgui();
#endif

            //drawing configurations
        };
    }
}//namespace EWE