#pragma once

#include "nes_system.h"

#include <vector>
#include <fstream>
#include <streambuf>

void run_rom(nes_system *system, const char *path, nes_rom_exec_mode mode);
