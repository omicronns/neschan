#include "stdafx.h"

#include "nes_cpu.h"
#include "nes_system.h"
#include "nes_ppu.h"

using namespace std;

nes_system::nes_system()
{
    _ram = make_unique<nes_memory>();
    _cpu = make_unique<nes_cpu>();
    _ppu = make_unique<nes_ppu>();
    _input = make_unique<nes_input>();
}

nes_system::~nes_system() {}

void nes_system::init()
{
    _stop_requested = false;
    _master_cycle = nes_cycle_t(0);
}

void nes_system::power_on()
{
    init();

    _ram->power_on(this);
    _cpu->power_on(this);
    _ppu->power_on(this);
    _input->power_on(this);
}

void nes_system::reset()
{
    init();

    _ram->reset();
    _cpu->reset();
    _ppu->reset();
    _input->reset();
}

void nes_system::run_program(uint8_t *program_data, std::size_t program_size, uint16_t addr)
{
    _ram->set_bytes(addr, program_data, program_size);
    _cpu->PC() = addr;

    test_loop();
}

void nes_system::load_rom(const char *rom_data, std::size_t rom_size, nes_rom_exec_mode mode)
{
    auto mapper = nes_rom_loader::load_from(rom_data, rom_size);
    _ram->load_mapper(mapper);
    _ppu->load_mapper(mapper);

    if (mode == nes_rom_exec_mode_direct)
    {
        nes_mapper_info info;
        mapper->get_info(info);
        _cpu->PC() = info.code_addr;
    }
    else
    {
        assert(mode == nes_rom_exec_mode_reset);
        _cpu->PC() = ram()->get_word(RESET_HANDLER);
    }
}

void nes_system::run_rom(const char *rom_data, std::size_t rom_size, nes_rom_exec_mode mode)
{
    load_rom(rom_data, rom_size, mode);

    test_loop();
}

void nes_system::test_loop()
{
    auto tick = nes_cycle_t(1);
    while (!_stop_requested)
    {
        step(tick);
    }
}

void nes_system::step(nes_cycle_t count)
{
    _master_cycle += count;

    // Manually step the individual components instead of all components
    // This saves a loop and also it's kinda stupid to step components that doesn't require stepping in the
    // first place. Such as ram / controller, etc.
    _cpu->step_to(_master_cycle);
    _ppu->step_to(_master_cycle);
}
