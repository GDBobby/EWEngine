#pragma once

#include "EightWinds/Data/KeyValueContainer.h"
#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Backend/Semaphore.h"
#include "EightWinds/Backend/StagingBuffer.h"
#include "EightWinds/CommandPool.h"
#include "EightWinds/CommandBuffer.h"
#include "EightWinds/Backend/STC_Helper.h"

#include "EightWinds/Data/RuntimeArray.h"

#include "EWEngine/Data/RingBuffer.h"

#include <cassert>
#include <thread>
#include <mutex>
#include <array>
#include <vector>

/*
* 
    image process
    step 1{
    begin STC

    barrier - undefined to either TRANSFER_DST_OPTIMAL, or LAYOUT_GENERAL
    copy staging buffer to image

    barrier - the former transfer layout to the destination layout, potentially with a queue ownership transfer. skip if same queue and same layout
    
    if(async, transfer){
        end transfer STC
        begin graphics STC
        duplicate last barrier
    }
    if(generating mips){
        generate mips
    }
    end last STC

    on transfer STC ending {
        destroy staging buffer
        free command buffer
        reset command pool (unless i can get this to work per fiber)
    }

*/

namespace EWE{
    struct CommandBuffer;

    class STC_Manager{
    private:
        LogicalDevice& logicalDevice;
        Queue& renderQueue;
        Queue& computeQueue; //potentially same as renderQueue
        Queue& transferQueue; //potentially same as renderQueue

        Queue& GetQueue(Queue::Type type) {
            switch (type) {
                case Queue::Graphics: return renderQueue;
                case Queue::Compute: return computeQueue;
                case Queue::Transfer: return transferQueue;
            }
            EWE_UNREACHABLE;
        }

        RingBuffer<CommandPool, max_frames_in_flight * 2> renderCommandPools;
        //idk count, i want like 2 per fiber but i wont know fiber count at compile time
        //im not too familiar with timeline semaphores, idk if they can be used in two separate threads with separate values (b10 or b01 or b101 or whatever)

        std::array<std::mutex, Queue::COUNT> stc_mutexes;
        std::array<RingBuffer<CommandPool, 16>, Queue::COUNT> stc_command_pools;
        std::mutex semAcqMut{};
        RingBuffer<TimelineSemaphore, 32> semaphores; 

        struct SingleTimeCommand {
            Queue::Type queueType; //i dont think this is necessary (2/12)
            CommandPool* cmdPool;
            CommandBuffer cmdBuf;
            SingleTimeCommand(Queue::Type queueType, CommandPool* cmdPool) : queueType{ queueType }, cmdPool { cmdPool }, cmdBuf{ cmdPool->AllocateCommand(VK_COMMAND_BUFFER_LEVEL_PRIMARY) } {} //construct the commandbuffer here
            SingleTimeCommand(SingleTimeCommand const& copySrc) = delete;
            SingleTimeCommand& operator=(SingleTimeCommand const& copySrc) = delete;
            SingleTimeCommand(SingleTimeCommand&& moveSrc) noexcept;
            SingleTimeCommand& operator=(SingleTimeCommand&& moveSrc);
        };

        SingleTimeCommand* GetSTC(Queue::Type requested_queue) {
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
                case Queue::Graphics:
                    break;
            }
            return new SingleTimeCommand(actualQueueType, stc_command_pools[requested_queue].GetNext());
        }

    public:
        [[nodiscard]] explicit STC_Manager(LogicalDevice& logicalDevice, Queue& renderQueue);

        ~STC_Manager();
        TimelineSemaphore* GetSemaphore();
        void ReturnSemaphore(TimelineSemaphore* semaphore);

        bool CheckFencesForUsage();
        void CheckFencesForCallbacks();

        //this will ONLY work in a fiber
        void AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue::Type dstQueue);
        void AsyncTransferToCompute(std::function<void(CommandBuffer& cmdBuf)> transfer, std::function<void(CommandBuffer& cmdBuf)> compute);
    };
} //namespace EWE