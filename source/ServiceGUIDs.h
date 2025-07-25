#pragma once
#include <windows.h>

// GUID for USB boot interface - may need to be adjusted based on actual PS5 camera GUID
static const GUID GUID_DEVINTERFACE_USB_DEVICE = 
{ 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };
