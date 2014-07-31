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

    for (uint32_t i = 0; i < total; i++)
    {
      m_table_entries.push_back(TableEntry(util::subset(m_fnt, i * 8, 8)));
    }

    for (auto& entry : m_table_entries)
    {
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
  FST::FST(std::string root, uint32_t fst_offset)
  {

    uint32_t dir_index = 1;     //  Start at 1 since 0 is root
    uint32_t file_index = 0;    //  ID of the next file
    uint32_t table_offset = 0;  //  Offset into the string table

    //  Sub tables start after all main table entries which are 8 bytes each
    uint32_t sub_table_offset = total_directories(root, true) * 8;

    for (fs::recursive_directory_iterator dir(root), end; dir != end; ++dir)
    {
      if (fs::is_directory(dir->path()))
      {
        uint32_t parent_id = m_dir_parent[dir->path().parent_path()].parent();
        m_dir_parent[dir->path()] = TableEntry(sub_table_offset + table_offset, file_index, dir_index);

        ++dir_index;
      }
      else
      {
        ++file_index;
      }

      //  Increase offset by the size this entry would take in the string table
      table_offset += dir->path().filename().string().length() + 1;
    }

    std::vector<uint8_t> string_table = create_string_table(root);
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
        util::push_int<uint16_t>(string_table, m_dir_parent[dir->path()].parent() | 0xF000);
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
