#pragma once

// Windows HID parser headers are order-sensitive. hidusage.h must establish
// USAGE/PUSAGE before hidpi.h is parsed on some Windows SDK versions.
#include <Windows.h>
#include <hidusage.h>
#include <hidsdi.h>
#include <hidpi.h>
