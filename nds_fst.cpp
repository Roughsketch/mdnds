#include "nds_fst.h"

namespace fs = boost::filesystem;

namespace nds
{
  FST::FST(std::string disc)
  {
    Header header(util::read_file(disc, 0x200));
    m_fat = util::read_file(disc, header.file_alloc_size(), header.file_alloc_table());
    m_fnt = util::read_file(disc, header.file_name_size(), header.file_name_table());

    std::vector<std::string> current_directory = { "." };         //  The current directory that is being read from
    std::map<uint16_t, std::string> directories = { { 0, "." } }; //  A map of all directories <dir_id, name>

    uint32_t dir_index = 0;                           //  Current directory index
    uint32_t total = util::read<uint16_t>(m_fnt, 6);  //  Total entries

    std::cout << "Storing main table entries" << std::endl;

    for (uint32_t i = 0; i < total; i++)
    {
      m_table_entries.push_back(TableEntry(util::subset(m_fnt, i * 8, 8)));
    }

    std::cout << "Iterating main table entries" << std::endl;
    for (auto& entry : m_table_entries)
    {
      std::cout << "dir_index: " << dir_index << std::endl;

      uint32_t file_id = entry.first_id();  //  The starting file id in this folder
      uint32_t subdirs = 0;                 //  The amount of sub directories in this folder

      //  Offsets are relative to the FNT offset
      uint32_t offset = entry.offset();

      //  0 indicates the section has ended
      while (m_fnt[offset] != 0)
      {
        std::string entry = "";       //  Holds the current entry identifier
        uint8_t len = m_fnt[offset];  //  First byte is the length of the file/directory name

        //  If length is 0x01 to 0x7F then it is a file entry
        if (len > 0x00 && len < 0x80)
        {
          entry = util::read(m_fnt, offset + 1, len);         //  Get the file name
          entry = current_directory[dir_index] + "/" + entry; //  Prepend the directory structure before the file name
          offset += len + 1;                                  //  Offset increase by file name length + length byte

          //  File Allocation Table holds pairs of integers noting start and end offsets into the rom.
          uint32_t file_start = util::read<uint32_t>(m_fat, file_id * 8);
          uint32_t file_end = util::read<uint32_t>(m_fat, file_id * 8 + 4);

          //  Create new NDSEntry with file start offset and file size and store it into the entries map
          m_entries.push_back(FileEntry(entry, file_start, file_end));

          //  Next file id
          file_id++;
        }
        //  If the length is 0x81 to 0xFF then it is a directory entry
        else if (len > 0x80)
        {
          len -= 0x80;                                //  Subtract 0x80 to get directory name length
          entry = util::read(m_fnt, offset + 1, len); //  Get the directory name

          //  Directories have an extra 2 bytes indicating the directory id
          uint16_t id = util::read<uint16_t>(m_fnt, offset + len + 1) & 0x0FFF; //  id is in form 0xFXXX so bitwise and it with 0xFFF

          offset += len + 3;  //  Increase offset by length byte + 2 bytes for id + name length

          directories[id] = entry;  //  Add this directory to the list

          //  If current parent directory is root
          if (dir_index == 0)
          {
            //  Push back root identifier + new directory name
            current_directory.push_back("./" + entry);
          }
          else
          {
            //  Place this directory under its parent
            current_directory.insert(current_directory.begin() + dir_index + subdirs + 1, current_directory[dir_index] + "/" + entry);
            subdirs++;
          }
        }
        else
        {
          //  0x00 and 0x80 are invalid
          //  0x00 indicates end of sub-table
          //  0x80 is reserved
        }
      }

      //  Increase directory index to point to the next directory name in current_directory
      dir_index++;
    }
  }

  /*
    Summary
      Builds a FST (FNT + FAT) from a given directory

    Parameters:
      root: The root directory to read files and directories from
      fst_offset: The offset where the start of the FST (FNT) will be in the ROM.
  */
  FST::FST(std::string root, uint32_t fst_offset, uint32_t file_id_offset)
  {
    file_id = file_id_offset;
    dir_id = 1;
    initialize_directory_table(root);

    m_fnt = create_main_table(root);
    std::vector<uint8_t> string_table = create_string_table(root);

    //  Append string table to the FNT
    std::copy(string_table.begin(), string_table.end(), std::back_inserter(m_fnt));

    //  Offset is FST offset + FNT size + FAT size (total files * size of FAT entry)
    file_offset = fst_offset + m_fnt.size() + ((file_id_offset + total_files(root, true)) * 8);
    file_offset += util::pad(file_offset, 4);
    create_allocation_table(root);
  }

