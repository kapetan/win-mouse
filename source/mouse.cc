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

	event = new MouseEvent();
	event_callback = callback;
	stopped = false;

	uv_async_init(uv_default_loop(), async, OnSend);
	uv_mutex_init(&lock);

	hook_ref = MouseHookRegister(OnMouseEvent, this);
}

Mouse::~Mouse() {
	Stop();
	uv_mutex_destroy(&lock);
	delete event;
	delete event_callback;
}

void Mouse::Initialize(Handle<Object> exports) {
	Nan::HandleScope scope;

	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(Mouse::New);
	tpl->SetClassName(Nan::New<String>("Mouse").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "destroy", Mouse::Destroy);
	Nan::SetPrototypeMethod(tpl, "ref", Mouse::AddRef);
	Nan::SetPrototypeMethod(tpl, "unref", Mouse::RemoveRef);

	Mouse::constructor.Reset(tpl->GetFunction());
	exports->Set(Nan::New<String>("Mouse").ToLocalChecked(), tpl->GetFunction());
}

void Mouse::Stop() {
	if(stopped)	return;
	stopped = true;

	MouseHookUnregister(hook_ref);
	uv_close((uv_handle_t*) async, OnClose);
}

void Mouse::HandleEvent(WPARAM type, POINT point) {
	if(!IsMouseEvent(type)) return;
	uv_mutex_lock(&lock);
	event->x = point.x;
	event->y = point.y;
	event->type = type;
	uv_mutex_unlock(&lock);
	uv_async_send(async);
}

void Mouse::HandleSend() {
	if(stopped) return;

	Nan::HandleScope scope;

	uv_mutex_lock(&lock);
	MouseEvent e = {
		event->x,
		event->y,
		event->type
	};
	uv_mutex_unlock(&lock);

	const char* name;

	if(e.type == WM_LBUTTONDOWN) name = LEFT_DOWN;
	if(e.type == WM_LBUTTONUP) name = LEFT_UP;
	if(e.type == WM_RBUTTONDOWN) name = RIGHT_DOWN;
	if(e.type == WM_RBUTTONUP) name = RIGHT_UP;
	if(e.type == WM_MOUSEMOVE) name = MOVE;

	Local<Value> argv[] = {
		Nan::New<String>(name).ToLocalChecked(),
		Nan::New<Number>(e.x),
		Nan::New<Number>(e.y)
	};

	event_callback->Call(3, argv);
}

NAN_METHOD(Mouse::New) {
	Nan::Callback* callback = new Nan::Callback(info[0].As<Function>());

	Mouse* mouse = new Mouse(callback);
	mouse->Wrap(info.This());
	mouse->Ref();

	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Mouse::Destroy) {
	Mouse* mouse = Nan::ObjectWrap::Unwrap<Mouse>(info.Holder());
	mouse->Stop();
	mouse->Unref();

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
