#include "nds.h"

#include <chrono>

using namespace nds;

namespace fs = boost::filesystem;

namespace nds
{
  void extract(std::string disc, std::string dir)
  {
    std::string sysdir = dir + "/sys/";
    std::string filedir = dir + "/files/";
    std::string overlaydir = dir + "/overlay/";

    //  Create the directories required
    boost::filesystem::create_directories(dir);
    boost::filesystem::create_directories(sysdir);
    boost::filesystem::create_directories(filedir);
    boost::filesystem::create_directories(overlaydir);

    Header header(util::read_file(disc, 0x200));
    FST table(disc);
    uint16_t start_id = table.start_id();
    auto fat = table.get_fat();

    //  Extract all the important components that will be put back if re-built
    util::write_file(sysdir + "header.bin", util::read_file(disc, 0x200));
    util::write_file(sysdir + "fnt.bin", table.get_fnt());
    util::write_file(sysdir + "fat.bin", fat);
    util::write_file(sysdir + "arm9_overlay.bin", util::read_file(disc, header.arm9_overlay_size(), header.arm9_overlay_offset()));
    util::write_file(sysdir + "arm7_overlay.bin", util::read_file(disc, header.arm7_overlay_size(), header.arm7_overlay_offset()));
    util::write_file(sysdir + "arm9.bin", util::read_file(disc, header.arm9_size(), header.arm9_rom_offset()));
    util::write_file(sysdir + "arm7.bin", util::read_file(disc, header.arm7_size(), header.arm7_rom_offset()));

    //  Extract all the overlay files
    for (uint16_t i = 0; i < start_id; i++)
    {
      uint32_t start = util::read<uint32_t>(fat, i * 8);
      uint32_t end = util::read<uint32_t>(fat, i * 8 + 4);

      std::cout << "Writing file: overlay_" << util::zero_pad(i, 4) << std::endl;

      util::write_file(overlaydir + "overlay_" + util::zero_pad(i, 4) + ".bin", util::read_file(disc, end - start, start));
    }

    //  Extract the files
    for (auto& file : table.files())
    {
      std::cout << "Writing file: " << file.path() << std::endl;

      extract_file(file, disc, filedir);
    }
  }

  void extract_file(FileEntry& file, std::string disc, std::string filedir)
  {
    fs::path basepath = fs::path(file.path()).parent_path();

    //  Make sure the path the file is written to is made
    fs::create_directories(filedir + basepath.string());

    //  Write the file
    util::write_file(filedir + file.path(), util::read_file(disc, file.size(), file.begin()));
  }

  void build(std::string dir, std::string disc)
  {
    std::string sysdir = dir + "/sys/";
    std::string filedir = dir + "/files/";
    std::string overlaydir = dir + "/overlay/";

    std::vector<uint8_t> nds(0x4000);

    std::vector<uint8_t> headerbin = util::read_file(sysdir + "header.bin", 0x200);
    std::vector<uint8_t> arm9 = util::read_file(sysdir + "arm9.bin");
    std::vector<uint8_t> arm9_overlay = util::read_file(sysdir + "arm9_overlay.bin");
    std::vector<uint8_t> arm7 = util::read_file(sysdir + "arm7.bin");
    std::vector<uint8_t> arm7_overlay = util::read_file(sysdir + "arm7_overlay.bin");

    Header header(headerbin);

    //  Add ARM9 bin
    header.set_arm9_offset(0x4000);
    std::copy(arm9.begin(), arm9.end(), std::back_inserter(nds));
    util::pad(nds, util::pad(nds.size(), 0x18));

    //  Add ARM9 overlay
    header.set_arm9_overlay_offset(nds.size());
    std::copy(arm9_overlay.begin(), arm9_overlay.end(), std::back_inserter(nds));
    util::pad(nds, util::pad(nds.size(), 0x100), 0xFF);  //  Pad to 0x100 since overlay files must be on a 0x100 mark


    //  Store the overlay FAT entries separate so we can add them to the real FAT later
    std::vector<uint8_t> overlay_fat;
    uint32_t overlay_count = 0;

    for (fs::directory_iterator dir(overlaydir), end; dir != end; ++dir)
    {
      if (fs::is_regular_file(dir->path()) && fs::extension(dir->path()) == ".bin")
      {
        auto file = util::read_file(dir->path().string());

        util::push_int(overlay_fat, nds.size());                //  Start address in ROM
        util::push_int(overlay_fat, nds.size() + file.size());  //  End address in ROM

        std::copy(file.begin(), file.end(), std::back_inserter(nds));
        //util::pad(nds, util::pad(nds.size(), 0x100), 0xFF);  //  Pad to 0x100 since overlay files must be on a 0x100 mark

        overlay_count++;
      }
    }

    util::pad(nds, util::pad(nds.size(), 0x1000));  //  Pad to 0x1000 since ARM7 code must be on an even 0x1000 mark
    //  Add ARM7 bin
    header.set_arm7_offset(nds.size());
    std::copy(arm7.begin(), arm7.end(), std::back_inserter(nds));

    //  Add ARM7 overlay
    if (arm7_overlay.size() > 0)
    {
      header.set_arm7_overlay_offset(nds.size());
      std::copy(arm7_overlay.begin(), arm7_overlay.end(), std::back_inserter(nds));
    }

    FST fst(filedir, nds.size(), overlay_count);

    std::vector<uint8_t> fnt = fst.get_fnt();
    std::vector<uint8_t> fat = fst.get_fat();

    //  Add overlays to the FAT
    std::copy(fat.begin(), fat.end(), std::back_inserter(overlay_fat));
    fat = overlay_fat;

    //  Add FNT
    header.set_fnt_offset(nds.size());
    header.set_fnt_size(fnt.size());
    std::copy(fnt.begin(), fnt.end(), std::back_inserter(nds));
    util::pad(nds, util::pad(nds.size(), 4));

    //  Add FAT
    header.set_fat_offset(nds.size());
    header.set_fat_size(fat.size());
    std::copy(fat.begin(), fat.end(), std::back_inserter(nds));
    //util::pad(nds, util::pad(nds.size(), 0x10));

    //  Write all files to the disc
    add_files(nds, filedir);

    //  Write the header data
    headerbin = header.get_raw();
    std::copy(headerbin.begin(), headerbin.end(), nds.begin());

    util::pad(nds, util::pad(nds.size(), header.capacity()), 0xFF);

    util::write_file(disc, nds);
  }

