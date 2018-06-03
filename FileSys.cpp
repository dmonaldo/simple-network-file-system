// CPSC 3500: File System
// Implements the file system commands that are available to the shell.
#include <sys/socket.h>
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
  fs_sock = sock; //use this socket to receive file system operations
  //from the client and send back response messages
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
  close(fs_sock);
}

// make a directory
void FileSys::mkdir(const char *name)
{
  char buffer[1024];
  bool error = false;
  read(fs_sock, buffer, 1024);
  if(strlen(name) > MAX_FNAME_SIZE +1){
    strcpy(buffer, "504: File name is too long\r\n");
    error = true;
  }

  // read current directory for duplicate name
  dirblock_t* curr_block_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE + 1];
  char curr_file_name[MAX_FNAME_SIZE + 1];
  strcpy(file_name, name);
  bfs.read_block(curr_dir, (void*)&curr_block_ptr);
  if(!error){
    if (curr_block_ptr->num_entries == MAX_DIR_ENTRIES){
      strcpy(buffer, "506: Directory is full\r\n");
      error = true;
      delete curr_block_ptr;
    }
  }
  
  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);
    
    if (strcmp(curr_file_name, file_name) == 0){
      if(!error){
        strcpy(buffer, "502: File exists\r\n");
        error = true;
        delete curr_block_ptr;
      }
    }
  }

  // make new free block 
  short block_num = bfs.get_free_block();
  if (block_num == 0){
    block_num = bfs.get_free_block();
    if (block_num == 0){
      if(!error){
        strcpy(buffer, "505: Disk is full\r\n");
        error = true;
        delete curr_block_ptr;
      }
    }
  }
    
  //fill new directory block_num to hold 0 to show blocks are unused
  if(!error){
    dirblock_t* new_block = new dirblock_t;
    new_block->magic = DIR_MAGIC_NUM;
    new_block->num_entries = 0;
    for (int i = 0; i < MAX_DIR_ENTRIES; i++){
      new_block->dir_entries[i].block_num = 0;
    }
    bfs.write_block(block_num, (void*)&new_block);
    delete new_block;
    strcpy(buffer, "200 ok\r\n Length: 0\r\n");
    strcpy(curr_block_ptr->dir_entries[curr_block_ptr->num_entries].name, name);
    curr_block_ptr->dir_entries[curr_block_ptr->num_entries].block_num =
      block_num;
    curr_block_ptr->num_entries++;

    // write block and delete
    bfs.write_block(curr_dir, (void*)&curr_block_ptr);
    delete curr_block_ptr;
  }
  send(fs_sock, buffer, strlen(buffer), 0);
}

// switch to a directory
void FileSys::cd(const char *name)
{
  bool error = false;
  bool found = false;
  char buffer[256];
  
  //retrieve current directory data block
  dirblock_t* curr_dir_block_ptr = new dirblock_t;
  bfs.read_block(curr_dir, (void *) &curr_dir_block_ptr);

  //check if any sub directories exist in current directory
  if(curr_dir_block_ptr->num_entries > 0){
    //check each sub directory and check directory names for match
    for(int i= 1; i <= curr_dir_block_ptr->num_entries; i++){
      if(strcmp(curr_dir_block_ptr->dir_entries[i].name, name) == 0){
        found = true;
        if(!is_directory(curr_dir_block_ptr->dir_entries[i].block_num)){
          error = true;
          strcpy(buffer, "500 File is not a directory");
        }
        else{
          curr_dir = curr_dir_block_ptr->dir_entries[i].block_num;
        }
      }
    }
  }
  // if this point reached, no matching directory found
  if(found && !error){
    strcat(buffer, "503 File does not exist");
  }
  delete curr_dir_block_ptr;
  send(fs_sock, buffer, strlen(buffer), 0);
}
// switch to home directory
void FileSys::home(){
  curr_dir = 1;
  
  //dirblock_t *dirblock;
  //bfs.read_block(1, (void *) & dirblock);
}

