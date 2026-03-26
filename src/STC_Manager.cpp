#include "EWEngine/STC_Manager.h"

#include "EightWinds/RenderGraph/Resources.h"
#include "EightWinds/Backend/PipelineBarrier.h"

#include "EightWinds/Image.h"

#include "EWEngine/Global.h"

#include "EightWinds/RenderGraph/RenderGraph.h"

#include "marl/event.h"
#include <chrono>

#include "EightWinds/Reflect/Enum.h"

namespace EWE {

	Queue::Type STC_Manager::GetQueueType(Queue& queue) const{
		if(queue == renderQueue){
			return Queue::Graphics;
		}
		else if (queue == computeQueue){
			return Queue::Compute;
		}
		else if (queue == transferQueue){
			return Queue::Transfer;
		}
		EWE_UNREACHABLE;
	}

	Queue& STC_Manager::GetQueue(Queue::Type type) {
		switch (type) {
			case Queue::Graphics: return renderQueue;
			case Queue::Compute: return computeQueue;
			case Queue::Transfer: return transferQueue;
			default: EWE_UNREACHABLE;
		}
		EWE_UNREACHABLE;
	}

	SingleTimeCommand* STC_Manager::GetSTC(Queue::Type requested_queue) {
		Queue::Type actualQueueType = requested_queue;
		switch (requested_queue) {
			case Queue::Transfer:
				if (transferQueue == renderQueue) {
					actualQueueType = Queue::Graphics;
				}
				else if (transferQueue == computeQueue) {
					//if the compute queue is also the transfer queue, it will behave the same as tranfer->render
					//if the destination queue is compute, it will be considered a 1 queue transfer
					actualQueueType = Queue::Compute; 
				}
				break;
			case Queue::Compute:
				if (computeQueue == renderQueue) {
					actualQueueType = Queue::Graphics;
				}
				break;
			default: break;
		}
		return new SingleTimeCommand(stc_command_pools[requested_queue].GetNext(), semaphores.GetNext());
	}

    STC_Manager::STC_Manager(LogicalDevice& _logicalDevice, Queue& _transferQueue, RenderGraph& _renderGraph)
    : logicalDevice{_logicalDevice},
		renderGraph{_renderGraph},
		renderQueue{ _renderGraph.renderQueue }, computeQueue{ _renderGraph.computeQueue}, transferQueue{_transferQueue},
        renderCommandPools{logicalDevice, renderQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT },
        stc_mutexes{},
        stc_command_pools{
            RingBuffer<CommandPool, 16>{logicalDevice, renderQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT},
            RingBuffer<CommandPool, 16>{logicalDevice, computeQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT},
            RingBuffer<CommandPool, 16>{logicalDevice, transferQueue, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT},
        },
		semaphores{logicalDevice}
    {
#if EWE_DEBUG_NAMING
		renderQueue.SetName("render queue");
		if(computeQueue == transferQueue){
			computeQueue.SetName("C&T queue, compute handle");
			transferQueue.SetName("C&T queue, transfer handle");
		}
		else{
			computeQueue.SetName("compute queue");
			transferQueue.SetName("transfer queue");
		}
#endif
    }
    STC_Manager::~STC_Manager() {
		
    }
    
