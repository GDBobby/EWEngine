#pragma once

#include "EWEngine/Assets/Hash.h"

#include "EWEngine/Assets/Base.h"

#include "EWEngine/Assets/Buffers.h"
#include "EWEngine/Assets/DII.h"
#include "EWEngine/Assets/GPUTasks.h"
#include "EWEngine/Assets/Images.h"
#include "EWEngine/Assets/ImageViews.h"
#include "EWEngine/Assets/BaseInstPackages.h"
#include "EWEngine/Assets/ObjectPackages.h"
#include "EWEngine/Assets/PackageRecords.h"
#include "EWEngine/Assets/RasterPackages.h"
#include "EWEngine/Assets/Samplers.h"
#include "EWEngine/Assets/SubmissionTasks.h"
#include "EWEngine/Assets/Shaders.h"

namespace EWE{

namespace Asset{

    template<>
    struct Manager<AssetHash>{
        [[nodiscard]] explicit Manager(std::filesystem::path const& asset_directory);

        std::filesystem::path root_directory;

        Manager<Buffer> buffer;
        //Manager<GPUTask> gpuTask;
        Manager<Image> image;
        Manager<ImageView> imageView;
        Manager<Command::InstructionPackage> instPkg;
        Manager<Command::ObjectPackage> objPkg;
        Manager<Command::PackageRecord> pkgRecord;
        Manager<RasterPackage> rasterTask;
        Manager<Sampler> sampler;
        Manager<DescriptorImageInfo> dii;
        Manager<SubmissionTask> subTask;
        Manager<Shader> shader;

        void DropCallback(std::filesystem::path const& path);

#ifdef EWE_IMGUI
        void Imgui();
#endif
    };
} //namespace Asset
    using AssetManager = Asset::Manager<AssetHash>;
    void GLFW_Drop_Callback(GLFWwindow* window, int count, const char** paths);
} //namespace EWE

