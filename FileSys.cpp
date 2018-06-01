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
  curr_dir = 1; //by default current directory is home directory,
  //in disk block #1
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
  if(strlen(name) > MAX_FNAME_SIZE +1){
    cout << "504: File name is too long\n";
    return;
  }
  // read current directory for duplicate name
  dirblock_t* curr_block_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE + 1];
  char curr_file_name[MAX_FNAME_SIZE + 1];
  strcpy(file_name, name);
  bfs.read_block(curr_dir, (void*)&curr_block_ptr);
  
  if (curr_block_ptr->num_entries == MAX_DIR_ENTRIES){
    cout << "506: Directory is full.\n";
    delete curr_block_ptr;
    return;
  }

  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);

    if (strcmp(curr_file_name, file_name) == 0){
      cout << "502: File exists\n";
      delete curr_block_ptr;
      return;
    }
  }
  

  // make new free block 
  short block_num = bfs.get_free_block();
  if (block_num == 0){
    block_num = bfs.get_free_block();
    if (block_num == 0){
      cout << "505: Disk is full.\n";
      delete curr_block_ptr;
      return;
    }
  }
    
  //fill new directory block_num to hold 0 to show blocks are unused
  dirblock_t* new_block = new dirblock_t;
  new_block->magic = DIR_MAGIC_NUM;
  new_block->num_entries = 0;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++){
    new_block->dir_entries[i].block_num = 0;
  }
  bfs.write_block(block_num, (void*)&new_block);
  delete new_block;

  strcpy(curr_block_ptr->dir_entries[curr_block_ptr->num_entries].name, name);
  curr_block_ptr->dir_entries[curr_block_ptr->num_entries].block_num =
    block_num;
  curr_block_ptr->num_entries++;

  // write block and delete
  bfs.write_block(curr_dir, (void*)&curr_block_ptr);
  delete curr_block_ptr;
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
  //removes a sub directory from the current directory aslong as the
  //subdirectory is empty
  //check subdirectory exists in current durrectory
  dirblock_t* curr_block_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE + 1];
  char curr_file_name[MAX_FNAME_SIZE + 1];
  strcpy(file_name, name);
  bfs.read_block(curr_dir, (void*)&curr_block_ptr);
  
  //if (curr_block_ptr->num_entries == MAX_DIR_ENTRIES){
  //delete curr_block_ptr;
  //cout << "Directory is full.\n";
  //return;
  //}
  short found_block;
  unsigned int found_index;
  bool empty = false;
  bool found = false;
  dirblock_t* found_dir_ptr = new dirblock_t
  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);
    if (strcmp(curr_file_name, file_name) == 0){
      found = true;
      found_index = i;
      found_block = curr_block_ptr->dir_entries[i].block_num;
      bfs.read_block(curr_block_ptr->dir_entries[i].block_num,
                     (void*)&found_dir_ptr);
    }
  }
  if(!is_directory(found_dir_ptr->magic)){
    cout << "500: File is not a directory\n";
    delete found_dir_ptr;
    delete curr_dir_ptr;
    return;
  }
  else if(!found){
    cout << "503: File does not exsit\n";
    delete found_dir_ptr;
    delete curr_dir_ptr;
    return;
  }
  else if(found_dir_ptr->num_entries != 0){
    cout << "507: Directory is not empty\n" << endl;
    delete curr_block_ptr;
    delete found_dir_ptr;
    return;
  }
  else{
    bfs.reclaim_block(curr_block_ptr->dir_entries[found_index].block_num);
    curr_block_ptr->dir_entries[found_index].name[0] = '\0';
    curr_block_ptr->dir_entries[found_index].block_num = 0;
    curr_block_ptr->num_entries--;
    bfs.write_block(curr_dir, (void*)&curr_dir_ptr);
  }
  
  delete found_dir_ptr;
  delete curr_block_ptr;
  return;
    
}

// list the contents of current directory
void FileSys::ls()
{
  //read the data held at the current directory from the disk
  dirblock_t* curr_block_ptr = new dirblock_t;
  char curr_file_name[MAX_FNAME_SIZE + 1];
  bfs.read_block(curr_dir, curr_block_ptr);

  if (curr_block_ptr->num_entries == 0){
    delete curr_block_ptr;
    cout << "Directory is empty.\n";
    return;
  }

  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    if(curr_dir_ptr->dir_entries[i].block_num != 0){
      //read the name of the file by charter until null reached
    }
    //read each file name entry as long a block for file is not equal to 0
    //strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);
    //put the curr_file_name into socket buffer to send text format to client
  }
  delete curr_block_ptr;
  return;
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

