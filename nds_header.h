#ifndef _MD_NDS_HEADER_H
#define _MD_NDS_HEADER_H

#include <cstdint>
#include <vector>
#include <iterator>

#include "util.h"

namespace nds
{
  class Header
  {
  public:
    static const int Size = 0x200;

    Header() {};
    Header(std::vector<uint8_t>& rom)
    {
      std::copy(rom.begin(), rom.begin() + Header::Size, std::back_inserter(m_header));
    }
    
    std::vector<uint8_t> get_raw();

    std::string title();
    std::string game_code();
    std::string maker_code();

    uint8_t unit_code();
    uint8_t encryption_seed();
    uint32_t capacity();
    uint8_t version();
    uint8_t autostart();

    uint32_t arm9_rom_offset();
    uint32_t arm9_entry_address();
    uint32_t arm9_ram_address();
    uint32_t arm9_size();

    uint32_t arm7_rom_offset();
    uint32_t arm7_entry_address();
    uint32_t arm7_ram_address();
    uint32_t arm7_size();

    uint32_t file_name_table();
    uint32_t file_name_size();
    uint32_t file_alloc_table();
    uint32_t file_alloc_size();

    uint32_t arm9_overlay_offset();
    uint32_t arm9_overlay_size();
    uint32_t arm7_overlay_offset();
    uint32_t arm7_overlay_size();

    uint32_t command_port_normal();
    uint32_t command_port_key1();

    uint32_t icon_title_offset();

    uint16_t secure_checksum();
    uint16_t secure_loading_timeout();

    uint32_t arm9_auto_load();
    uint32_t arm7_auto_load();

    uint64_t secure_area_disable();

    uint32_t size_used();
    uint32_t header_size();

    void set_fnt_offset(uint32_t);
    void set_fnt_size(uint32_t);
    void set_fat_offset(uint32_t);
    void set_fat_size(uint32_t);

    void set_arm9_offset(uint32_t);
    void set_arm9_overlay_offset(uint32_t);
    void set_arm7_offset(uint32_t);
    void set_arm7_overlay_offset(uint32_t);

    enum Offset
    {
      Title = 0x00,
      GameCode = 0x0C,
      MakerCode = 0x10,
      UnitCode = 0x12,
      EncryptionSeed = 0x13,
      Capacity = 0x14,
      Version = 0x1E,
      AutoStart = 0x1F,
      ARM9Rom = 0x20,
      ARM9Entry = 0x24,
      ARM9RAM = 0x28,
      ARM9Size = 0x2C,
      ARM7Rom = 0x30,
      ARM7Entry = 0x34,
      ARM7RAM = 0x38,
      ARM7Size = 0x3C,
      FileTableOffset = 0x40,
      FileTableSize = 0x44,
      FileAllocationOffset = 0x48,
      FileAllocationSize = 0x4C,
      ARM9Overlay = 0x50,
      ARM9OverlaySize = 0x54,
      ARM7Overlay = 0x58,
      ARM7OverlaySize = 0x5C,
      CommandPortNormal = 0x60,
      CommandPortKey1 = 0x64,
      IconTitle = 0x68,
      SecureChecksum = 0x6C,
      SecureLoadingTimeout = 0x6E,
      ARM9AutoLoadRAM = 0x70,
      ARM7AutoLoadRAM = 0x74,
      SecureAreaDisable = 0x78,
      SizeUsed = 0x80,
      HeaderSize = 0x84,
      Logo = 0xC0,
      LogoChecksum = 0x15C,
      HeaderChecksum = 0x15E,
      DebugRomOffset = 0x160,
      DebugSize = 0x164,
      DebugRAMAddress = 0x168,
    };
  private:
    std::vector<uint8_t> m_header;
  };

  inline std::vector<uint8_t> Header::get_raw()
  {
    return m_header;
  }

  inline std::string Header::title()
  {
    return std::string(m_header.begin() + Offset::Title, m_header.begin() + Offset::Title + 12);
  }

  inline std::string Header::game_code()
  {
    return std::string(m_header.begin() + Offset::GameCode, m_header.begin() + Offset::GameCode + 4);
  }

  inline std::string Header::maker_code()
  {
    return std::string(m_header.begin() + Offset::MakerCode, m_header.begin() + Offset::MakerCode + 2);
  }

  inline uint8_t Header::unit_code()
  {
    return m_header[Offset::UnitCode];
  }

  inline uint8_t Header::encryption_seed()
  {
    return m_header[Offset::EncryptionSeed];
  }

  inline uint32_t Header::capacity()
  {
    return 0x20000 << m_header[Offset::Capacity];
  }

  inline uint8_t Header::version()
  {
    return m_header[Offset::Version];
  }

  inline uint8_t Header::autostart()
  {
    return m_header[Offset::AutoStart];
  }

