#ifndef _MD_NDS_H
#define _MD_NDS_H

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <thread>

#include "nds_header.h"
#include "nds_fst.h"

namespace nds
{
  void extract(std::string disc, std::string dir);
  void extract_file(FileEntry& file, std::string disc, std::string filedir);
  void build(std::string dir, std::string disc);
  void files(std::string disc);

  bool valid_directory(std::string dir);
}

#endif
