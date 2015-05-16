#ifndef _MOUSE_H
#define _MOUSE_H

#include <node_object_wrap.h>
#include <nan.h>

#include "mouse_hook.h"

using namespace v8;

struct MouseEvent {
	LONG x;
	LONG y;
	WPARAM type;
};

class Mouse : public node::ObjectWrap {
	public:
		static void Initialize(Handle<Object> exports);
		static Persistent<Function> constructor;

		void Stop();
		void HandleEvent(WPARAM, POINT);
		void HandleSend();

	private:
		MouseHookRef hook_ref;
		MouseEvent* event;
		NanCallback* event_callback;
		uv_async_t* async;
		uv_mutex_t lock;
		bool stopped;

		explicit Mouse(NanCallback*);
		~Mouse();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);
		static NAN_METHOD(AddRef);
		static NAN_METHOD(RemoveRef);
};

#endif
