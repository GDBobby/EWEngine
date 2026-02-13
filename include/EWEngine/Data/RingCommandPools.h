#pragma once

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Data/RuntimeArray.h"
#include "EightWinds/LogicalDevice.h"
#include "EightWinds/CommandPool.h"
#include "EightWinds/CommandBuffer.h"

namespace EWE{
	template<std::size_t _Size>
	struct RingCommandPools {
		using Size = _Size;

		RuntimeArray<CommandPool> commandPools; //probably 2, just so i have 1 ready to go as soon as the frame starts
		RuntimeArray<bool> usage;
		const std::size_t size;
		[[nodiscard]] explicit RingCommandPools(LogicalDevice& logicalDevice, Queue& queue, VkCommandPoolCreateFlags poolFlags, std::size_t size)
			: commandPools{size, logicalDevice, queue, poolFlags },
			usage{size, false},
			size{size}
		{

		}

		uint8_t currentIndex = 0;

		CommandBuffer GetCommandBuffer() {
			uint8_t temp_index = currentIndex;
			currentIndex = (currentIndex + 1) % size;
			return commandPools[temp_index].AllocateCommand(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		}

		void FreeBuffer(CommandBuffer& commandBuffer) {
			for (std::size_t i = 0; i < size; i++) {
				if (commandPools[i] == commandBuffer.commandPool) {
#if EWE_DEBUG
					assert(usage[i]);
#endif
					usage[i] = false;
					pool.Reset(0);
					return;
				}
			}
			EWE_UNREACHABLE;
		}
	};
} //namespace EWE