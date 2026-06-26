#pragma once

namespace EWE{

    struct RenderGraph;
    struct EWEException;

    void Debug_RenderGraph_DEVICE_LOST(RenderGraph const& renderGraph, EWEException const& except);
} //namespace EWE