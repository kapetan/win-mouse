#include "mouse.h"

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

Persistent<Function> Mouse::constructor;

Mouse::Mouse(NanCallback* callback) {
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
	NanScope();

	Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(Mouse::New);
	tpl->SetClassName(NanNew<String>("Mouse"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "destroy", Mouse::Destroy);
	NODE_SET_PROTOTYPE_METHOD(tpl, "ref", Mouse::AddRef);
	NODE_SET_PROTOTYPE_METHOD(tpl, "unref", Mouse::RemoveRef);

	NanAssignPersistent(Mouse::constructor, tpl->GetFunction());
	exports->Set(NanNew<String>("Mouse"), tpl->GetFunction());
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

	NanScope();

	uv_mutex_lock(&lock);
	MouseEvent e = {
		event->x,
		event->y,
		event->type
	};
	uv_mutex_unlock(&lock);

	Local<String> name;

	if(e.type == WM_LBUTTONDOWN) name = NanNew<String>("left-down");
	if(e.type == WM_LBUTTONUP) name = NanNew<String>("left-up");
	if(e.type == WM_RBUTTONDOWN) name = NanNew<String>("right-down");
	if(e.type == WM_RBUTTONUP) name = NanNew<String>("right-up");
	if(e.type == WM_MOUSEMOVE) name = NanNew<String>("move");

	Local<Value> argv[] = {
		name,
		NanNew<Number>(e.x),
		NanNew<Number>(e.y)
	};

	event_callback->Call(3, argv);
}

NAN_METHOD(Mouse::New) {
	NanScope();

	NanCallback* callback = new NanCallback(args[0].As<Function>());

	Mouse* mouse = new Mouse(callback);
	mouse->Wrap(args.This());
	mouse->Ref();

	NanReturnValue(args.This());
}

NAN_METHOD(Mouse::Destroy) {
	NanScope();

	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(args.Holder());
	mouse->Stop();
	mouse->Unref();

	NanReturnUndefined();
}

NAN_METHOD(Mouse::AddRef) {
	NanScope();

	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(args.Holder());
	uv_ref((uv_handle_t*) mouse->async);

	NanReturnUndefined();
}

NAN_METHOD(Mouse::RemoveRef) {
	NanScope();

	Mouse* mouse = ObjectWrap::Unwrap<Mouse>(args.Holder());
	uv_unref((uv_handle_t*) mouse->async);

	NanReturnUndefined();
}
