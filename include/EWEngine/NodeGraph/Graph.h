#pragma once

#include "EightWinds/PerFlight.h"
#include "EightWinds/Buffer.h"

#include "EWEngine/NodeGraph/Node.h"
#include "EWEngine/NodeGraph/ContiguousContainer.h"

#include "EightWinds/RenderGraph/Command/Record.h"

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

            DeferredReference<GlobalPushConstant>* def_push = nullptr;
            DeferredReference<VertexDrawParamPack>* def_vertParamPack = nullptr;
            DeferredReference<LabelParamPack>* def_label = nullptr;
            DeferredReference<PipelineParamPack>* def_pipe = nullptr;

            Pipeline* pipe;
            Shader* vert_shader;
            Shader* frag_shader;
            PipeLayout* pipeLayout;

            //how do I robustly connect this with GPUTask?
            void Record(CommandRecord& record);

            void InitializeRender();
            void UpdateRender(Input::Mouse const& mouseData);

#ifdef EWE_IMGUI
            void Imgui();
#endif

            //drawing configurations
        };
    }
}//namespace EWE