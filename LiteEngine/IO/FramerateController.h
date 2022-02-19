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

        // ���۵�ʱ�䣨������ͣ��
        int64_t accTime = 0;

        // ��һ�μ�¼��ʱ��
        int64_t lastTime = 0;

        // ���ڷ����ۼ�ʱ��
        int64_t elapsedTime = 0;


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
                    // ���Բ�һֱռ�� CPU �����и߾��ȵ�֡�ʿ���
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

        // ��������������һ֡�������õ�ʱ���ܺ�
        double getLastFrameEndTime() {
            return 1.0 * this->elapsedTime / this->frequencyCount;
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

            // ��ͣ��ʱ��ֻҪ�洢�� accTime �ͺ��ˣ�����Ҫ���� lastTime
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            auto delta = getPositiveTime(currentTime.QuadPart - this->lastTime);
            this->accTime += delta;
            this->elapsedTime += delta;
        }


        void wait() {
            this->begin();

            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            int64_t frameTime = this->accTime + getPositiveTime(currentTime.QuadPart - this->lastTime);

            if (method == FramerateControlling::WaitableObject) {
                if (frameTime < this->desiredDuration) {
                    LARGE_INTEGER waitTime;
                    waitTime.QuadPart = frameTime - this->desiredDuration;
                    if (!SetWaitableTimer(timer, &waitTime, 0, NULL, NULL, FALSE)) {
                        // CloseHandle(timer);
                        throw std::exception("cannot set timer");
                    }
                    WaitForSingleObject(timer, INFINITE);
                }
            } else if(method == FramerateControlling::Spin) {
                while (frameTime < this->desiredDuration) {
                    QueryPerformanceCounter(&currentTime);
                    frameTime = this->accTime + getPositiveTime(currentTime.QuadPart - this->lastTime);
                }
            } else {
                // == FramerateControlling::GiveUp
                // do nothing
            }

            QueryPerformanceCounter(&currentTime);
            int64_t deltaTime = this->accTime + getPositiveTime(currentTime.QuadPart - this->lastTime);
            this->accTime = 0;
            this->elapsedTime += deltaTime;
            this->lastTime = currentTime.QuadPart;

            this->lastFrameDuration = 1.0 * deltaTime / this->frequencyCount;
            this->averageFPS = AverageWeight * (1 / this->lastFrameDuration) + (1 - AverageWeight) * this->averageFPS;
        
        }
    };
}