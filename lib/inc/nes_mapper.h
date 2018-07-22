#pragma once

#include <cassert>
#include <cstdint>

#include <memory>

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
    nes_mapper_nrom(
        uint8_t *prg_rom, std::size_t prg_rom_size,
        uint8_t *chr_rom, std::size_t chr_rom_size,
        bool vertical_mirroring)
        : _prg_rom(prg_rom), _prg_rom_size(prg_rom_size),
          _chr_rom(chr_rom), _chr_rom_size(chr_rom_size), _vertical_mirroring(vertical_mirroring)
    {}

    virtual void on_load_ram(nes_memory &mem);
    virtual void on_load_ppu(nes_ppu &ppu);
    virtual void get_info(nes_mapper_info &info);

private :
    uint8_t *_prg_rom;
    std::size_t _prg_rom_size;
    uint8_t *_chr_rom;
    std::size_t _chr_rom_size;
    bool _vertical_mirroring;
};

//
// iNES Mapper 1
// http://wiki.nesdev.com/w/index.php/MMC1
//
class nes_mapper_mmc1 : public nes_mapper
{
public :
    nes_mapper_mmc1(
        uint8_t *prg_rom, std::size_t prg_rom_size,
        uint8_t *chr_rom, std::size_t chr_rom_size,
        bool vertical_mirroring)
        : _prg_rom(prg_rom), _prg_rom_size(prg_rom_size),
          _chr_rom(chr_rom), _chr_rom_size(chr_rom_size), _vertical_mirroring(vertical_mirroring)
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

    uint8_t *_prg_rom;
    std::size_t _prg_rom_size;
    uint8_t *_chr_rom;
    std::size_t _chr_rom_size;
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
    nes_mapper_mmc3(
        uint8_t *prg_rom, std::size_t prg_rom_size,
        uint8_t *chr_rom, std::size_t chr_rom_size,
        bool vertical_mirroring)
        : _prg_rom(prg_rom), _prg_rom_size(prg_rom_size),
          _chr_rom(chr_rom), _chr_rom_size(chr_rom_size), _vertical_mirroring(vertical_mirroring)
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

    uint8_t *_prg_rom;
    std::size_t _prg_rom_size;
    uint8_t *_chr_rom;
    std::size_t _chr_rom_size;
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
    static shared_ptr<nes_mapper> load_from(uint8_t *rom_data, std::size_t rom_size);
};

