#include "mouse.h"

void Initialize(Local<Object> exports) {
	Mouse::Initialize(exports);
}

NODE_MODULE(addon, Initialize)
