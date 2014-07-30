mdnds
===

The purpose of this tool is to make it easier to extract and build NDS ROMs with command line tools.

### Commands

To extract all the files from the disc into a directory:

    extract file.nds output/directory/path

**Note:** Build is not yet supported.
To build a ROM you must pass in a directory that has had the contents of a ROM extracted to it previously. If it detects missing files or improper structure it will not build anything.
    
    build previously/extracted/directory output.nds
    
Files will simply list the contents of the disc to the console.

    files file.nds

#### Aliases

For convinience there are single letter aliases for all the commands.

|Command|Alias|
|-------|----:|
|extract|   e |
|build  |   b |
|files  |   f |
