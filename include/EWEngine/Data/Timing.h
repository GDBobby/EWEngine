#pragma once

#include <chrono>
#include <cstdint>

namespace EWE{
    struct LoopTimer {
        using DurationType = std::chrono::duration<float, std::micro>;
        using ClockType = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<ClockType>;

        DurationType duration{};

        DurationType min{0.f};
        DurationType peak{};
        DurationType last_average{0.f};
        DurationType average{0.f};

        DurationType delta{0.f};

        TimePoint last_time;

        uint16_t avg_max = 1000;
        uint16_t avg_count{0};
        

        DurationType GetCurrentDelta();
        bool ReadyForRenderUpdate();
        bool ReadyForLogicUpdate();

        void SetLoopDuration();
    };
}