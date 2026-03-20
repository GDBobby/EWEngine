#pragma once

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Backend/Semaphore.h"
#include "EightWinds/Backend/StagingBuffer.h"
#include "EightWinds/CommandPool.h"
#include "EightWinds/CommandBuffer.h"
#include "EightWinds/Backend/STC_Helper.h"

#include "EWEngine/Data/RingBuffer.h"

#include <mutex>
#include <array>

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
    public:
        Queue& renderQueue;
        Queue& computeQueue; //potentially same as renderQueue
        Queue& transferQueue; //potentially same as renderQueue

        Queue& GetQueue(Queue::Type type) {
            switch (type) {
                case Queue::Graphics: return renderQueue;
                case Queue::Compute: return computeQueue;
                case Queue::Transfer: return transferQueue;
                default: EWE_UNREACHABLE;
            }
            EWE_UNREACHABLE;
        }
    private:
        LogicalDevice& logicalDevice;

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

        SingleTimeCommand* GetSTC(Queue::Type requested_queue);

    public:

        Queue::Type GetQueueType(Queue& queue) const;

        [[nodiscard]] explicit STC_Manager(LogicalDevice& logicalDevice, Queue& renderQueue);
        ~STC_Manager();

        STC_Manager(STC_Manager const& copySrc) = delete;
        STC_Manager(STC_Manager&& moveSrc) = delete;
        STC_Manager& operator=(STC_Manager const& copySrc) = delete;
        STC_Manager& operator=(STC_Manager&& moveSrc) = delete;

        TimelineSemaphore* GetSemaphore();
        void ReturnSemaphore(TimelineSemaphore* semaphore);

        bool CheckFencesForUsage();
        void CheckFencesForCallbacks();

        //this will ONLY work in a fiber
        void AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue& rh_queue);
        void AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue::Type dstQueue);
        void AsyncTransferToCompute(std::function<void(CommandBuffer& cmdBuf)> transfer, std::function<void(CommandBuffer& cmdBuf)> compute);
    };
} //namespace EWE