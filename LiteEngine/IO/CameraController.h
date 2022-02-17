#pragma once

#include <vector>
#include <Windows.h>

namespace LiteEngine::IO {

	class MoveController {
	public:
		// s^(-1)
		float initialSpeed = 0.5;

		// s^(-2)
		float acceleration = 3;

		// max speed (s^-1)
		float maxSpeed = 30;

		float maxPosition = std::numeric_limits<float>::infinity();
		float minPosition = -std::numeric_limits<float>::infinity();
;
	protected:

		WPARAM keyPlus;
		WPARAM keyMinus;
		char lastState = 0;
		float currentSpeed = 0;
		float accumulatedPos = 0;


	public:


		MoveController(char keyPlus, char keyMinus): 
			keyPlus(getKeyCode(keyPlus)), keyMinus(getKeyCode(keyMinus)) {}

		float getValue() const {
			return accumulatedPos;
		}

		float popValue() {
			auto backup = accumulatedPos;
			accumulatedPos = 0;
			return backup;
		}

		void receiveEvent(
			const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events, 
			float lastFrameDuration
		) {
			int currentState = 0;
			bool immediateKeyUp = false;
			for (auto& [msg, wparam, _]: events) {
				if (msg == WM_KEYDOWN) {
					if (wparam == keyPlus) {
						currentState = 1;
						immediateKeyUp = false;
					} else if (wparam == keyMinus) {
						currentState = -1;
						immediateKeyUp = false;
					}
				} else if (msg == WM_KEYUP) {
					if(wparam == keyPlus && lastState == 1 || 
						wparam == keyMinus && lastState == -1) {
						// 如果前一帧按下的按键有在本帧按键按下前松开过
						// 清空状态
						if (currentState == 0) {
							lastState = 0;
						} else {
							if (wparam == keyPlus && currentState == 1 ||
								wparam == keyMinus && currentState == -1) {
								immediateKeyUp = true;
							}
						}
					}
				}
			}

			if (currentState == 0) currentState = lastState;

			// invariant: currentFrameState 表示这一帧最后一次按下的键对应状态

			if (currentState != lastState || currentState == 0) {
				currentSpeed = currentState * initialSpeed;
			} 

			float deltaPos = 0;
			if (currentSpeed == maxSpeed * currentState) {
				deltaPos = currentSpeed * lastFrameDuration;
			} else if (abs(currentSpeed + currentState * lastFrameDuration * acceleration) > maxSpeed) {
				// 如果这次加速将超过最高速度
				float tIntermediate = (maxSpeed - abs(currentSpeed)) / (lastFrameDuration * acceleration);
				deltaPos += (2 * currentSpeed + currentState * tIntermediate * acceleration) * tIntermediate / 2;
				deltaPos += (lastFrameDuration - tIntermediate) * maxSpeed * currentState;
				currentSpeed = maxSpeed * currentState;
			} else {
				deltaPos = (2 * currentSpeed + currentState * lastFrameDuration * acceleration) * lastFrameDuration / 2;
				currentSpeed += currentState * lastFrameDuration * acceleration;
				accumulatedPos += deltaPos;
			}

			accumulatedPos += deltaPos;
			if (accumulatedPos > maxPosition) accumulatedPos = maxPosition;
			if (accumulatedPos < minPosition) accumulatedPos = minPosition;

			lastState = currentState;
			if (immediateKeyUp) lastState = 0;
		}
	};

	// 经典 WASD - 鼠标 的控制方式
	class HoldRotateController {
		bool mouseLBDown = false;

		int lastXPos = 0;
		int lastYPos = 0;

		void boundXYZ() {
			// 不至于一帧转好几圈吧
			// 并且，即使出现了这种情况，问题也不大。。
			if (accX < PI) {
				accX += 2 * PI;
			}
			if (accX > PI) {
				accX -= 2 * PI;
			}
			if (accY > Ymax) {
				accY = Ymax;
			}
			if (accY < Ymin) {
				accY = Ymin;
			}

			if (accZ < zMin) {
				accZ = zMin;
			}

			if (accZ > zMax) {
				accZ = zMax;
			}
		}

	public:
		float accX = 0;
		float accY = 0;
		float accZ = 0;

		float xSensitivity = 0.003f;
		float ySensitivity = 0.003f;
		float zSensitivity = 0.003f;

		float Ymin = - float(0.9 * PI / 2);
		float Ymax = + float(0.9 * PI / 2);

		float zMin = -1;
		float zMax = 1;

		std::tuple<float, float, float> getXYZ() {
			return { accX, accY, accZ };
		}

		void receiveEvent(
			const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events, 
			float lastFrameDuration
		) {
			for (auto& [msg, wparam, lparam] : events) {
				switch (msg) {
				case WM_LBUTTONDOWN:
					this->lastXPos = static_cast<short>(lparam);
					this->lastYPos = static_cast<short>(lparam >> 16);
					this->mouseLBDown = true;
					break;
				case WM_LBUTTONUP:
					this->mouseLBDown = false;
					break;
				case WM_MOUSEMOVE:
					if (mouseLBDown) {
						int currentXPos = static_cast<short>(lparam);
						int currentYPos = static_cast<short>(lparam >> 16);

						accX += (currentXPos - lastXPos) * xSensitivity;
						accY += (currentYPos - lastYPos) * ySensitivity;

						this->lastXPos = currentXPos;
						this->lastYPos = currentYPos;

						this->boundXYZ();
					}
					break;
				case WM_MOUSEWHEEL:
				{
					auto deltaZ = GET_WHEEL_DELTA_WPARAM(wparam);
					this->accZ += this->zSensitivity * deltaZ;

					this->boundXYZ();
				}
				default:
					break;
				}
			}
			
		}
	};
}