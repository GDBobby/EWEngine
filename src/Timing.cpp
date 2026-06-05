#include "EWEngine/Data/Timing.h"

namespace EWE{


    void LoopTimer::SetLoopDuration(){
        if (EngineSettings::settingsData.FPS == 0) {
            //small value, for effectively uncapped frame rate
            //given recent games frying GPUs while frame rate is unlimited, users have to go into their settings file and change their frame rate to 0
            //idk if GPU frying will be an issue or not, but this is intentionally obscure
            duration = std::chrono::duration<double>{1.0 / 1000.0};
        }
        else {
            duration = std::chrono::duration<double>{1.0 / EngineSettings::settingsData.FPS};
        }
		
    }

    LoopTimer::DurationType LoopTimer::GetCurrentDelta(){
        return last_time - ClockType::now();
    }

    bool LoopTimer::ReadyForUpdate() {
        const auto current_time = ClockType::now();

        delta = current_time - last_time;
        if(delta >= duration){
            last_time = current_time;
            if(++avg_count >= avg_max){
                last_average = average / avg_count;
                avg_count = 0;
                average = DurationType{0};
            }
        }
        return delta >= duration;
    }
}