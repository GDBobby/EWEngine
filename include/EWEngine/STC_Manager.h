#pragma once

#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Backend/Semaphore.h"
#include "EightWinds/Backend/StagingBuffer.h"
#include "EightWinds/CommandPool.h"
#include "EightWinds/CommandBuffer.h"
#include "EightWinds/Backend/STC_Helper.h"
#include "EightWinds/Backend/SingleTimeCommand.h"

#include "EWEngine/Data/RingBuffer.h"

#include <mutex>
#include <array>

namespace EWE{
    struct CommandBuffer;
    struct RenderGraph;

    class STC_Manager{
    private:
        LogicalDevice& logicalDevice;
        RenderGraph& renderGraph;
    public:
        Queue& renderQueue;
        Queue& computeQueue; //potentially same as renderQueue
        Queue& transferQueue; //potentially same as renderQueue

        Queue& GetQueue(Queue::Type type);
    private:

        RingBuffer<CommandPool, max_frames_in_flight * 2> renderCommandPools;
        //idk count, i want like 2 per fiber but i wont know fiber count at compile time
        //im not too familiar with timeline semaphores, idk if they can be used in two separate threads with separate values (b10 or b01 or b101 or whatever)

        std::array<std::mutex, Queue::COUNT> stc_mutexes;
        std::array<RingBuffer<CommandPool, 16>, Queue::COUNT> stc_command_pools;
        std::mutex semAcqMut{};
        RingBuffer<TimelineSemaphore, 32> semaphores; 

        SingleTimeCommand* GetSTC(Queue::Type requested_queue);

    public:

        Queue::Type GetQueueType(Queue& queue) const;

        [[nodiscard]] explicit STC_Manager(LogicalDevice& logicalDevice, Queue& renderQueue, RenderGraph& _renderGraph);
        ~STC_Manager();

        STC_Manager(STC_Manager const& copySrc) = delete;
        STC_Manager(STC_Manager&& moveSrc) = delete;
        STC_Manager& operator=(STC_Manager const& copySrc) = delete;
        STC_Manager& operator=(STC_Manager&& moveSrc) = delete;

        TimelineSemaphore* GetSemaphore();
        void ReturnSemaphore(TimelineSemaphore* semaphore);

        bool CheckFencesForUsage();
        void CheckFencesForCallbacks();

        void SingleQueueTransfer(AsyncTransferContext_Image& transferContext, Queue::Type dstQueueType);
        void AsyncTransfer_Helper(AsyncTransferContext_Image& transferContext, Queue::Type dstQueueType);
        //this will ONLY work in a fiber
        void AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue& rh_queue);
        void AsyncTransfer(AsyncTransferContext_Image& transferContext, Queue::Type dstQueue);
        void AsyncTransferToCompute(std::function<void(CommandBuffer& cmdBuf)> transfer, std::function<void(CommandBuffer& cmdBuf)> compute);
    };
} //namespace EWE