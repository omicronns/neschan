#include <nes_mapper.h>
#include <nes_trace.h>

#include <algorithm>

shared_ptr<nes_mapper> nes_rom_loader::load_from(uint8_t *rom_data, std::size_t rom_size)
{
    assert(sizeof(ines_header) == 0x10);

    uint8_t *data = rom_data;

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

    auto prg_rom = data;
    data += prg_rom_size;
    auto chr_rom = data;
    data += chr_rom_size;

    shared_ptr<nes_mapper> mapper;

    // @TODO - Change this into a mapper factory class
    switch (mapper_id)
    {
    case 0: mapper = make_shared<nes_mapper_nrom>(prg_rom, prg_rom_size, chr_rom, chr_rom_size, vertical_mirroring); break;
    case 1: mapper = make_shared<nes_mapper_mmc1>(prg_rom, prg_rom_size, chr_rom, chr_rom_size, vertical_mirroring); break;
    case 4: mapper = make_shared<nes_mapper_mmc3>(prg_rom, prg_rom_size, chr_rom, chr_rom_size, vertical_mirroring); break;
    default:
        assert(!"Unsupported mapper id");
    }

    return mapper;
}