  inline uint32_t Header::arm9_rom_offset()
  {
    return util::read<uint32_t>(m_header, Offset::ARM9Rom);
  }

  inline uint32_t Header::arm9_entry_address()
  {
    return util::read<uint32_t>(m_header, Offset::ARM9Entry);
  }

  inline uint32_t Header::arm9_ram_address()
  {
    return util::read<uint32_t>(m_header, Offset::ARM9RAM);
  }

  inline uint32_t Header::arm9_size()
  {
    return util::read<uint32_t>(m_header, Offset::ARM9Size);
  }

  inline uint32_t Header::arm7_rom_offset()
  {
    return util::read<uint32_t>(m_header, Offset::ARM7Rom);
  }

  inline uint32_t Header::arm7_entry_address()
  {
    return util::read<uint32_t>(m_header, Offset::ARM7Entry);
  }

  inline uint32_t Header::arm7_ram_address()
  {
    return util::read<uint32_t>(m_header, Offset::ARM7RAM);
  }

  inline uint32_t Header::arm7_size()
  {
    return util::read<uint32_t>(m_header, Offset::ARM7Size);
  }

  inline uint32_t Header::file_name_table()
  {
    return util::read<uint32_t>(m_header, Offset::FileTableOffset);
  }

  inline uint32_t Header::file_name_size()
  {
    return util::read<uint32_t>(m_header, Offset::FileTableSize);
  }

  inline uint32_t Header::file_alloc_table()
  {
    return util::read<uint32_t>(m_header, Offset::FileAllocationOffset);
  }

  inline uint32_t Header::file_alloc_size()
  {
    return util::read<uint32_t>(m_header, Offset::FileAllocationSize);
  }

  inline uint32_t Header::arm9_overlay_offset()
  {
    return util::read<uint32_t>(m_header, Offset::ARM9Overlay);
  }

  inline uint32_t Header::arm9_overlay_size()
  {
    return util::read<uint32_t>(m_header, Offset::ARM9OverlaySize);
  }

  inline uint32_t Header::arm7_overlay_offset()
  {
    return util::read<uint32_t>(m_header, Offset::ARM7Overlay);
  }

  inline uint32_t Header::arm7_overlay_size()
  {
    return util::read<uint32_t>(m_header, Offset::ARM7OverlaySize);
  }

  inline uint32_t Header::command_port_normal()
  {
    return util::read<uint32_t>(m_header, Offset::CommandPortNormal);
  }

  inline uint32_t Header::command_port_key1()
  {
    return util::read<uint32_t>(m_header, Offset::CommandPortKey1);
  }

  inline uint32_t Header::icon_title_offset()
  {
    return util::read<uint32_t>(m_header, Offset::IconTitle);
  }

  inline uint16_t Header::secure_checksum()
  {
    return util::read<uint16_t>(m_header, Offset::SecureChecksum);
  }

  inline uint16_t Header::secure_loading_timeout()
  {
    return util::read<uint16_t>(m_header, Offset::SecureLoadingTimeout);
  }

  inline uint32_t Header::arm9_auto_load()
  {
    return util::read<uint32_t>(m_header, Offset::ARM9AutoLoadRAM);
  }

  inline uint32_t Header::arm7_auto_load()
  {
    return util::read<uint32_t>(m_header, Offset::ARM7AutoLoadRAM);
  }

  inline uint64_t Header::secure_area_disable()
  {
    return util::read<uint64_t>(m_header, Offset::SecureAreaDisable);
  }

  inline uint32_t Header::size_used()
  {
    return util::read<uint32_t>(m_header, Offset::SizeUsed);
  }

  inline uint32_t Header::header_size()
  {
    return util::read<uint32_t>(m_header, Offset::HeaderSize);
  }

  inline void Header::set_fnt_offset(uint32_t value)
  {
    util::write_int(m_header, value, Offset::FileTableOffset);
  }

  inline void Header::set_fnt_size(uint32_t value)
  {
    util::write_int(m_header, value, Offset::FileTableSize);
  }

  inline void Header::set_fat_offset(uint32_t value)
  {
    util::write_int(m_header, value, Offset::FileAllocationOffset);
  }

  inline void Header::set_fat_size(uint32_t value)
  {
    util::write_int(m_header, value, Offset::FileAllocationSize);
  }

  inline void Header::set_arm9_offset(uint32_t value)
  {
    util::write_int(m_header, value, Offset::ARM9Rom);
  }

  inline void Header::set_arm9_overlay_offset(uint32_t value)
  {
    util::write_int(m_header, value, Offset::ARM9Overlay);
  }

  inline void Header::set_arm7_offset(uint32_t value)
  {
    util::write_int(m_header, value, Offset::ARM7Rom);
  }

  inline void Header::set_arm7_overlay_offset(uint32_t value)
  {
    util::write_int(m_header, value, Offset::ARM7Overlay);
  }

}

#endif
