#pragma once

#include <mutex>
#include <functional>

#include <Windows.h>


namespace LiteEngine {

struct WindowSize {
	uint32_t width;
	uint32_t height;
};

class RenderingWindow {
public:
	using EventType = std::tuple<UINT, WPARAM, LPARAM>;
private:
	static constexpr LPCWSTR CLASSNAME = L"RenderingWindow";
	static void registerWndClass();
	static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
	HWND hwnd = nullptr;
	bool windowShouldClose = false;
	DWORD style = 0;
	std::vector<EventType> events;
	bool mouseCaptured = false;
	int mouseCenterX = 0;
	int mouseCenterY = 0;

	int mouseWinCenterX = 0;
	int mouseWinCenterY = 0;

	virtual void render();
	virtual LRESULT event(UINT msg, WPARAM wparam, LPARAM lparam);

public:
	std::function<void(const RenderingWindow&, const std::vector<EventType>&)> renderCallback;

	RenderingWindow(
		LPCWSTR window_name = L"",
		DWORD dw_style = WS_OVERLAPPEDWINDOW,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int width = CW_USEDEFAULT,
		int height = CW_USEDEFAULT,
		std::function<void(const RenderingWindow&, const std::vector<EventType>&)> renderCallback = [](const RenderingWindow&, const std::vector<EventType>&) {}
	);

	virtual ~RenderingWindow() {}

	bool show();
	void runRenderingLoop();

	HWND getHwnd() const {
		return hwnd;
	}

	// width, height
	WindowSize getSize() const {
		RECT rect;
		GetClientRect(this->hwnd, &rect);
		WindowSize out;
		out.width = rect.right - rect.left;
		out.height = rect.bottom - rect.top;
		return out;
	}

	void resizeClientArea(uint32_t width, uint32_t height) {
		RECT rect;
		GetWindowRect(hwnd, &rect);

		RECT buffer = {};
		buffer.bottom = height;
		buffer.right = width;

		AdjustWindowRect(&buffer, this->style, false);
		SetWindowPos(hwnd, nullptr, rect.left, rect.top, 
			buffer.right - buffer.left, 
			buffer.bottom - buffer.top, 0);
	}

};

}
