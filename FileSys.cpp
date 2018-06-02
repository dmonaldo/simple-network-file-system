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
 /*
   ignore all of this
   
  dirblock_t directory;
  directory.magic = DIR_MAGIC_MUM;
  directory.num = entries = 0;
  for(int i = 0; i < MAX_DIR_ENTRIES; i++){
    directory_block.dir_entries[i].block_num = 0;
  }
  directory_block.dir_entries[0].name = name;
  directory_block.dir_entries[0].block_num = bfs.get_free_block();
  disk.write_block(directory, (void *) &directory);
  
  
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
void FileSys::home()
{
  struct dirblock_t dirblock;
  bfs.read_block(1, (void *) & dirblock);
}

// remove a directory
void FileSys::rmdir(const char *name)
{
}

// list the contents of current directory
void FileSys::ls()
{
}

// create an empty data file
void FileSys::create(const char *name)
{
  if(strlen(name) > MAX_FNAME_SIZE + 1){
    // 504 File name is too long
    return;
  }
  // read current directory for duplicate name
  dirblock_t* curr_block_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE + 1];
  char curr_file_name[MAX_FNAME_SIZE + 1];
  strcpy(file_name, name);
  bfs.read_block(curr_dir, (void*) &curr_block_ptr);

  if(curr_block_ptr->num_entries == MAX_DIR_ENTRIES){
    delete curr_block_ptr;
    //cout << "506 Directory is full." << endl;
    return;
  }

  for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);
    if(strcmp(curr_file_name, file_name) == 0){
      delete curr_block_ptr;
      //cout << "502 File exists" << endl;
      return;
    }
  }

  // make new free block
  short block_num = bfs.get_free_block();
  if(block_num == 0){
    block_num = bfs.get_free_block();
    if(block_num == 0){
      //cout << "505 Disk is full" << endl;
      delete curr_block_ptr;
      return;
    }
  }
  
  // fill new inode block_num to hold 0 to show blocks are unused
  inode_t* curr_dir_inode = new inode_t;
  // initalize timestamps?
  // set permissions/modes?
  curr_dir_inode->magic = INODE_MAGIC_NUM;
  curr_dir_inode->size = 0;
  for(int k = 0; k < MAX_DATA_BLOCKS; k++){
    curr_dir_inode->blocks[k] = 0; // everything unused
  }
  // then write to the block
  bfs.write_block(block_num, (void*) &curr_dir_inode);
  delete curr_dir_inode;

  strcpy(curr_block_ptr->dir_entries[curr_block_ptr->num_entries].name, name);
  curr_block_ptr->dir_entries[curr_block_ptr->num_entries].block_num =
    block_num;
  curr_block_ptr->num_entries++;

  //write block and delete
  bfs.write_block(curr_dir, (void*) &curr_block_ptr);
  delete curr_block_ptr;
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
  bool dir_check;
  int counter = 0;
  int first_block;
  bool check_first_block = false;
  dirblock_t* curr_block_ptr = new dirblock_t;
  
  // call is_directory
  
  //dir_check = is_directory(name);
  //if(dir_check){
  
    //cout << "Directory name: " << curr_block_ptr->dir_entries
    //[curr_block_ptr->num_entries].name << endl;
    //cout << "Directory block: " << curr_block_ptr->dir_entries
    // [curr_block_ptr->num_entries].block_num << endl;                                                           
  //}
  //else{
    //data file
    // determine number of blocks
    inode_t* new_inode = new inode_t;
    for(int k = 0; k < MAX_DATA_BLOCKS; k++){
      if(new_inode->blocks[k] != 0){
        counter = counter + 1;
        // determine first block
        if(!check_first_block){
          first_block = k;
          check_first_block = true;
        }
      }
    }
   /*
    cout << "Bytes in file: " << new_inode->size << endl;
    cout << "Number of blocks: " << counter << endl;
    cout << "First block: " << first_block << endl;
   */
}

// HELPER FUNCTIONS (optional)
/*
bool FileSys::is_directory(const char *name)
{
  if(dirblock_t name->magic == DIR_MAGIC_NUM){
    return true;
  }
  else{
    return false;
  }
}
*/
