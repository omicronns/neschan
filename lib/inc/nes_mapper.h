#pragma once

#include <cstdint>
#include <nes_trace.h>
#include <memory>
#include <algorithm>
#include <vector>

#include <common.h>

using namespace std;

enum nes_mapper_flags : uint16_t
{
    nes_mapper_flags_none = 0,

    nes_mapper_flags_mirroring_mask = 0x3,

    // A, B
    // A, B
    nes_mapper_flags_vertical_mirroring = 0x2,

    // A, A
    // B, B
    nes_mapper_flags_horizontal_mirroring = 0x3,

    // ?
    nes_mapper_flags_one_screen_upper_bank = 0x1,

    // ?
    nes_mapper_flags_one_screen_lower_bank = 0x0,

    // Has registers
    nes_mapper_flags_has_registers = 0x4,
};

struct nes_mapper_info
{
    uint16_t code_addr;            // start running code here
    uint16_t reg_start;            // beginning of mapper registers
    uint16_t reg_end;              // end of mapper registers - inclusive
    nes_mapper_flags flags;        // whatever flags you might need
};

class nes_ppu;
class nes_cpu;
class nes_memory;

class nes_mapper
{
public :
    //
    // Called when mapper is loaded into memory
    // Useful when all you need is a one-time memcpy
    //
    virtual void on_load_ram(nes_memory &mem) {}

    //
    // Called when mapper is loaded into PPU
    // Useful when all you need is a one-time memcpy
    //
    virtual void on_load_ppu(nes_ppu &ppu) {}

    //
    // Returns various mapper related information
    //
    virtual void get_info(nes_mapper_info &) = 0;

    //
    // Write mapper register in the given address
    // Caller should check if addr is in range of register first
    //
    virtual void write_reg(uint16_t addr, uint8_t val) {};

    virtual ~nes_mapper() {}
};

//
// iNES Mapper 0
// http://wiki.nesdev.com/w/index.php/NROM
//
class nes_mapper_nrom : public nes_mapper
{
public :
    nes_mapper_nrom(shared_ptr<vector<uint8_t>> &prg_rom, shared_ptr<vector<uint8_t>> &chr_rom, bool vertical_mirroring)
        :_prg_rom(prg_rom), _chr_rom(chr_rom), _vertical_mirroring(vertical_mirroring)
    {

    }

    virtual void on_load_ram(nes_memory &mem);
    virtual void on_load_ppu(nes_ppu &ppu);
    virtual void get_info(nes_mapper_info &info);

private :
    shared_ptr<vector<uint8_t>> _prg_rom;
    shared_ptr<vector<uint8_t>> _chr_rom;
    bool _vertical_mirroring;
};

//
// iNES Mapper 1
// http://wiki.nesdev.com/w/index.php/MMC1
//
class nes_mapper_mmc1 : public nes_mapper
{
public :
    nes_mapper_mmc1(shared_ptr<vector<uint8_t>> &prg_rom, shared_ptr<vector<uint8_t>> &chr_rom, bool vertical_mirroring)
        :_prg_rom(prg_rom), _chr_rom(chr_rom), _vertical_mirroring(vertical_mirroring)
    {
        _bit_latch = 0;
    }

    virtual void on_load_ram(nes_memory &mem);
    virtual void on_load_ppu(nes_ppu &ppu);
    virtual void get_info(nes_mapper_info &info);

    virtual void write_reg(uint16_t addr, uint8_t val);

 private :
    void write_control(uint8_t val);
    void write_chr_bank_0(uint8_t val);
    void write_chr_bank_1(uint8_t val);
    void write_prg_bank(uint8_t val);

private :
    nes_ppu *_ppu;
    nes_memory *_mem;

    shared_ptr<vector<uint8_t>> _prg_rom;
    shared_ptr<vector<uint8_t>> _chr_rom;
    bool _vertical_mirroring;


    uint8_t _bit_latch;                         // for serial port
    uint8_t _reg;                               // current register being written
    uint8_t _control;                           // control register
};

