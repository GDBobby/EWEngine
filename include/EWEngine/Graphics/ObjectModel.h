#pragma once

#include "EWEngine/Graphics/Model.h"
#include "EightWinds/RenderGraph/RasterPackage.h"

namespace EWE{

    struct ModelObject{
        Model* model; //pointer, reference, or regular member? idk
        //PipeLayout* layout; //could just be shaders, doesn't matter
        ObjectRasterData rasterConfig;
    };
}