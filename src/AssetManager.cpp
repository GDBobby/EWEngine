#include "EWEngine/Assets/Manager.h"


namespace EWE{
namespace Asset{

    AssetHash BufferHash(Buffer const& buffer){
        return CrossPlatformPathHash(buffer.name);
    }
    AssetHash ImageHash(Image const& image){
        return CrossPlatformPathHash(image.name);
    }
    AssetHash DIIHash(DescriptorImageInfo const& dii) {
        return CrossPlatformPathHash(dii.name);



    }
}//namespace Asset
} //namespace EWE