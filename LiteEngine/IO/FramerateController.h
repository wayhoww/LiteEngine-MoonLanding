#pragma once

#include <Windows.h>
#include <iostream>

namespace LiteEngine::IO {

    class FramerateController {
        double desiredDuration = 0;
    public:
        FramerateController(double desiredFPS) {
            this->desiredDuration = 1 / desiredFPS;
        }

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        auto elapsedTicks = currentTime.QuadPart - this->time_in_ticks;
        this->time_in_ticks = currentTime.QuadPart;

        //if (this->currentFPS < 0) {
        //    this->currentFPS = this->desiredFPS > 0 ? this->desiredFPS : 60;
        //    this->averageFPS = this->currentFPS;
        //} else {
        //    this->lastFrameDuration = 1.0 * elapsedTicks / this->timer_frequency;
        //    this->currentFPS = 1.0 / elapsedTicks * this->timer_frequency;
        //    this->averageFPS = this->averageFPS * 0.95 + this->currentFPS * (1 - 0.95);
        //}

        void waitForNextFrame(double lastFrameDuration) {
            /* Declarations */
            HANDLE timer;   /* Timer handle */
            LARGE_INTEGER li;   /* Time defintion */
                                /* Create timer */

            auto sleepTime = std::llround((desiredDuration - lastFrameDuration) * 10000000);

            if (!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
                throw std::exception("cannot create timer");
            /* Set timer properties */
            li.QuadPart = -sleepTime;
            if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
                CloseHandle(timer);
                throw std::exception("cannot set timer");
            }
            /* Start & wait for timer */
            WaitForSingleObject(timer, INFINITE);
            /* Clean resources */
            CloseHandle(timer);
            /* Slept without problems */
           

            /*auto sleepTime = std::round((desiredDuration - lastFrameDuration) * 1000);
            if (sleepTime < 0) return;
            Sleep(sleepTime);*/
        }
    };
}