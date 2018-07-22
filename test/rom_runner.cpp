#include "rom_runner.h"

void run_rom(nes_system *system, const char *path, nes_rom_exec_mode mode) {
    ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path, std::ifstream::in | std::ifstream::binary);

    std::vector<uint8_t> rom_data;

    file.seekg(0, std::ios::end);
    rom_data.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    rom_data.assign((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    system->run_rom(rom_data.data(), rom_data.size(), mode);
}
