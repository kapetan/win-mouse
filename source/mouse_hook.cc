#include <stdexcept>
#include "mouse_hook.h"

void RunThread(void* arg) {
	MouseHookManager* mouse = (MouseHookManager*) arg;
	mouse->_Run();
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if(nCode >= 0) {
		MSLLHOOKSTRUCT* data = (MSLLHOOKSTRUCT*) lParam;
		POINT point = data->pt;

		MouseHookManager::GetInstance()->_HandleEvent(wParam, point);
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

MouseHookRef MouseHookRegister(MouseHookCallback callback, void* data) {
	return MouseHookManager::GetInstance()->Register(callback, data);
}

void MouseHookUnregister(MouseHookRef ref) {
	MouseHookManager::GetInstance()->Unregister(ref);
}

MouseHookManager* MouseHookManager::GetInstance() {
	static MouseHookManager manager;
	return &manager;
}

MouseHookManager::MouseHookManager() {
	running = false;
	thread_id = NULL;
	listeners = new std::list<MouseHookRef>();
	uv_mutex_init(&event_lock);
	uv_mutex_init(&init_lock);
	uv_cond_init(&init_cond);
}

MouseHookManager::~MouseHookManager() {
	if(!listeners->empty()) Stop();

	delete listeners;
	uv_mutex_destroy(&event_lock);
	uv_mutex_destroy(&init_lock);
	uv_cond_destroy(&init_cond);
}

MouseHookRef MouseHookManager::Register(MouseHookCallback callback, void* data) {
	uv_mutex_lock(&event_lock);
	
	bool empty = listeners->empty();

	MouseHookRef entry = new MouseHookEntry();
	entry->callback = callback;
	entry->data = data;
	listeners->push_back(entry);

	uv_mutex_unlock(&event_lock);
	
	if(empty) uv_thread_create(&thread, RunThread, this);

	return entry;
}

void MouseHookManager::Unregister(MouseHookRef ref) {
	uv_mutex_lock(&event_lock);

	listeners->remove(ref);
	delete ref;
	bool empty = listeners->empty();

	uv_mutex_unlock(&event_lock);
	
	if(empty) Stop();
}

void MouseHookManager::_HandleEvent(WPARAM type, POINT point) {
	uv_mutex_lock(&event_lock);

	for(std::list<MouseHookRef>::iterator it = listeners->begin(); it != listeners->end(); it++) {
		(*it)->callback(type, point, (*it)->data);
	}

	uv_mutex_unlock(&event_lock);
}

void MouseHookManager::_Run() {
	MSG msg;
	BOOL val;

	uv_mutex_lock(&init_lock);

	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	thread_id = GetCurrentThreadId();
	
	uv_cond_signal(&init_cond);
	uv_mutex_unlock(&init_lock);

	HHOOK hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, (HINSTANCE) NULL, 0);

	while((val = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if(val == -1) throw std::runtime_error("GetMessage failed (return value -1)");
		if(msg.message == WM_STOP_MESSAGE_LOOP) break;
	}

	UnhookWindowsHookEx(hook);

	uv_mutex_lock(&init_lock);
	thread_id = NULL;
	uv_mutex_unlock(&init_lock);
}

void MouseHookManager::Stop() {
	uv_mutex_lock(&init_lock);

	while(thread_id == NULL) uv_cond_wait(&init_cond, &init_lock);
	DWORD id = thread_id;
	
	uv_mutex_unlock(&init_lock);

	PostThreadMessage(id, WM_STOP_MESSAGE_LOOP, NULL, NULL);
	uv_thread_join(&thread);
}
