#include "EWEngine/Assets/RenderAttachments.h"
#include "EWEngine/Global.h"
#include "EightWinds/Backend/RenderInfo.h"

namespace EWE{
namespace Asset{


    void GenerateViewPair(FullRenderInfo& fri, int8_t index){
		PerFlight<Image*> image_con;
		PerFlight<ImageView*> view_con;
		for_each_frame{
			image_con[frame] = Global::assetManager->image.data_arena.GetCell();
			view_con[frame] = Global::assetManager->imageView.data_arena.GetCell();
		}
		fri.full.GenerateImage(
			image_con, view_con, 
			Global::window->screenDimensions.width, Global::window->screenDimensions.height, 
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
			Logger::Print("failed to open file : %s / %s\n", root_directory.string().c_str(), path.string().c_str());
			return false;
		}

		WriteAttachmentInfoToFile(outFile, fri.full.setInfo);

		uint8_t grt_size = fri.full.generated_reference_tracker.size();
		outFile.write(reinterpret_cast<const char*>(&grt_size), sizeof(uint8_t));
		for(auto& gr : fri.full.generated_reference_tracker){
			AssetHash owner_hash = INVALID_HASH;
			if(gr.source_owner != nullptr){
				owner_hash = GetHash(*gr.source_owner);
			}
			outFile.write(reinterpret_cast<const char*>(&owner_hash), sizeof(AssetHash));
			outFile.write(reinterpret_cast<const char*>(&gr.src_index), sizeof(int8_t));
			outFile.write(reinterpret_cast<const char*>(&gr.dst_index), sizeof(int8_t));
		}

		PerFlight<AssetHash> view_hash;
		for(auto& view : fri.full.color_views){
			for_each_frame{
				view_hash[frame] = GetHash(*view[frame]);
			}
			outFile.write(reinterpret_cast<const char*>(view_hash.buffer), sizeof(AssetHash) * 2);
			/*
			for_each_frame{
				bool generated = view_mgr.association_container.find(view_hash) == view_mgr.association_container.end();
				outFile.write(reinterpret_cast<const char*>(&generated), sizeof(bool)); //1 byte???
				if(generated){
					static std::size_t att_offset = offsetof(FullRenderInfo, full);
					std::size_t original_addr = reinterpret_cast<std::size_t>(view[frame]->image.name.string().data()) - att_offset;
					FullRenderInfo* original_owner = reinterpret_cast<FullRenderInfo*>(original_addr);
					const AssetHash owner_hash = Global::assetManager->attachment_info.Get(*original_owner);
					outFile.write(reinterpret_cast<const char*>(&owner_hash), sizeof(AssetHash));

					int8_t index = reintepret_cast<int8_t>(view[frame]->image.name[sizeof(std::size_t)]);
					outFile.write(reinterpret_cast<const char*>(&index), sizeof(int8_t));
					//can I guarantee its always on the same frame? probably
					uint8_t image_frame = reintepret_cast<uint8_t>(view[frame]->image.name[sizeof(std::size_t) + 1]);
					outFile.write(reinterpret_cast<const char*>(&image_frame), sizeof(uint8_t));
				}
				else{
					outFile.write(reinterpret_cast<const char*>(&view_hash), sizeof(AssetHash));
				}
			}
			*/
		}
		if(fri.full.setInfo.using_depth){
			for_each_frame{
				view_hash[frame] = GetHash(*fri.full.depth_views[frame]);
			}
			outFile.write(reinterpret_cast<const char*>(view_hash.buffer), sizeof(AssetHash) * 2);
			/*
			for_each_frame{ 
				bool generated = view_mgr.association_container.find(view_hash) == view_mgr.association_container.end();
				outFile.write(reinterpret_cast<const char*>(&generated), sizeof(bool)); //1 byte???
				if(generated){
					auto& view = fri.full.depth_views;
					static std::size_t att_offset = offsetof(FullRenderInfo, full);
					std::size_t original_addr = reintepret_cast<std::size_t>(view[frame]->image.name.data()) - att_offset;
					FullRenderInfo* original_owner = reinterpret_cast<FullRenderInfo*>(original_addr);
					const AssetHash owner_hash = Global::assetManager->attachment_info.Get(*original_owner);
					outFile.write(reinterpret_cast<const char*>(&owner_hash), sizeof(AssetHash));

					int8_t index = reintepret_cast<int8_t>(view[frame]->image.name[sizeof(std::size_t)]);
					outFile.write(reinterpret_cast<const char*>(&index), sizeof(int8_t));
					//can I guarantee its always on the same frame? probably
					uint8_t image_frame = reintepret_cast<uint8_t>(view[frame]->image.name[sizeof(std::size_t) + 1]);
					outFile.write(reinterpret_cast<const char*>(&image_frame), sizeof(uint8_t));
				}
				else{
					outFile.write(reinterpret_cast<const char*>(&view_hash), sizeof(AssetHash));
				}
			}
			*/
			
		}

		outFile.close();
		return true;
	}
	template<>
	bool LoadAssetFromFile(FullRenderInfo* ptr_to_raw_mem, std::filesystem::path const& root_directory, std::filesystem::path const& path){
		auto const full_path = root_directory / path;
		std::ifstream inFile{full_path, std::ios::binary};
		if(!inFile.is_open()){
			auto const full_path_string = full_path.string();
			Logger::Print("failed initial open attempt on file : %s\n", full_path_string.c_str());
			if(!std::filesystem::exists(full_path)){
				Logger::Print("attempting to open file that doesn't exist : %s\n", full_path_string.c_str());
				return false;
			}
			inFile.open(full_path, std::ios::binary);
			if(!inFile.is_open()){
				Logger::Print("failed explicit open : %s\n", full_path_string.c_str());
				return false;
			}
		}

		AttachmentSetInfo setInfo;
		ReadAttachmentInfoFromFile(inFile, setInfo);

		auto& fri = *std::construct_at(ptr_to_raw_mem, path.string(), *Global::logicalDevice, Global::stcManager->renderQueue, setInfo);

		uint8_t grt_size;
		inFile.read(reinterpret_cast<char*>(&grt_size), sizeof(uint8_t));
		fri.full.generated_reference_tracker.resize(grt_size);
		for(auto& gr : fri.full.generated_reference_tracker){
			AssetHash owner_hash;
			inFile.read(reinterpret_cast<char*>(&owner_hash), sizeof(AssetHash));
			inFile.read(reinterpret_cast<char*>(&gr.src_index), sizeof(int8_t));
			inFile.read(reinterpret_cast<char*>(&gr.dst_index), sizeof(int8_t));
		
			auto GetDst = [&]() -> PerFlight<ImageView*>&{
				if(gr.dst_index >= 0){ return fri.full.color_views[gr.dst_index]; }
				else { return fri.full.depth_views; }
			};
			if(owner_hash != INVALID_HASH) {
				auto GetSrc = [&]() -> PerFlight<ImageView*>{
					if(gr.src_index >= 0){ return gr.source_owner->full.color_views[gr.src_index]; }
					else { return gr.source_owner->full.depth_views;}
				};
				gr.source_owner = Global::assetManager->attachment_info.Get(owner_hash);
				EWE_ASSERT(gr.source_owner != nullptr);

				GetDst() = GetSrc();
			}
			else{
				//generate the image now? or let it stay nullptr?
				GenerateViewPair(fri, gr.dst_index);
			}
		}

		PerFlight<AssetHash> view_hash;
		for(std::size_t i = 0; i < setInfo.colors.Size(); i++) {
			inFile.read(reinterpret_cast<char*>(view_hash.buffer), sizeof(AssetHash) * 2);
			if(fri.full.color_views[i][0] == nullptr){
				if(view_hash[0] != INVALID_HASH){
					EWE_ASSERT(view_hash[1] != INVALID_HASH);
					for_each_frame{
						fri.full.color_views[i][frame] = Global::assetManager->imageView.Get(view_hash[frame]);
					}
				}
				else{
					//i dont know if this a valid configuration
					Logger::Print<Logger::Warning>("unknown validity\n");
					GenerateViewPair(fri, i);
				}
				//get the hash
			}
			/*
			for_each_frame{
				bool generated;
				inFile.read(reinterpret_cast<char*>(&generated), sizeof(bool));
				if(generated){
					AssetHash owner_hash;
					inFile.read(reinterpret_cast<char*>(&owner_hash), sizeof(AssetHash));
					int8_t index;
					uint8_t image_frame;
					inFile.read(reinterpret_cast<char*>(index), sizeof(int8_t));
					inFile.read(reinterpret_cast<char8>(image_frame), sizeof(uint8_t));

					auto* owner_fri = Global::assetManager->attachment_info.Get(owner_hash);
					if(index >= 0){
						fri.full.color_views[i][frame] = owner_fri->full.color_views[index][image_frame];
					}
					else{
						fri.full.color_views[i][frame] = owner_fri->full.depth_views[image_frame];
					}
				}
				else{
					inFile.read(reinterpret_cast<char*>(&view_hash), sizeof(AssetHash));
					fri.full.color_views[i][frame] = Global::assetManager->imageView.Get(view_hash);
				}
			}
			*/
		}
		if(fri.full.setInfo.using_depth){
			inFile.read(reinterpret_cast<char*>(view_hash.buffer), sizeof(AssetHash) * 2);
			if(fri.full.depth_views[0] == nullptr){
				if(view_hash[0] != INVALID_HASH){
					EWE_ASSERT(view_hash[1] != INVALID_HASH);
					for_each_frame{
						fri.full.depth_views[frame] = Global::assetManager->imageView.Get(view_hash[frame]);
					}
				}
				else{
					GenerateViewPair(fri, -1);
				}
			}
		}

		inFile.close();
		return true;
	}
} //namespace 
} //namespace EWE