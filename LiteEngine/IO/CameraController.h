#pragma once

#include <vector>
#include <Windows.h>

namespace LiteEngine::IO {

	// 经典 WASD - 鼠标 的控制方式
	class MouseInput {
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

		float xSensitivity = 0.003;
		float ySensitivity = 0.003;
		float zSensitivity = 0.003;

		float Ymin = - 0.9 * PI / 2;
		float Ymax = + 0.9 * PI / 2;

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
					this->lastYPos = (lparam >> 16);
					this->mouseLBDown = true;
					break;
				case WM_LBUTTONUP:
					this->mouseLBDown = false;
					break;
				case WM_MOUSEMOVE:
					if (mouseLBDown) {
						float currentXPos = static_cast<short>(lparam);
						float currentYPos = (lparam >> 16);

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