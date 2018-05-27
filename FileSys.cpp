// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <unistd.h>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount(int sock) {
  bfs.mount();
  curr_dir = 1; //by default current directory is home directory, in disk block #1
  fs_sock = sock; //use this socket to receive file system operations from the client and send back response messages
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
  close(fs_sock);
}

// make a directory
void FileSys::mkdir(const char *name)
{
  dirblock_t directory;
  directory.magic = DIR_MAGIC_MUM;
  directory.num = entries = 0;
  for(int i = 0; i < MAX_DIR_ENTRIES; i++){
    directory_block.dir_entries[i].block_num = 0;
  }
  directory_block.dir_entries[0].name = name;
  directory_block.dir_entries[0].block_num = bfs.get_free_block();
  disk.write_block(directory, (void *) &directory);
  
  /*
  inode_t = directory_inode;
  directory_inode.magic = INODE_MAGIC_NUM; // magic number
  //directory_inode.size = MAX_FILE_SIZE;  // file size in bytes
  directory_inode.blocks[MAX_DATA_BLOCKS]; // array of direct indices to data
  // blocks
  contents = {name, directory_inode};
  */
}

// switch to a directory
void FileSys::cd(const char *name)
{
}

// switch to home directory
void FileSys::home() {
}

// remove a directory
void FileSys::rmdir(const char *name)
{
}

// list the contents of current directory
void FileSys::ls()
{
  //home();
  // list contents in this directory
  dirblock_t currdir.dir_entries[MAX_DIR_ENTRIES];
}

// create an empty data file
void FileSys::create(const char *name)
{
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
}

// delete a data file
void FileSys::rm(const char *name)
{
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
}

// HELPER FUNCTIONS (optional)

