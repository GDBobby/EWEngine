#include "EWEngine/Assets/Hash.h"

#include "xxhash.h"


#include "EightWinds/Buffer.h"
#include "EightWinds/Image.h"
#include "EightWinds/ImageView.h"
#include "EightWinds/Command/InstructionPackage.h"
#include "EightWinds/Command/ObjectInstructionPackage.h"
#include "EightWinds/Command/PackageRecord.h"
#include "EightWinds/RenderGraph/RasterPackage.h"
#include "EightWinds/Sampler.h"
#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/RenderGraph/SubmissionTask.h"
#include "EightWinds/Shader.h"
#include "EightWinds/RenderGraph/RenderGraph.h"

namespace EWE{
namespace Asset{

    AssetHash CrossPlatformPathHash(std::filesystem::path const& path){
        std::string gen_string = path.generic_string();

        return XXH3_64bits(gen_string.data(), gen_string.size());
    }
} //namespace Asset

#define HASH_IMPL(Type) template<> AssetHash Asset::GetHash(Type const& type) { return CrossPlatformPathHash(type.name); }
    
    HASH_IMPL(Buffer)
    HASH_IMPL(Image)
    HASH_IMPL(ImageView)
    template<> AssetHash Asset::GetHash(Command::InstructionPackage const& type) { return Asset::CrossPlatformPathHash(type.name); }
    template<> AssetHash Asset::GetHash(Command::ObjectPackage const& type) { return Asset::CrossPlatformPathHash(type.name); }
    template<> AssetHash Asset::GetHash(Command::PackageRecord const& type) { return Asset::CrossPlatformPathHash(type.name); }
    HASH_IMPL(RasterPackage)
    template<> AssetHash Asset::GetHash(Sampler const& type) {return Sampler::Condense(type.info);}
    HASH_IMPL(DescriptorImageInfo)
    HASH_IMPL(SubmissionTask)
    HASH_IMPL(Shader)
    HASH_IMPL(RenderGraph)

#undef HASH_IMPL
} //namespace EWE