    void STC_Manager::AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue& rh_queue){
		AsyncTransfer(transferContext, GetQueueType(rh_queue));
	}

	void STC_Manager::AsyncTransfer_Helper(AsyncTransferContext_Image& transferContext, Queue::Type dstQueueType){

        Queue& rh_queue = GetQueue(dstQueueType);


		VkCommandBufferBeginInfo const cmdBeginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr
		};

        SingleTimeCommand* first_stc = GetSTC(Queue::Transfer);
		first_stc->cmdBuf.Begin(cmdBeginInfo);

        VkImageLayout initial_layout = (transferContext.generatingMipMaps || dstQueueType == Queue::Type::Compute) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		UsageData<Image> initial_usage{
			.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.layout = initial_layout
		};
		if (transferContext.resource.usage.layout == VK_IMAGE_LAYOUT_GENERAL) {
			initial_usage.layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		Resource<Image> initial_resource{ *transferContext.resource.resource[0], initial_usage};

		{ //initial transition from UNDEFINED to either TRANSFER_DST_OPTIMAL or GENERAL
			auto initial_transition_barrier = Barrier::Acquire_Image(transferQueue, initial_resource, 0);
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


		Command_Helper::CopyBufferToImage(first_stc->cmdBuf, transferContext.stagingBuffer->buffer, *transferContext.resource.resource[0], transferContext.image_region, initial_usage.layout);

		TimelineSemaphore* first_sem = semaphores.GetNext();
#if EWE_DEBUG_BOOL
		first_sem->debugName = "STC first";
#endif

		VkCommandBufferSubmitInfo cmdInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.pNext = nullptr,
			.commandBuffer = first_stc->cmdBuf,
			.deviceMask = 0
		};

		VkSemaphoreSubmitInfo semInfo = first_sem->GetSignalSubmitInfo(VK_PIPELINE_STAGE_2_TRANSFER_BIT);

		VkSubmitInfo2 submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			.waitSemaphoreInfoCount = 0,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &semInfo
		};

		auto ownershipBarrier = Barrier::Transition_Image(transferQueue, initial_resource, rh_queue, transferContext.resource, 0);

		VkDependencyInfo dependency_info{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.memoryBarrierCount = 0,
			.bufferMemoryBarrierCount = 0,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &ownershipBarrier
		};
		vkCmdPipelineBarrier2(first_stc->cmdBuf, &dependency_info);

		first_stc->cmdBuf.End();
		first_stc->cmdPool->queue.Submit2(1, &submitInfo, VK_NULL_HANDLE);

		//auto ownership_stc = GetSTC(dstQueueType);

		/*
		UsageData<Image> finalUsage{
			.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, //before anything else gets done?
			.accessMask = VK_ACCESS_2_SHADER_READ_BIT,
			.layout = transferContext.resource.usage.layout
		};
		*/
		STCManagement::Helper<Image> stc_helper{
			.barrier = ownershipBarrier,
			.res = transferContext.resource,
			.dstQueue = &rh_queue
		};
		renderGraph.ResourceOwnershipTransfer(stc_helper);

		//ownership_stc = GetSTC(dstQueueType);
		//cmdInfo.commandBuffer = ownership_stc->cmdBuf;
#if EWE_DEBUG_BOOL
		//ownership_stc->semaphore.debugName = std::string("STC ") + Reflect::Enum::ToString(dstQueueType);