  void add_files(std::vector<uint8_t>& rom, std::string root)
  {
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_regular_file(dir->path()))
      {
        std::cout << "Adding " << dir->path().filename().string() << " at offset " << std::hex << rom.size() << std::dec << std::endl;
        auto data = util::read_file(dir->path().string());
        std::copy(data.begin(), data.end(), std::back_inserter(rom));

        util::pad(rom, util::pad(rom.size(), 4), 0xFF);
      }
    }

    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_directory(dir->path()))
      {
        add_files(rom, dir->path().string());
      }
    }
  }

  void files(std::string disc)
  {
    FST table(disc);

    //  Print out each file path
    for (auto& file : table.files())
    {
      std::cout << "./" << file.path() << std::endl;
    }
  }


  /*
    Summary:
      Determines whether a given root directory has the required structure and files to be used to create a GCM

    Parameter:
      root: Directory to check

    Returns:
      True if the directory can be used to create a GCM, or false otherwise.
  */
  bool valid_directory(std::string root)
  {
    bool ret = true;

    //  If the directory does not have /files and /sys then it wasn't extracted by this extractor
    if (fs::is_directory(root) == false)
    {
      std::cout << root << " is not a valid directory." << std::endl;
      ret = false;
    }

    if (fs::is_directory(root + "/files") == false)
    {
      std::cout << "Missing directory /files under " << root << std::endl;
      ret = false;
    }

    if (fs::is_directory(root + "/sys") == false)
    {
      std::cout << "Missing directory /sys under " << root << std::endl;
      ret = false;
    }

    if (fs::is_directory(root + "/overlay") == false)
    {
      std::cout << "Missing directory /overlay under " << root << std::endl;
      ret = false;
    }

    //  Check for individual system files which are required to re-build the disc
    if (fs::is_regular_file(root + "/sys/arm9_overlay.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/arm9_overlay.bin" << std::endl;
      ret = false;
    }

    if (fs::is_regular_file(root + "/sys/arm7_overlay.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/arm7_overlay.bin" << std::endl;
      ret = false;
    }

    if (fs::is_regular_file(root + "/sys/arm9.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/arm9.bin" << std::endl;
      ret = false;
    }

    if (fs::is_regular_file(root + "/sys/arm7.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/arm7.bin" << std::endl;
      ret = false;
    }

    if (fs::is_regular_file(root + "/sys/fnt.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/fnt.bin" << std::endl;
      ret = false;
    }

    if (fs::is_regular_file(root + "/sys/fat.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/fat.bin" << std::endl;
      ret = false;
    }

    if (fs::is_regular_file(root + "/sys/header.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/header.bin" << std::endl;
      ret = false;
    }

    return ret;
  }
}