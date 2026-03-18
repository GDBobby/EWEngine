#include "EWEngine/Assets/DII.h"

#include "EightWinds/DescriptorImageInfo.h"
#include "EightWinds/Sampler.h"

#include "EWEngine/Global.h"

namespace EWE{
namespace Asset{

    Manager<DescriptorImageInfo>::Manager(
        LogicalDevice& logicalDevice,
        Manager<Sampler>& samplers,
        Manager<ImageView>& views,
        std::filesystem::path const& root_path
    )
    : logicalDevice{logicalDevice},
        samplers{samplers},
        views{views},
        filesystem{root_path, std::vector<std::string>{".dii"}}
    {

    }

    static constexpr std::size_t dii_version = 0;

    void Manager<DescriptorImageInfo>::Read(std::string_view file_name){

        std::ifstream stream{file_name.data(), std::ios::binary};
        EWE_ASSERT(stream.is_open());

        Stream::Operator streamHandler{stream};

        std::size_t buffer;
        streamHandler.Process(buffer);
        EWE_ASSERT(buffer == dii_version);

        streamHandler.Process(buffer);

        //this assumes that only the default view can be used
        //otherwise, i would need to store a handle for the view
        std::string image_name{};
        image_name.resize(buffer);
        streamHandler.Process(image_name.data(), image_name.size());

        AssetHash view_hash;
        ImageView& view = views.Get(view_hash);

        Sampler* sampler = nullptr;
        uint8_t temp_buffer;
        streamHandler.Process(temp_buffer);
        if(temp_buffer == 1){
            //if we're here, and it's nullptr, we're reading. otherwise, writing
            uint64_t sampler_condensed;
            streamHandler.Process(sampler_condensed);
            sampler = &samplers.Get(sampler_condensed);
        }

        DescriptorType type;
        streamHandler.Process(type);

        VkImageLayout layout;
        streamHandler.Process(layout);

        if(sampler != nullptr){
            data_arena.AddElement(*sampler, view, type, layout);
        }
        else{
            data_arena.AddElement(view, type, layout);
        }
    }

    void Manager<DescriptorImageInfo>::Write(DescriptorImageInfo const& dii, std::string_view file_name){

        std::ofstream stream{file_name.data(), std::ios::binary};
        EWE_ASSERT(stream.is_open());

        Stream::Operator streamHandler{stream};

        std::size_t buffer = dii_version;
        streamHandler.Process(buffer);

        //this assumes that only the default view can be used
        //otherwise, i would need to store a handle for the view
        auto hash = views.GetHash(dii.view);
        streamHandler.Process(hash);

        uint8_t temp_buffer = dii.sampler != nullptr;
        streamHandler.Process(temp_buffer);
        if(dii.sampler != nullptr){
            uint64_t sample_condensed = Sampler::Condense(dii.sampler->info);
            streamHandler.Process(sample_condensed);
        }

        streamHandler.Process(dii.type);

        streamHandler.Process(dii.imageInfo.imageLayout);
    }



#ifdef EWE_IMGUI
    void Manager<DescriptorImageInfo>::Imgui(){
        filesystem.Imgui();
    }
#endif

} //namespace Asset
} //namespace EWE