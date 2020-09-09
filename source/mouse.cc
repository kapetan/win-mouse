#include "mouse.h"

const char* LEFT_DOWN = "left-down";
const char* LEFT_UP = "left-up";
const char* RIGHT_DOWN = "right-down";
const char* RIGHT_UP = "right-up";
const char* MOVE = "move";

bool IsMouseEvent(WPARAM type) {
	return type == WM_LBUTTONDOWN ||
		type == WM_LBUTTONUP ||
		type == WM_RBUTTONDOWN ||
		type == WM_RBUTTONUP ||
		type == WM_MOUSEMOVE;
}

void OnMouseEvent(WPARAM type, POINT point, void* data) {
	Mouse* mouse = (Mouse*) data;
	mouse->HandleEvent(type, point);
}

NAUV_WORK_CB(OnSend) {
	Mouse* mouse = (Mouse*) async->data;
	mouse->HandleSend();
}

void OnClose(uv_handle_t* handle) {
	uv_async_t* async = (uv_async_t*) handle;
	delete async;
}

Nan::Persistent<Function> Mouse::constructor;

Mouse::Mouse(Nan::Callback* callback) {
	async = new uv_async_t;
	async->data = this;

	for (size_t i = 0; i < BUFFER_SIZE; i++) {
		eventBuffer[i] = new MouseEvent();
	}

	readIndex = 0;
	writeIndex = 0;

	event_callback = callback;
	async_resource = new Nan::AsyncResource("win-mouse:Mouse");
	stopped = false;

	uv_async_init(uv_default_loop(), async, OnSend);
	uv_mutex_init(&lock);

	hook_ref = MouseHookRegister(OnMouseEvent, this);
}

Mouse::~Mouse() {
	Stop();
	uv_mutex_destroy(&lock);
	delete event_callback;

	// HACK: Sometimes deleting async resource segfaults.
	// Probably related to https://github.com/nodejs/nan/issues/772
	if (!Nan::GetCurrentContext().IsEmpty()) {
		delete async_resource;
	}

	for (size_t i = 0; i < BUFFER_SIZE; i++) {
		delete eventBuffer[i];
	}
}

void Mouse::Initialize(Local<Object> exports, Local<Value> module, Local<Context> context) {
	Nan::HandleScope scope;

	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Mouse::New);
	tpl->SetClassName(Nan::New<String>("Mouse").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "destroy", Mouse::Destroy);
	Nan::SetPrototypeMethod(tpl, "ref", Mouse::AddRef);
	Nan::SetPrototypeMethod(tpl, "unref", Mouse::RemoveRef);

	Mouse::constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
	exports->Set(context,
		Nan::New("Mouse").ToLocalChecked(),
		Nan::GetFunction(tpl).ToLocalChecked());
}

void Mouse::Stop() {
	uv_mutex_lock(&lock);

	if (!stopped) {
		stopped = true;
		MouseHookUnregister(hook_ref);
		uv_close((uv_handle_t*) async, OnClose);
	}

	uv_mutex_unlock(&lock);
}

void Mouse::HandleEvent(WPARAM type, POINT point) {
	if(!IsMouseEvent(type) || stopped) return;

	uv_mutex_lock(&lock);

	if (!stopped) {
		eventBuffer[writeIndex]->x = point.x;
		eventBuffer[writeIndex]->y = point.y;
		eventBuffer[writeIndex]->type = type;
		writeIndex = (writeIndex + 1) % BUFFER_SIZE;
		uv_async_send(async);
	}

	uv_mutex_unlock(&lock);
}

void Mouse::HandleSend() {
	Nan::HandleScope scope;

	uv_mutex_lock(&lock);

	while (readIndex != writeIndex && !stopped) {
		MouseEvent e = {
			eventBuffer[readIndex]->x,
			eventBuffer[readIndex]->y,
			eventBuffer[readIndex]->type
		};
		readIndex = (readIndex + 1) % BUFFER_SIZE;
		const char* name;

		if (e.type == WM_LBUTTONDOWN) name = LEFT_DOWN;
		if (e.type == WM_LBUTTONUP) name = LEFT_UP;
		if (e.type == WM_RBUTTONDOWN) name = RIGHT_DOWN;
		if (e.type == WM_RBUTTONUP) name = RIGHT_UP;
		if (e.type == WM_MOUSEMOVE) name = MOVE;

		Local<Value> argv[] = {
			Nan::New<String>(name).ToLocalChecked(),
			Nan::New<Number>(e.x),
			Nan::New<Number>(e.y)
		};

		event_callback->Call(3, argv, async_resource);
	}

	uv_mutex_unlock(&lock);
}

NAN_METHOD(Mouse::New) {
	Nan::Callback* callback = new Nan::Callback(info[0].As<Function>());

	Mouse* mouse = new Mouse(callback);
	mouse->Wrap(info.This());

	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Mouse::Destroy) {
	Mouse* mouse = Nan::ObjectWrap::Unwrap<Mouse>(info.Holder());
	mouse->Stop();

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Mouse::AddRef) {
	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(info.Holder());
	uv_ref((uv_handle_t*) mouse->async);

	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Mouse::RemoveRef) {
	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(info.Holder());
	uv_unref((uv_handle_t*) mouse->async);

	info.GetReturnValue().SetUndefined();
}
