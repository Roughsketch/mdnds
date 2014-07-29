/*
    Originally created by Maiddog <maiddogsrl@gmail.com> 7/23/2014

    GCM v1.0 - 7/20/2014
    Support for extraction, re-building, and file listing


    Copyright (c) 2014 maiddog.com

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    3. The name of the author may not be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
    THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>
#include <boost/filesystem.hpp>

#include "nds.h"

void usage()
{
  std::cout << "Usage: nds.exe <Command> <Root> <Output>";
  std::cout << R"DOC(
    <Command>: "build"|"b" or "extract"|"e" or "files"|"f"
    <Root>   : Build: Directory where a disc was previously extracted
               Extract: Path to the disc to extract from
               Files: Path to the disc
    <Output> : Build: Output file path and name
               Extract: Output directory where files will be extracted
    Examples:
      nds.exe extract Example.nds output_dir
      nds.exe build output_dir RebuiltExample.nds
  )DOC" << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    usage();
    exit(EXIT_FAILURE);
  }

  std::string cmd(argv[1]);   //  Command comes first

  if (argc == 4 && (cmd == "build" || cmd == "b"))
  {
    std::string root(argv[2]);  //  Root directory or file path
    std::string out(argv[3]);   //  Output directory or file path
    if (nds::valid_directory(root))
    {
      nds::build(root, out);
    }
    else
    {
      std::cout << "Invalid directory.";
      exit(EXIT_FAILURE);
    }
  }
  else if (argc == 4 && (cmd == "extract" || cmd == "e"))
  {
    std::string root(argv[2]);  //  Root directory or file path
    std::string out(argv[3]);   //  Output directory or file path
    nds::extract(root, out);
  }
  else if (argc == 3 && (cmd == "files" || cmd == "f"))
  {
    std::string root(argv[2]);  //  Root directory or file path
    nds::files(root);
  }
  else
  {
    std::cout << "Invalid command: " << cmd << std::endl;
    usage();
    exit(EXIT_FAILURE);
  }


  return EXIT_SUCCESS;
}