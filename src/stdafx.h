// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32
#include "targetver.h"
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifdef _WIN32
// Windows Header Files:
#include <windows.h>
#endif

// C RunTime Header Files
#include <cstdlib>
#include <cassert>
#include <cstdint>

#include <string>

#include <nes_system.h>
#include <nes_ppu.h>
#include <nes_cpu.h>
#include <nes_input.h>
#include <nes_trace.h>

#include <SDL.h>
#include <SDL_joystick.h>
