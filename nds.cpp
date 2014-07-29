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

    //  Create the directories required
    boost::filesystem::create_directories(dir);
    boost::filesystem::create_directories(sysdir);
    boost::filesystem::create_directories(filedir);

    Header header(util::read_file(disc, 0x200));

    //  Extract all the important components that will be put back if re-built
    util::write_file(sysdir + "header.bin", util::read_file(disc, 0x200));
    util::write_file(sysdir + "fnt.bin", util::read_file(disc, header.file_name_size(), header.file_name_table()));
    util::write_file(sysdir + "fat.bin", util::read_file(disc, header.file_alloc_size(), header.file_alloc_table()));
    util::write_file(sysdir + "arm9_overlay.bin", util::read_file(disc, header.arm9_overlay_size(), header.arm9_overlay_offset()));
    util::write_file(sysdir + "arm7_overlay.bin", util::read_file(disc, header.arm7_overlay_size(), header.arm7_overlay_offset()));
    util::write_file(sysdir + "arm9.bin", util::read_file(disc, header.arm9_size(), header.arm9_rom_offset()));
    util::write_file(sysdir + "arm7.bin", util::read_file(disc, header.arm7_size(), header.arm7_rom_offset()));

    FST table(disc);

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

    if (fs::is_regular_file(root + "/sys/fst.bin") == false)
    {
      std::cout << "Missing file " << root << "/sys/fst.bin" << std::endl;
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