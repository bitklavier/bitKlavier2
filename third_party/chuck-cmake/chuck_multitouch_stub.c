// Stub implementations for Apple private MultitouchSupport framework symbols.
// bitKlavier embeds ChucK for audio scripting only — HID multitouch is not used.
// Returning NULL from MTDeviceCreateList keeps MTDManager::inited == false so
// all device operations in util_hid.cpp become safe no-ops at runtime.

#include <CoreFoundation/CoreFoundation.h>

typedef void* MTDeviceRef;
typedef void* Finger_t;
typedef int (*MTContactCallbackFunction)(MTDeviceRef, Finger_t*, int, double, int);

CFMutableArrayRef MTDeviceCreateList(void) { return NULL; }
void MTRegisterContactFrameCallback(MTDeviceRef d, MTContactCallbackFunction cb) { (void)d; (void)cb; }
void MTUnregisterContactFrameCallback(MTDeviceRef d, MTContactCallbackFunction cb) { (void)d; (void)cb; }
void MTDeviceStart(MTDeviceRef d, int i) { (void)d; (void)i; }
void MTDeviceStop(MTDeviceRef d) { (void)d; }
