#include "EWEngine/Assets/RenderAttachments.h"
#include "EWEngine/Global.h"
#include "EightWinds/Backend/RenderInfo.h"

#include "EWEngine/EWEngine.h"

namespace EWE{
namespace Asset{

	static constexpr std::size_t file_version = 0;

    void GenerateViewPair(FullRenderInfo& fri, int8_t index){
		PerFlight<Image*> image_con;
		PerFlight<ImageView*> view_con;
		for_each_frame{
			image_con[frame] = engine->assetManager.image.data_arena.GetCell();
			view_con[frame] = engine->assetManager.imageView.data_arena.GetCell();
		}
		fri.full.GenerateImage(
			image_con, view_con, 
			engine->window.screenDimensions.width, engine->window.screenDimensions.height, 
			index
		);
	}

	void WriteAttachmentInfoToFile(std::ofstream& outFile, AttachmentSetInfo const& info){
#define cast_write(data) reinterpret_cast<const char*>(&data), sizeof(data)
		outFile.write(cast_write(info.width));
		outFile.write(cast_write(info.height));
		outFile.write(cast_write(info.renderingFlags));

		outFile.write(cast_write(info.using_depth));
		outFile.write(cast_write(info.depth));
		uint8_t temp_size = static_cast<uint8_t>(info.colors.Size());
		outFile.write(cast_write(temp_size));
		outFile.write(reinterpret_cast<const char*>(info.colors.Data()), sizeof(AttachmentInfo) * temp_size);
#undef cast_write
	}

	void ReadAttachmentInfoFromFile(std::ifstream& inFile, AttachmentSetInfo& info){
#define cast_read(data) inFile.read(reinterpret_cast<char*>(&data), sizeof(data))

		cast_read(info.width);
		cast_read(info.height);
		cast_read(info.renderingFlags);

		cast_read(info.using_depth);
		cast_read(info.depth);
		uint8_t temp_size;
		cast_read(temp_size);
		info.colors.ClearAndResize(temp_size);
		inFile.read(reinterpret_cast<char*>(info.colors.Data()), sizeof(AttachmentInfo) * temp_size);

#undef cast_read
	}

	template<>
	bool WriteAssetToFile(FullRenderInfo const& fri, std::filesystem::path const& root_directory, std::filesystem::path const& path){
		auto const full_path = root_directory / path;
		std::ofstream outFile{full_path, std::ios::binary};
		if(!outFile.is_open()){
			Log::Debug("failed to open file : %s / %s\n", root_directory.string().c_str(), path.string().c_str());
			return false;
		}
		
		outFile.write(reinterpret_cast<const char*>(&file_version), sizeof(std::size_t));

		WriteAttachmentInfoToFile(outFile, fri.full.setInfo);

		uint8_t grt_size = fri.full.meta.Size();
		outFile.write(reinterpret_cast<const char*>(&grt_size), sizeof(uint8_t));
		for(auto& gr : fri.full.meta){
			AssetHash owner_hash = INVALID_HASH;
			if(gr.src_owner != nullptr){
				owner_hash = GetHash(*gr.src_owner);
			}
			outFile.write(reinterpret_cast<const char*>(&owner_hash), sizeof(AssetHash));
			outFile.write(reinterpret_cast<const char*>(&gr.src_index), sizeof(int8_t));
		}

		/*
		PerFlight<AssetHash> view_hash;
		for(auto& view : fri.full.color_views){
			for_each_frame{
				view_hash[frame] = GetHash(*view[frame]);
			}
			outFile.write(reinterpret_cast<const char*>(view_hash.buffer), sizeof(AssetHash) * 2);
		}
		if(fri.full.setInfo.using_depth){
			for_each_frame{
				view_hash[frame] = GetHash(*fri.full.depth_views[frame]);
			}
			outFile.write(reinterpret_cast<const char*>(view_hash.buffer), sizeof(AssetHash) * 2);
		}
		*/

		outFile.close();
		return true;
	}
	template<>
	bool LoadAssetFromFile(FullRenderInfo* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path){
		auto const full_path = root_directory / path;
		std::ifstream inFile{full_path, std::ios::binary};
		if(!inFile.is_open()){
			auto const full_path_string = full_path.string();
			Log::Debug("failed initial open attempt on file : %s\n", full_path_string.c_str());
			if(!std::filesystem::exists(full_path)){
				Log::Debug("attempting to open file that doesn't exist : %s\n", full_path_string.c_str());
				return false;
			}
			inFile.open(full_path, std::ios::binary);
			if(!inFile.is_open()){
				Log::Debug("failed explicit open : %s\n", full_path_string.c_str());
				return false;
			}
		}

		std::size_t version_buffer;
		inFile.read(reinterpret_cast<char*>(&version_buffer), sizeof(std::size_t));
		if(version_buffer != file_version){
			Log::Warning("invalid verison, not handled yet\n");
			return false;
		}

		AttachmentSetInfo setInfo;
		ReadAttachmentInfoFromFile(inFile, setInfo);

		uint8_t grt_size;
		inFile.read(reinterpret_cast<char*>(&grt_size), sizeof(uint8_t));

		std::vector<AttachmentMeta> attachmentMeta{};

		attachmentMeta.resize(grt_size);
		for(auto& gr : attachmentMeta){
			AssetHash owner_hash;
			inFile.read(reinterpret_cast<char*>(&owner_hash), sizeof(AssetHash));
			inFile.read(reinterpret_cast<char*>(&gr.src_index), sizeof(int8_t));
		}

		auto& fri = *std::construct_at(ptr_to_raw_mem, 
			path.string(), 
			engine->logicalDevice, 
			engine->renderQueue, 
			setInfo, 
			attachmentMeta,
			engine->window.screenDimensions.width, engine->window.screenDimensions.height
		);

		/*
		PerFlight<AssetHash> view_hash;
		for(std::size_t i = 0; i < setInfo.colors.Size(); i++) {
			inFile.read(reinterpret_cast<char*>(view_hash.buffer), sizeof(AssetHash) * 2);
			if(fri.full.color_views[i][0] == nullptr){
				if(view_hash[0] != INVALID_HASH){
					EWE_ASSERT(view_hash[1] != INVALID_HASH);
					for_each_frame{
						fri.full.color_views[i][frame] = engine->assetManager.imageView.Get(view_hash[frame]);
					}
				}
				else{
					//i dont know if this a valid configuration
					Log::Warning("unknown validity\n");
					GenerateViewPair(fri, i);
				}
				//get the hash
			}
		}
		if(fri.full.setInfo.using_depth){
			inFile.read(reinterpret_cast<char*>(view_hash.buffer), sizeof(AssetHash) * 2);
			if(fri.full.depth_views[0] == nullptr){
				if(view_hash[0] != INVALID_HASH){
					EWE_ASSERT(view_hash[1] != INVALID_HASH);
					for_each_frame{
						fri.full.depth_views[frame] = engine->assetManager.imageView.Get(view_hash[frame]);
					}
				}
				else{
					GenerateViewPair(fri, -1);
				}
			}
		}
		*/

		inFile.close();
		return true;
	}
} //namespace 
} //namespace EWE