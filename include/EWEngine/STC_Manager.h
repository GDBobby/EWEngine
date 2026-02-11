#pragma once

#include "EightWinds/Data/KeyValueContainer.h"
#include "EightWinds/VulkanHeader.h"
#include "EightWinds/Backend/Semaphore.h"
#include "EightWinds/Backend/StagingBuffer.h"
#include "EightWinds/CommandPool.h"
#include "EightWinds/CommandBuffer.h"

#include "EightWinds/Data/RuntimeArray.h"

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

    class QueueSyncPool{
    private:
        const uint16_t size;

        RuntimeArray<TimelineSemaphore> semaphores;
        struct RotationalCommandPools { //need a circular buffer

            const std::size_t 
            RuntimeArray<CommandPool> commandPool; //probably 2, just so i have 1 ready to go as soon as the frame starts
            uint8_t currentIndex = 0;

            CommandBuffer GetCommandBuffer() {
                uint8_t temp_index = currentIndex;
                currentIndex = (currentIndex + 1) % pools_per_frame_in_flight;
                return commandPool[temp_index].AllocateCommand(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            }
        };

        std::mutex semAcqMut{};
        Queue& renderQueue;
        Queue& transferQueue; //potentially same as renderQueue
        Queue& computeQueue; //potentially same as renderQueue
        //potentially async compute queue
        
        //this will be pushed into a vector
        struct DelayedGraphicsWork{
            StagingBuffer* stagingBuffer; //optional
            CommandBuffer* submittedBuffer; //needs to be reset or freed. the pool can be deallocated?
            CommandBuffer* matchingGraphicsWork; //optional - would i ever want this to be 2 staged? as in matching graphics submission wait, then another submission wait?
        };

        struct TransferPackage {
            //this is initial transition, staging, and potentially release of 
            std::function<void(CommandBuffer& cmdBuf)> transfer;
            std::function<void(CommandBuffer& cmdBuf)> acquire; //potentially graphics or compute? need a better name
            
            std::vector<void()> transfer_callbacks;
            std::vector<void()> acquire_callbacks;
        };

    public:
        [[nodiscard]] explicit QueueSyncPool(uint16_t size);

        ~QueueSyncPool();
        TimelineSemaphore& GetSemaphore(VkSemaphore semaphore);
        
        TimelineSemaphore* GetSemaphoreForSignaling();

        bool CheckFencesForUsage();
        void CheckFencesForCallbacks();

        //this will ONLY work in a fiber
        void AsyncTransferToGraphics(std::function<void(CommandBuffer& cmdBuf)> transfer, std::function<void(CommandBuffer& cmdBuf)> graphics);
        void AsyncTransferToCompute(std::function<void(CommandBuffer& cmdBuf)> transfer, std::function<void(CommandBuffer& cmdBuf)> compute);
    };
} //namespace EWE