//
// iNES Mapper 4
// http://wiki.nesdev.com/w/index.php/MMC3
//
class nes_mapper_mmc3 : public nes_mapper
{
public:
    nes_mapper_mmc3(shared_ptr<vector<uint8_t>> &prg_rom, shared_ptr<vector<uint8_t>> &chr_rom, bool vertical_mirroring)
        :_prg_rom(prg_rom), _chr_rom(chr_rom), _vertical_mirroring(vertical_mirroring)
    {
        // 1 -> neither 0 or 0x40 - means not yet initialized (and always will be different)
        _prev_prg_mode = 1;

        _bank_select = 0;
    }

    virtual void on_load_ram(nes_memory &mem);
    virtual void on_load_ppu(nes_ppu &ppu);
    virtual void get_info(nes_mapper_info &info);

    virtual void write_reg(uint16_t addr, uint8_t val);

private:
    void write_bank_select(uint8_t val);
    void write_bank_data(uint8_t val);
    void write_mirroring(uint8_t val);
    void write_prg_ram_protect(uint8_t val) { assert(false); }
    void write_irq_latch(uint8_t val) { assert(false); }
    void write_irq_reload(uint8_t val) { assert(false); }
    void write_irq_disable(uint8_t val) { }
    void write_irq_enable(uint8_t val) { assert(false); }

private:
    nes_ppu * _ppu;
    nes_memory *_mem;

    shared_ptr<vector<uint8_t>> _prg_rom;
    shared_ptr<vector<uint8_t>> _chr_rom;
    bool _vertical_mirroring;

    uint8_t _bank_select;                       // control register
    uint8_t _prev_prg_mode;                     // previous prg mode
};

#define FLAG_6_USE_VERTICAL_MIRRORING_MASK 0x1
#define FLAG_6_HAS_BATTERY_BACKED_PRG_RAM_MASK 0x2
#define FLAG_6_HAS_TRAINER_MASK  0x4
#define FLAG_6_USE_FOUR_SCREEN_VRAM_MASK 0x8
#define FLAG_6_LO_MAPPER_NUMBER_MASK 0xf0
#define FLAG_7_HI_MAPPER_NUMBER_MASK 0xf0

class nes_rom_loader
{
public :
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

    // Loads a NES ROM file
    // Automatically detects format according to extension and header
    // Returns a nes_mapper instance which has all necessary memory mapped
    static shared_ptr<nes_mapper> load_from(const char *rom_data, std::size_t rom_size)
    {
        assert(sizeof(ines_header) == 0x10);

        const char *data = rom_data;

        // Parse header
        ines_header header;
        std::copy_n(data, sizeof(header), (char *)&header);
        data += sizeof(header);

        if (header.flag6 & FLAG_6_HAS_TRAINER_MASK)
        {
            NES_TRACE1("[NES_ROM] HEADER: Trainer bytes 0x200 present.");
            NES_TRACE1("[NES_ROM] Skipping trainer bytes...");

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

        auto prg_rom = make_shared<vector<uint8_t>>(prg_rom_size);
        auto chr_rom = make_shared<vector<uint8_t>>(chr_rom_size);

        std::copy_n(data, prg_rom->size(), (char *)prg_rom->data());
        data += prg_rom->size();
        std::copy_n(data, chr_rom->size(), (char *)chr_rom->data());
        data += chr_rom->size();

        shared_ptr<nes_mapper> mapper;

        // @TODO - Change this into a mapper factory class
        switch (mapper_id)
        {
        case 0: mapper = make_shared<nes_mapper_nrom>(prg_rom, chr_rom, vertical_mirroring); break;
        case 1: mapper = make_shared<nes_mapper_mmc1>(prg_rom, chr_rom, vertical_mirroring); break;
        case 4: mapper = make_shared<nes_mapper_mmc3>(prg_rom, chr_rom, vertical_mirroring); break;
        default:
            assert(!"Unsupported mapper id");
        }

        return mapper;
    }
};

