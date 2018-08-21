#ifndef _MOUSE_H
#define _MOUSE_H

#include <nan.h>

#include "mouse_hook.h"

using namespace v8;

struct MouseEvent {
	LONG x;
	LONG y;
	WPARAM type;
};

const unsigned int BUFFER_SIZE = 10;

class Mouse : public Nan::ObjectWrap {
	public:
		static void Initialize(Handle<Object> exports);
		static Nan::Persistent<Function> constructor;

		void Stop();
		void HandleEvent(WPARAM, POINT);
		void HandleSend();

	private:
		MouseHookRef hook_ref;
		MouseEvent* eventBuffer[BUFFER_SIZE];
		unsigned int readIndex;
		unsigned int writeIndex;
		Nan::Callback* event_callback;
		Nan::AsyncResource* async_resource;
		uv_async_t* async;
		uv_mutex_t lock;
		bool stopped;

		explicit Mouse(Nan::Callback*);
		~Mouse();

		static NAN_METHOD(New);
		static NAN_METHOD(Destroy);
		static NAN_METHOD(AddRef);
		static NAN_METHOD(RemoveRef);
};

#endif