#endif
		//semInfo = ownership_stc->semaphore.GetSignalSubmitInfo(VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		//ownership_stc->cmdBuf.Begin(cmdBeginInfo);

		//vkCmdPipelineBarrier2(ownership_stc->cmdBuf, &dependency_info);
		

		//ownership_stc->cmdBuf.End();
		//ownership_stc->cmdPool->queue.Submit2(submitInfo, VK_NULL_HANDLE);

		while (!first_sem->Check(first_sem->value)) {
			marl::Event event{ marl::Event::Mode::Auto };
			event.wait_for(std::chrono::microseconds(1)); //the poitn is to just relinquish control, we don't want to wait for a long time
			//std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
		first_stc->cmdBuf.state = CommandBuffer::State::Invalid;
		if (transferContext.stagingBuffer) {
			transferContext.stagingBuffer->Free();
			delete transferContext.stagingBuffer;
		}
		stc_command_pools[Queue::Transfer].Return(first_stc->cmdPool);
		delete first_stc;
		/*
		the rendergraph is gonna handle this

		while (!ownership_stc->semaphore.Check(ownership_stc->semaphore.value)) {
			marl::Event event{ marl::Event::Mode::Manual };
			event.wait_for(std::chrono::microseconds(1)); //the poitn is to just relinquish control, we don't want to wait for a long time
			ownership_stc->cmdBuf.state = CommandBuffer::State::Invalid;
		}
		ownership_stc->cmdBuf.state = CommandBuffer::State::Invalid;
		transferContext.resource.resource[0]->readyForUsage = true;
		transferContext.resource.resource[0]->data.layout = transferContext.resource.usage.layout;
		stc_command_pools[dstQueueType].Return(ownership_stc->cmdPool);
		delete ownership_stc;
		*/
	}

	void STC_Manager::SingleQueueTransfer(AsyncTransferContext_Image& transferContext, Queue::Type dstQueueType){

		Queue& queue = GetQueue(dstQueueType);

		VkCommandBufferBeginInfo const cmdBeginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr
		};

        SingleTimeCommand* first_stc = GetSTC(Queue::Transfer);
		first_stc->cmdBuf.Begin(cmdBeginInfo);

        VkImageLayout initial_layout = (transferContext.generatingMipMaps || dstQueueType == Queue::Type::Compute) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		UsageData<Image> initial_usage{
			.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.layout = initial_layout
		};
		if (transferContext.resource.usage.layout == VK_IMAGE_LAYOUT_GENERAL) {
			initial_usage.layout = VK_IMAGE_LAYOUT_GENERAL;
		}
		Resource<Image> initial_resource{ *transferContext.resource.resource[0], initial_usage};

		{ //initial transition from UNDEFINED to either TRANSFER_DST_OPTIMAL or GENERAL
			auto initial_transition_barrier = Barrier::Acquire_Image(queue, initial_resource, 0);
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


		Command_Helper::CopyBufferToImage(first_stc->cmdBuf, transferContext.stagingBuffer->buffer, *transferContext.resource.resource[0], transferContext.image_region, initial_usage.layout);

		semAcqMut.lock();
		TimelineSemaphore* first_sem = semaphores.GetNext();
		semAcqMut.unlock();
#if EWE_DEBUG_BOOL
		first_sem->debugName = "STC first";
#endif

		VkCommandBufferSubmitInfo cmdInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.pNext = nullptr,
			.commandBuffer = first_stc->cmdBuf,
			.deviceMask = 0
		};

		VkSemaphoreSubmitInfo semInfo = first_sem->GetSignalSubmitInfo(VK_PIPELINE_STAGE_2_TRANSFER_BIT);

		VkSubmitInfo2 submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			.waitSemaphoreInfoCount = 0,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &semInfo
		};


		SingleTimeCommand* current_stc = first_stc;
		TimelineSemaphore* current_sem = first_sem;

		if (initial_usage.layout != transferContext.resource.usage.layout) { //ownership and potentially layout transition
			auto ownershipBarrier = Barrier::Transition_Image(queue, initial_resource, queue, transferContext.resource, 0);

			VkDependencyInfo dependency_info{
				.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext = nullptr,
				.memoryBarrierCount = 0,
				.bufferMemoryBarrierCount = 0,
				.imageMemoryBarrierCount = 1,
				.pImageMemoryBarriers = &ownershipBarrier
			};
			vkCmdPipelineBarrier2(first_stc->cmdBuf, &dependency_info);

		}
		if (transferContext.generatingMipMaps) {
			EWE_ASSERT(current_stc->cmdPool->queue.family.SupportsGraphics());
		}
		
		current_stc->cmdBuf.End();
		current_stc->cmdPool->queue.Submit2(submitInfo, VK_NULL_HANDLE);

		EWE_ASSERT(first_sem == current_sem);
		while (!current_sem->Check(1)) {
			marl::Event event{ marl::Event::Mode::Manual };
			event.wait_for(std::chrono::microseconds(1)); //the poitn is to just relinquish control, we don't want to wait for a long time
		}
		transferContext.resource.resource[0]->readyForUsage = true;
		transferContext.resource.resource[0]->data.layout = transferContext.resource.usage.layout;
		stc_command_pools[Queue::Transfer].Return(current_stc->cmdPool);
		delete current_stc;
		
	}

    //fiber focused
    void STC_Manager::AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue::Type dstQueueType){

		EWE_ASSERT(!CheckMainThread());
		std::string thread_name = std::string("AT:") + transferContext.resource.resource[0]->name;
		NameCurrentThread(thread_name);
        Queue& rh_queue = GetQueue(dstQueueType);
		if (transferContext.generatingMipMaps) {
			EWE_ASSERT(rh_queue.family.SupportsGraphics());
		}

		//when im ready for signle queue transfers i can fix this up
        Queue& lh_queue = transferQueue;

		if(lh_queue != rh_queue){
			AsyncTransfer_Helper(transferContext, dstQueueType);
		}
		else{
			SingleQueueTransfer(transferContext, dstQueueType);
		}
    }

}//namespace EWE