// remove a directory
void FileSys::rmdir(const char *name)
{
  dirblock_t* curr_block_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE + 1];
  char curr_file_name[MAX_FNAME_SIZE + 1];
  strcpy(file_name, name);
  bfs.read_block(curr_dir, (void*)&curr_block_ptr);
  bool error = false;  

  char buffer[1024];
  
  short found_block;
  unsigned int found_index;
  bool found = false;
  dirblock_t* found_dir_ptr = new dirblock_t;
  //finds the directory to remove wthin the current directory, sets found
  //bool to true if it exists in the directory
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
  //ensures the found directory is a directory and not a file
  if(!is_directory(found_dir_ptr->magic)){
    if(!error){
      error = true;
      strcpy(buffer, "500: File is not a directory\r\n");
    }
  }
  //if the found bool was not set to true then the filename passed in does
  //not exist and the error message is wrote into the buffer
  else if(!found && !error){
    error = true;
    strcpy(buffer ,"503: File does not exsit\r\n");
  }
  //if the found directory is a directory but still contains files it cannot
  //be removed and this error is wrote into the buffer
  else if(found_dir_ptr->num_entries != 0 && !error){
    error = true;
    strcpy(buffer, "507: Directory is not empty\r\n");
  }
  else{
    //if error is still set to false then remove the directory and set all
    //values back to null/0 as well as reclaiming its block num
    if(!error){
      bfs.reclaim_block(curr_block_ptr->dir_entries[found_index].block_num);
      curr_block_ptr->dir_entries[found_index].name[0] = '\0';
      curr_block_ptr->dir_entries[found_index].block_num = 0;
      curr_block_ptr->num_entries--;
      bfs.write_block(curr_dir, (void*)&curr_block_ptr);
      strcpy(buffer, "200 OK\r\n Length: 0\r\n");
    }
  }
  
  delete found_dir_ptr;
  delete curr_block_ptr;
  send(fs_sock, buffer, strlen(buffer), 0);
}

