#pragma once

#include <Windows.h>
#include <iostream>

#pragma comment(lib, "Winmm")

namespace LiteEngine::IO {

    enum class FramerateControlling {
        GiveUp,
        WaitableObject,
        Spin
    };

    class FramerateController {
        static constexpr double AverageWeight = 0.1;

        FramerateControlling method;

        int64_t desiredDuration = 0;
        HANDLE timer;  

        bool paused = true;

        // 积累的时间（用于暂停）
        int64_t accTime = 0;

        // 上一次记录的时间
        int64_t lastTime = 0;


        int64_t getPositiveTime(int64_t time) {
            if (time < 0) {
                return std::numeric_limits<int64_t>::max() + (time + 1);
            } else {
                return time;
            }
        }

        double lastFrameDuration = 0;
        double averageFPS = 0;

        int64_t frequencyCount = 0;

    public:
        ~FramerateController() {
            CloseHandle(timer);
        }

        FramerateController(
            double desiredFPS, 
            FramerateControlling cont = FramerateControlling::GiveUp
        ): method(cont) {

            static std::once_flag initTimerFlag;
            if (cont == FramerateControlling::WaitableObject) {
                std::call_once(initTimerFlag, []() {
                    // 可以不一直占用 CPU 并进行高精度的帧率控制
                    TIMECAPS tc;
                    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
                        throw std::exception("cannot get timeing info");
                    }
                    timeBeginPeriod(std::min(std::max(tc.wPeriodMin, 1u), tc.wPeriodMax));
                });
            }
            

            LARGE_INTEGER fCount;
            QueryPerformanceFrequency(&fCount);
            this->frequencyCount = fCount.QuadPart;
            this->averageFPS = desiredFPS;
            this->desiredDuration = std::llround(this->frequencyCount / desiredFPS);
            if (!(timer = CreateWaitableTimer(nullptr, true, nullptr)))
                throw std::exception("cannot create timer");
        }

        double getLastFrameDuration() {
            return this->lastFrameDuration;
        }

        double getAverageFPS() {
            return this->averageFPS;
        }

        double getCurrentFPS() {
            return 1 / this->lastFrameDuration;
        }



        void begin() {
            if (!paused)return;
            paused = false;

            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            this->lastTime = currentTime.QuadPart;
        }

        void pause() {
            if (paused) return;
            paused = true;

            // 暂停的时候只要存储好 accTime 就好了，不需要更新 lastTime
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            this->accTime += getPositiveTime(currentTime.QuadPart - this->lastTime);
        }


        void wait() {
            this->begin();

            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            int64_t elapsedTime = this->accTime + getPositiveTime(currentTime.QuadPart - this->lastTime);

            if (method == FramerateControlling::WaitableObject) {
                if (elapsedTime < this->desiredDuration) {
                    LARGE_INTEGER waitTime;
                    waitTime.QuadPart = elapsedTime - this->desiredDuration;
                    if (!SetWaitableTimer(timer, &waitTime, 0, NULL, NULL, FALSE)) {
                        // CloseHandle(timer);
                        throw std::exception("cannot set timer");
                    }
                    WaitForSingleObject(timer, INFINITE);
                }
            } else if(method == FramerateControlling::Spin) {
                while (elapsedTime < this->desiredDuration) {
                    QueryPerformanceCounter(&currentTime);
                    elapsedTime = this->accTime + getPositiveTime(currentTime.QuadPart - this->lastTime);
                }
            } else {
                // == FramerateControlling::GiveUp
                // do nothing
            }

            QueryPerformanceCounter(&currentTime);
            int64_t newElapsedTime = this->accTime + getPositiveTime(currentTime.QuadPart - this->lastTime);
            this->accTime = 0;
            this->lastTime = currentTime.QuadPart;

            this->lastFrameDuration = 1.0 * newElapsedTime / this->frequencyCount;
            this->averageFPS = AverageWeight * (1 / this->lastFrameDuration) + (1 - AverageWeight) * this->averageFPS;
        
        }
    };
}