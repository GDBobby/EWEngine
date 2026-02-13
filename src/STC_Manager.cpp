#include "EWEngine/STC_Manager.h"

#include "EightWinds/RenderGraph/Resources.h"
#include "EightWinds/Backend/PipelineBarrier.h"

#include "EightWinds/Image.h"

#include "EWEngine/Global.h"


#include "marl/event.h"

namespace EWE {

	Queue& GetComputeQueue(LogicalDevice& logicalDevice, Queue& renderQueue) {
		for (std::size_t i = 0; i < logicalDevice.queues.Size(); i++) {
			if (logicalDevice.queues[i].family.SupportsCompute() && (logicalDevice.queues[i] != renderQueue)) {
				return logicalDevice.queues[i];
			}
		}
		EWE_ASSERT(renderQueue.family.SupportsTransfer());
		return renderQueue;
	}
	Queue& GetTransferQueue(LogicalDevice& logicalDevice, Queue& renderQueue, Queue& computeQueue) {
		for (std::size_t i = 0; i < logicalDevice.queues.Size(); i++) {
			if (logicalDevice.queues[i].family.SupportsTransfer() && (logicalDevice.queues[i] != computeQueue) && (logicalDevice.queues[i] != renderQueue)) {
				return logicalDevice.queues[i];
			}
		}
		if (computeQueue.family.SupportsTransfer()) {
			return computeQueue;
		}
		EWE_ASSERT(renderQueue.family.SupportsTransfer());
		return renderQueue;
	}

    STC_Manager::STC_Manager(LogicalDevice& logicalDevice, Queue& renderQueue)
    : logicalDevice{logicalDevice},
		renderQueue{ renderQueue }, computeQueue{ GetComputeQueue(logicalDevice, renderQueue)}, transferQueue{GetTransferQueue(logicalDevice, renderQueue, computeQueue)},
        renderCommandPools{logicalDevice, renderQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT },
        stc_mutexes{},
        stc_command_pools{
            CommandPool{logicalDevice, renderQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT},
            CommandPool{logicalDevice, computeQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT},
            CommandPool{logicalDevice, transferQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT},
        },
        semAcqMut{},
        semaphores{logicalDevice}

        //cmdBufs{}
    {
    }
    STC_Manager::~STC_Manager() {
    }
    
    //fiber focused
    void STC_Manager::AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue::Type dstQueue){

        VkImageLayout firstLayout = transferContext.generatingMipMaps ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        Queue& lh_queue = transferQueue;
        Queue& rh_queue = GetQueue(dstQueue);
		bool async_transfer = lh_queue != rh_queue;

        SingleTimeCommand* first_stc = GetSTC(Queue::Transfer);


		UsageData<Image> usage{
			.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		};
		if (transferContext.resource.usage.layout == VK_IMAGE_LAYOUT_GENERAL) {
			usage.layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		Resource<Image> lh_resource{ *transferContext.resource.resource[0], usage};

		{ //initial transition from UNDEFINED to either TRANSFER_DST_OPTIMAL or GENERAL
			auto initial_transition_barrier = Barrier::Acquire_Image(lh_queue, transferContext.resource, 0);
			VkDependencyInfo dependency_info{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.memoryBarrierCount = 0,
				.bufferMemoryBarrierCount = 0,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &initial_transition_barrier
			};
			vkCmdPipelineBarrier2(first_stc->cmdBuf, &dependency_info);
		}

		Command_Helper::CopyBufferToImage(first_stc->cmdBuf, transferContext.stagingBuffer->buffer, *transferContext.resource.resource[0], transferContext.image_region, usage.layout);

		SingleTimeCommand* current_stc = first_stc;
		TimelineSemaphore* first_sem = semaphores.GetNext();
		TimelineSemaphore* current_sem = first_sem;

		VkCommandBufferSubmitInfo cmdInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.pNext = nullptr,
			.commandBuffer = current_stc->cmdBuf,
			.deviceMask = 0
		};
		VkSemaphoreSubmitInfo semInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.pNext = nullptr,
			.semaphore = first_sem->vkSemaphore,
			.value = 1,
			.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT
		};

		VkSubmitInfo2 submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			.waitSemaphoreInfoCount = 0,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &semInfo
		};

		if ((lh_queue.FamilyIndex() != rh_queue.FamilyIndex()) || (usage.layout != transferContext.resource.usage.layout)) { //ownership and potentially layout transition
			auto ownershipBarrier = Barrier::Transition_Image(lh_queue, lh_resource, rh_queue, transferContext.resource, 0);

			VkDependencyInfo dependency_info{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.memoryBarrierCount = 0,
				.bufferMemoryBarrierCount = 0,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &ownershipBarrier
			};
			vkCmdPipelineBarrier2(first_stc->cmdBuf, &dependency_info);

			if (async_transfer) {
				first_stc->cmdBuf.End();
				first_stc->cmdPool->queue.Submit2(1, &submitInfo, VK_NULL_HANDLE);
				current_stc = GetSTC(Queue::Graphics);

				vkCmdPipelineBarrier2(current_stc->cmdBuf, &dependency_info);
			}
		}
		if (transferContext.generatingMipMaps) {
			EWE_ASSERT(current_stc->cmdPool->queue.family.SupportsGraphics());
		}

		current_stc->cmdBuf.End();
		cmdInfo.commandBuffer = current_stc->cmdBuf;
		semInfo.semaphore = current_sem->vkSemaphore;
		current_stc->cmdPool->queue.Submit2(submitInfo, VK_NULL_HANDLE);

		if (async_transfer) {
			first_sem->WaitOn(1);
			while (!first_sem->Check(1)) {
				marl::Event event{ marl::Event::Mode::Manual };
				event.wait_for(std::chrono::microseconds(1)); //the poitn is to just relinquish control, we don't want to wait for a long time
			}
			if (transferContext.stagingBuffer) {
				transferContext.stagingBuffer->Free();
				delete transferContext.stagingBuffer;
			}
			stc_command_pools[Queue::Transfer].Return(first_stc->cmdPool);
			delete first_stc;
			EWE_ASSERT(first_sem != current_sem);
			while (!current_sem->Check(1)) {
				marl::Event event{ marl::Event::Mode::Manual };
				event.wait_for(std::chrono::microseconds(1)); //the poitn is to just relinquish control, we don't want to wait for a long time
			}
			transferContext.resource.resource[0]->readyForUsage = true;
			transferContext.resource.resource[0]->layout = transferContext.resource.usage.layout;
			stc_command_pools[dstQueue].Return(current_stc->cmdPool);
			delete current_stc;
		}
		else {
			EWE_ASSERT(first_sem == current_sem);
			while (!current_sem->Check(1)) {
				marl::Event event{ marl::Event::Mode::Manual };
				event.wait_for(std::chrono::microseconds(1)); //the poitn is to just relinquish control, we don't want to wait for a long time
			}
			transferContext.resource.resource[0]->readyForUsage = true;
			transferContext.resource.resource[0]->layout = transferContext.resource.usage.layout;
			stc_command_pools[Queue::Transfer].Return(current_stc->cmdPool);
			delete current_stc;
		}
    }

}//namespace EWE