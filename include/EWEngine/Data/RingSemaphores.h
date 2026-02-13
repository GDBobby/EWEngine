#pragma once

#include "EightWinds/Data/StackBlock.h"

#include "EightWinds/Preprocessor.h"

#include <bitset>

//somewhat specialized
namespace EWE{
	template<typename T, std::size_t _Size>
	struct RingBuffer{
		using Size = _Size;
		
		StackBlock<T, Size> data;
		std::bitset<Size> usage;
		
		std::size_t starting_index = 0;
		
		T& GetNext(){
			std::size_t current_index = starting_index;
			while(usage[current_index]){
				current_index = (current_index + 1) % Size;
				if(current_index == starting_index){
					throw std::runtime_error("Ring Buffer out of space, all in use");
				}
			}
			starting_index = (current_index + 1) % Size;
			return data[current_index];
		}
		void Return(T& ref){
			for(std::size_t i = 0; i < Size; i++){
				if(&ref == &data[i]){
#if EWE_DEBUG
					assert(usage[i]);
#endif
					usage[i] = false;
					return;
				}
			}
			EWE_UNREACHABLE;
		}
	private:
		
		void UpdateIndex(){
			starting_index = (starting_index + 1) % Size;
		}
	};
}