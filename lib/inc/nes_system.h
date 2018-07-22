#pragma once


#include "nes_component.h"
#include "nes_cpu.h"
#include "nes_ppu.h"
#include "nes_memory.h"
#include "nes_mapper.h"
#include "nes_input.h"

using namespace std;

enum nes_rom_exec_mode
{
    // Run the PRG ROM directly - useful for ROM-based automation test path
    nes_rom_exec_mode_direct,

    // At power on, jump directly to the reset 'interrupt' handler which is effectively main
    // This is what ROM does typically
    // Interestingly this isn't really "documented" in nesdev.com - I had to infer it from
    // inspecting ROMs and using debugger from other emulators
    nes_rom_exec_mode_reset
};


//
// The NES system hardware that manages all the invidual components - CPU, PPU, APU, RAM, etc
// It synchronizes between different components
//
class nes_system
{
public :
    void power_on();
    void reset();

    // Stop the emulation engine and exit the main loop
    void stop() { _stop_requested = true; }

    void run_program(uint8_t *program_data, std::size_t program_size, uint16_t addr);
    void load_rom(uint8_t *rom_data, std::size_t rom_size, nes_rom_exec_mode mode);
    void run_rom(uint8_t *rom_data, std::size_t rom_size, nes_rom_exec_mode mode);

    nes_cpu     *cpu()      { return &_cpu;   }
    nes_memory  *ram()      { return &_ram;   }
    nes_ppu     *ppu()      { return &_ppu;   }
    nes_input   *input()    { return &_input; }

public :
    //
    // step <count> amount of cycles
    // We have a few options:
    // 1. Let nes_system drive the cycle - and each component own their own stepping towards that cycle
    // 2. Let CPU drive cycle - and other component "catch up"
    // 3. Let each component own their own thread - and synchronizes at cycle granuarity
    //
    // I think option #1 will produce the most accurate timing without subjecting too much to OS resource
    // management.
    //
    void step(nes_cycle_t count);

    bool stop_requested() { return _stop_requested; }

private :
    // Emulation loop that is only intended for tests
    void test_loop();

    void init();

    void load_mapper(uint8_t *rom_data, std::size_t rom_size);

private :
    nes_cycle_t _master_cycle;              // keep count of current cycle

    nes_cpu _cpu;
    nes_memory _ram;
    nes_ppu _ppu;
    nes_input _input;

    nes_mapper *_mapper;

    union nes_mappers {
        nes_mappers() {}
        ~nes_mappers() {}

        nes_mapper_nrom _nrom;
        nes_mapper_mmc1 _mmc1;
        nes_mapper_mmc3 _mmc3;
    } _mappers;

    bool _stop_requested;                   // useful for internal testing, or synchronization to rendering
};
