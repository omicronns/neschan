#include "nes_cpu.h"
#include "nes_system.h"
#include "nes_ppu.h"
#include "nes_input.h"

#include <algorithm>

using namespace std;

void nes_system::init()
{
    _stop_requested = false;
    _master_cycle = nes_cycle_t(0);
}

void nes_system::power_on()
{
    init();

    _ram.power_on(this);
    _cpu.power_on(this);
    _ppu.power_on(this);
    _input.power_on(this);
}

void nes_system::reset()
{
    init();

    _ram.reset();
    _cpu.reset();
    _ppu.reset();
    _input.reset();
}

void nes_system::run_program(uint8_t *program_data, std::size_t program_size, uint16_t addr)
{
    _ram.set_bytes(addr, program_data, program_size);
    _cpu.PC() = addr;

    test_loop();
}

void nes_system::load_rom(uint8_t *rom_data, std::size_t rom_size, nes_rom_exec_mode mode)
{
    load_mapper(rom_data, rom_size);
    _ram.load_mapper(_mapper);
    _ppu.load_mapper(_mapper);

    if (mode == nes_rom_exec_mode_direct)
    {
        nes_mapper_info info;
        _mapper->get_info(info);
        _cpu.PC() = info.code_addr;
    }
    else
    {
        assert(mode == nes_rom_exec_mode_reset);
        _cpu.PC() = ram()->get_word(RESET_HANDLER);
    }
}

#define FLAG_6_USE_VERTICAL_MIRRORING_MASK 0x1
#define FLAG_6_HAS_BATTERY_BACKED_PRG_RAM_MASK 0x2
#define FLAG_6_HAS_TRAINER_MASK  0x4
#define FLAG_6_USE_FOUR_SCREEN_VRAM_MASK 0x8
#define FLAG_6_LO_MAPPER_NUMBER_MASK 0xf0
#define FLAG_7_HI_MAPPER_NUMBER_MASK 0xf0

void nes_system::load_mapper(uint8_t *rom_data, std::size_t rom_size)
{
    struct ines_header
    {
        uint8_t magic[4];       // 0x4E, 0x45, 0x53, 0x1A
        uint8_t prg_size;       // PRG ROM in 16K
        uint8_t chr_size;       // CHR ROM in 8K, 0 -> using CHR RAM
        uint8_t flag6;
        uint8_t flag7;
        uint8_t prg_ram_size;   // PRG RAM in 8K
        uint8_t flag9;
        uint8_t flag10;         // unofficial
        uint8_t reserved[5];    // reserved
    };

    assert(sizeof(ines_header) == 0x10);

    uint8_t *data = rom_data;

    // Parse header
    ines_header header;
    std::copy_n(data, sizeof(header), (char *)&header);
    data += sizeof(header);

    if (header.flag6 & FLAG_6_HAS_TRAINER_MASK)
    {
        // skip the 512-byte trainer
        data += 0x200;
    }

    NES_TRACE1("[NES_ROM] HEADER: Flags6 = 0x" << std::hex << (uint32_t) header.flag6);
    bool vertical_mirroring = header.flag6 & FLAG_6_USE_VERTICAL_MIRRORING_MASK;
    if (vertical_mirroring)
    {
        NES_TRACE1("    Mirroring: Vertical");
    }
    else
    {
        NES_TRACE1("    Mirroring: Horizontal");
    }

    if (header.flag7 == 0x44)
    {
        // This might be one of the earlier dumps with bad iNes header (D stands for diskdude)
        NES_TRACE1("[NES_ROM] Bad flag7 0x44 detected. Resetting to 0...");
        header.flag7 = 0;
    }

    NES_TRACE1("[NES_ROM] HEADER: Flags7 = 0x" << std::hex << (uint32_t) header.flag7);
    int mapper_id = ((header.flag6 & FLAG_6_LO_MAPPER_NUMBER_MASK) >> 4) + ((header.flag7 & FLAG_7_HI_MAPPER_NUMBER_MASK));
    NES_TRACE1("[NES_ROM] HEADER: Mapper_ID = " << std::dec << mapper_id);

    int prg_rom_size = header.prg_size * 0x4000;    // 16KB
    int chr_rom_size = header.chr_size * 0x2000;    // 8KB

    NES_TRACE1("[NES_ROM] HEADER: PRG ROM Size = 0x" << std::hex << (uint32_t) prg_rom_size);
    NES_TRACE1("[NES_ROM] HEADER: CHR_ROM Size = 0x" << std::hex << (uint32_t) chr_rom_size);

    auto prg_rom = data;
    data += prg_rom_size;
    auto chr_rom = data;
    data += chr_rom_size;

    // @TODO - Change this into a mapper factory class
    switch (mapper_id)
    {
    case 0: _mapper = new(&_mappers._nrom) nes_mapper_nrom(prg_rom, prg_rom_size, chr_rom, chr_rom_size, vertical_mirroring); break;
    case 1: _mapper = new(&_mappers._mmc1) nes_mapper_mmc1(prg_rom, prg_rom_size, chr_rom, chr_rom_size, vertical_mirroring); break;
    case 4: _mapper = new(&_mappers._mmc3) nes_mapper_mmc3(prg_rom, prg_rom_size, chr_rom, chr_rom_size, vertical_mirroring); break;
    default:
        assert(!"Unsupported mapper id");
    }
}

void nes_system::run_rom(uint8_t *rom_data, std::size_t rom_size, nes_rom_exec_mode mode)
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
    _cpu.step_to(_master_cycle);
    _ppu.step_to(_master_cycle);
}