  void FST::initialize_directory_table(std::string root)
  {
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_directory(dir->path()))
      {
        std::cout << dir_id << "\t" << dir->path().string() << std::endl;
        m_dir_parent[dir->path()] = 0xF000 | dir_id;
        ++dir_id;
        initialize_directory_table(dir->path().string());
      }
    }
  }

  std::vector<uint8_t> FST::create_main_table(std::string root, bool is_root)
  {
    std::vector<uint8_t> main_table;

    //  If this is the true root then add the root table entry
    if (is_root)
    {
      //  Sub table offset starts after all main table entries (8 bytes each) + root (8 bytes)
      table_offset = total_directories(root, true) * 8 + 8;
      util::push_int(main_table, table_offset);
      util::push_int<uint16_t>(main_table, file_id);
      util::push_int<uint16_t>(main_table, total_directories(root, true) + 1);
    }

    //  Count every file in the current directory
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      table_offset += dir->path().filename().string().length() + 1;

      if (fs::is_regular_file(dir->path()))
      {
        file_id++;
      }
      else
      {
        table_offset += 2; // Add 2 for the directory ID
      }
    }

    table_offset++; //  Add 1 for the 0 byte ending the sub-table

    //  Go through every sub-directory in the current directory
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_directory(dir->path()))
      {
        std::cout << "Parent of " << dir->path().string() << " is " << std::hex << m_dir_parent[dir->path().parent_path()] << std::dec << std::endl;
        util::push_int(main_table, table_offset);
        util::push_int<uint16_t>(main_table, file_id);
        util::push_int<uint16_t>(main_table, 0xF000 | m_dir_parent[dir->path().parent_path()]);

        //  Recursively append contents to the string table
        auto table = create_main_table(dir->path().string(), false);
        std::copy(table.begin(), table.end(), std::back_inserter(main_table));
      }
    }

    return main_table;
  }

  void FST::create_allocation_table(std::string root)
  {
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_regular_file(dir->path()))
      {
        util::push_int(m_fat, file_offset);
        file_offset += static_cast<uint32_t>(fs::file_size(dir->path()));
        util::push_int(m_fat, file_offset);

        file_offset += util::pad(file_offset, 4);
      }
    }

    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_directory(dir->path()))
      {
        create_allocation_table(dir->path().string());
      }
    }
  }

  /*
    Summary:
      Recursively goes through a given directory and generates a
      FNT string table from the contents

    Parameters:
      root: The root directory to make the table from

    Returns:
      The raw bytes of the string table which is to be placed after
      the main tables in the FNT.
  */
  std::vector<uint8_t> FST::create_string_table(std::string root)
  {
    std::vector<uint8_t> string_table;

    //  Go through every file in the current directory
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_regular_file(dir->path()))
      {
        std::string name = dir->path().filename().string();

        //  Push back the file name length as a single byte
        string_table.push_back(name.length());

        //  Push back the actual file name
        util::push(string_table, name);
      }
    }

    //  Go through every sub-directory in the current directory
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_directory(dir->path()))
      {
        std::string name = dir->path().filename().string();

        //  Push back the directory name length and add 0x80 to mark it as a directory entry
        string_table.push_back(name.length() + 0x80);

        //  Push back the actual directory name
        util::push(string_table, name);

        //  Push back the directory ID and set the MSB to 1 to mark it as a directory
        util::push_int<uint16_t>(string_table, 0xF000 | m_dir_parent[dir->path()]);
      }
    }

    //  Push back a 0 to mark the end of this sub-table
    string_table.push_back(0);

    //  For every sub-directory
    for (fs::directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_directory(dir->path()))
      {
        //  Recursively append contents to the string table
        auto table = create_string_table(dir->path().string());
        std::copy(table.begin(), table.end(), std::back_inserter(string_table));
      }
    }

    return string_table;
  }

  /*
    Summary:
      Counts how many files are in a directory

    Parameters:
      root: Directory to count files in
      recurse: If true the search will include all sub-directories

    Returns:
      Total number of files found
  */
  uint32_t FST::total_files(boost::filesystem::path root, bool recurse)
  {
    uint32_t ret = 0;

    if (recurse)
    {
      for (fs::recursive_directory_iterator dir(root), end; dir != end; ++dir)
      {
        if (fs::is_regular_file(dir->path()))
        {
          ++ret;
        }
      }
    }
    else
    {
      for (fs::directory_iterator dir(root), end; dir != end; ++dir)
      {
        if (fs::is_regular_file(dir->path()))
        {
          ++ret;
        }
      }
    }

    return ret;
  }

  /*
    Summary:
      Counts how many sub-directories are in a directory

    Parameters:
      root: Directory to count sub-directories in
      recurse: If true the search will include all sub-directories

    Returns:
      Total number of sub-directories found
  */
  uint32_t FST::total_directories(boost::filesystem::path root, bool recurse)
  {
    uint32_t ret = 0;

    if (recurse)
    {
      for (fs::recursive_directory_iterator dir(root), end; dir != end; ++dir)
      {
        if (fs::is_directory(dir->path()))
        {
          ++ret;
        }
      }
    }
    else
    {
      for (fs::directory_iterator dir(root), end; dir != end; ++dir)
      {
        if (fs::is_directory(dir->path()))
        {
          ++ret;
        }
      }
    }

    return ret;
  }
}
