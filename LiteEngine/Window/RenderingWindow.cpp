#include "RenderingWindow.h"
#include <Windows.h>

namespace LiteEngine {

LRESULT CALLBACK RenderingWindow::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_CREATE) {
		auto cs = reinterpret_cast<LPCREATESTRUCT>(lparam);
		if (cs) {
			auto pThis = cs->lpCreateParams;
			if (pThis) {
				SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
				return 0;
			}
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} else {
		auto pThis = reinterpret_cast<RenderingWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (pThis) return pThis->event(msg, wparam, lparam);
		else return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

void RenderingWindow::registerWndClass() {
	static bool registered = false;
	static std::mutex lock;
	if (registered) return;
	lock.lock();
	if (registered) {
		lock.unlock();
		return;
	}
	WNDCLASS wnd = {};
	wnd.lpszClassName = CLASSNAME;
	wnd.lpfnWndProc = RenderingWindow::windowProc;
	RegisterClass(&wnd);
	registered = true;
	lock.unlock();
}

RenderingWindow::RenderingWindow(
	LPCWSTR window_name,
	DWORD dw_style,
	int x,
	int y,
	int width,
	int height,
	std::function<void(const RenderingWindow&, const std::vector<EventType>&)> renderCallback
) {
	registerWndClass();
	// 获取当前进程 .exe 的 hinstance
	auto hinstance = GetModuleHandle(nullptr);
	hwnd = CreateWindow(
		CLASSNAME, window_name, dw_style,
		x, y, width, height,
		nullptr, nullptr, hinstance,
		static_cast<void*>(this)
	);
	this->style = dw_style;
	this->renderCallback = renderCallback;
}

bool RenderingWindow::show() {
	return ShowWindow(hwnd, SW_SHOWNORMAL);
}

LRESULT RenderingWindow::event(UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {
		this->windowShouldClose = true;
		return 0;
	} else if (msg == WM_KEYDOWN) {
		this->events.push_back({ msg, wparam, lparam });
		return 0;
	} else {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

void RenderingWindow::runRenderingLoop() {
	MSG msg;
	while (!windowShouldClose) {
		bool have_msg = PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE);
		if (have_msg) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			render();
		}
	}
	windowShouldClose = false;
}

void RenderingWindow::render() {
	// 这个函数执行的时候不会有新的事件
	this->renderCallback(*this, this->events);
	this->events.clear();
}

}