// list the contents of current directory
void FileSys::ls()
{
  char bufferStart[] = "200 OK\r\n";
  string buffer;
  char msg[200];
  char msgLength[80];
  //read the data held at the current directory from the disk
  dirblock_t* curr_block_ptr = new dirblock_t;
  char curr_file_name[MAX_FNAME_SIZE + 1];
  bfs.read_block(curr_dir, curr_block_ptr);

  int k = 0;
  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    k = 0;
    if(curr_block_ptr->dir_entries[i].block_num != 0){
      while((curr_block_ptr->dir_entries[i].name[k] != '\0') &&
            (k <= MAX_FNAME_SIZE)){
        buffer.push_back(curr_block_ptr->dir_entries[i].name[k]);
      }
      buffer.append(" ");
    }
  }
  strcpy(msg, bufferStart);
  sprintf(msgLength, "Length: %d", sizeof(buffer));
  strcat(msg, "\r\n");
  strcat(msg, buffer.c_str());
  delete curr_block_ptr;
  
  send(fs_sock, msg, sizeof(msg), 0);  
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
  bool error = false;
  bool found = false;

  // may need to increase buffer size to account for terminal messages
  char buffer [MAX_FILE_SIZE + 256];

  datablock_t* cat_file_contents = new datablock_t;
  dirblock_t* curr_dir_block_ptr = new dirblock_t;
  bfs.read_block(curr_dir, curr_dir_block_ptr);
  for(int i = 0; i  < MAX_DIR_ENTRIES; i ++){
    //target file found in current element in dir_entries
    if(strcmp(name,curr_dir_block_ptr->dir_entries[i].name)==0){
      found = true;
      if(is_directory(curr_dir_block_ptr->dir_entries[i].block_num)){
        strcpy(buffer, "501: File is a directory\r\n");
        error = true;
        // delete curr_dir_block_ptr;
        //delete cat_file_contents;
        //break;
      }
      else if(!is_directory(curr_dir_block_ptr->dir_entries[i].block_num)){
        inode_t* cat_file_inode = new inode_t;
        bfs.read_block(curr_dir_block_ptr->dir_entries[i].block_num,
                       cat_file_inode);

        string file_size_str = to_string(cat_file_inode->size);
        int file_byte_count = (file_size_str.length() + 1);
        char* file_size = new char[file_byte_count];
        strcpy(file_size, file_size_str.c_str());
        
        strcat(buffer, "200 OK\r\n Length:");
        strcat(buffer, file_size);
        strcat(buffer, "\r\n");
        strcat(buffer, "\r\n");
        delete file_size;
        
        //append each data block pointed to by inode_t.blocks[] to our buffer
        for(int k = 0; k < sizeof(cat_file_inode->blocks); k++){
          bfs.read_block(cat_file_inode->blocks[k],
                         (void *) &cat_file_contents);
          strcat(buffer, cat_file_contents->data);
        }
        //delete curr_dir_block_ptr;
        //delete cat_file_contents;
        delete cat_file_inode;
        // send buffer to socket here
      }
    }
  }

  // if point reached, file not found.
  // ERROR 503 File does not exist
  if(!found && !error){
    strcpy(buffer, "503: File does not exist\r\n");
  }
  
  delete cat_file_contents;
  delete curr_dir_block_ptr;
  send(fs_sock, buffer, strlen(buffer), 0);
}
// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
  bool error = false;
  bool found = false;
  char buffer [MAX_FILE_SIZE + 256];
  
  datablock_t* cat_file_contents = new datablock_t;
  dirblock_t* curr_dir_block_ptr = new dirblock_t;

  // read contents of current directory's directory node into
  //curr_dir_block_ptr
  bfs.read_block(curr_dir, curr_dir_block_ptr);

  // look through all elements of dir_entries held in current directory blocks
  for(int i = 0; i < MAX_DIR_ENTRIES; i++){
    //target file found in current element in dir_entries
    if(strcmp(name,curr_dir_block_ptr->dir_entries[i].name)==0){
      found = true;
      if(is_directory(curr_dir_block_ptr->dir_entries[i].block_num)){
        strcpy(buffer, "501: File is a directory\r\n");
        error = true;
        //delete curr_dir_block_ptr;
        //delete cat_file_contents;
      }
      else if(!is_directory(curr_dir_block_ptr->dir_entries[i].block_num)){
        inode_t* cat_file_inode = new inode_t;
        unsigned int bytes_to_write;
        //read inode data for targeted file
        bfs.read_block(curr_dir_block_ptr->dir_entries[i].block_num,
                       cat_file_inode);
        
        if(n > cat_file_inode->size){
          bytes_to_write == cat_file_inode->size;
        }
        
        string file_head_size_str = to_string(cat_file_inode->size);
        int file_byte_count = (file_head_size_str.length() + 1);
        char* file_size = new char[16];
        strcpy(file_size, file_head_size_str.c_str());
        
        strcat(buffer, "200 OK\r\n Length:");
        strcat(buffer, file_size);
        strcat(buffer, "\r\n");
        strcat(buffer, "\r\n");
        delete file_size;

        //append each data block pointed to by inode_t.blocks[] to our buffer
        for(int k = 0; k < sizeof(cat_file_inode->blocks); k++){
          bfs.read_block(cat_file_inode->blocks[k],
                         (void *) &cat_file_contents);
          if(bytes_to_write < BLOCK_SIZE){
            strncat(buffer, cat_file_contents->data, bytes_to_write);
          }
          else{
            strcat(buffer, cat_file_contents->data);
            bytes_to_write -= BLOCK_SIZE;
          }
        }
        delete cat_file_inode;
      }
    }
  }
  // if point reached, file not found.
  // ERROR 503 File does not exist
  if(!found && !error){
    strcpy(buffer, "503: File does not exist\r\n");
  }
  delete cat_file_contents;
  delete curr_dir_block_ptr;
  send(fs_sock, buffer, strlen(buffer), 0);
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
}

// HELPER FUNCTIONS 

const bool FileSys::is_directory(short block_num)
{
  //create dirblock_t to read block into
  dirblock_t* target_dir = new dirblock_t;
  bfs.read_block(block_num, (void *) &target_dir);

  if(target_dir->magic == DIR_MAGIC_NUM){
    delete target_dir;
    return true;
  }
  else{
    delete target_dir;
    return false;
  }
}
