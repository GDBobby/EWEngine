#pragma once

#include <filesystem>

namespace EWE{
    using AssetHash = uint64_t;
namespace Asset{
    static constexpr AssetHash INVALID_HASH = UINT64_MAX; 

    AssetHash CrossPlatformPathHash(std::filesystem::path const& path);

    template<typename T>
    AssetHash GetHash(T const& res);
} //namespace Asset

    #define FWD_DEC_HASH(Type) struct Type; template<> AssetHash Asset::GetHash(Type const& type)

    FWD_DEC_HASH(Buffer);
    FWD_DEC_HASH(Image);
    FWD_DEC_HASH(ImageView);
namespace Command{
    struct InstructionPackage;
    struct ObjectPackage;
    struct PackageRecord;
} //namespace Command
    template<> AssetHash Asset::GetHash(Command::InstructionPackage const& type);
    template<> AssetHash Asset::GetHash(Command::ObjectPackage const& type);
    template<> AssetHash Asset::GetHash(Command::PackageRecord const& type);

    FWD_DEC_HASH(RasterPackage);
    FWD_DEC_HASH(Sampler);
    FWD_DEC_HASH(DescriptorImageInfo);
    FWD_DEC_HASH(SubmissionTask);
    FWD_DEC_HASH(Shader);
    FWD_DEC_HASH(RenderGraph);

    #undef FWD_DEC_HASH
} //namespace EWE