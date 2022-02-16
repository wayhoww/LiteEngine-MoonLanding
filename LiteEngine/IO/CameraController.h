#pragma once

#include <vector>
#include <Windows.h>

namespace LiteEngine::IO {

	// ���� WASD - ��� �Ŀ��Ʒ�ʽ
	class CameraController {
	public:
		void receiveEvent(
			const std::vector<std::tuple<UINT, WPARAM, LPARAM>>& events, 
			float lastFrameDuration
		) {
			for (auto& [msg, wparam, lparam] : events) {
				switch (msg) {
				case WM_KEYDOWN:
					OutputDebugStringA("keydown");
					break;
				case WM_KEYUP:
					OutputDebugStringA("keyup");
					break;
				default:
					break;
				}
			}
			
		}
	};
}