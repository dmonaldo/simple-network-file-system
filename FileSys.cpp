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
  char buffer[1024];
  curr_dir = 1;
  strcpy(buffer, "switched to the home directory\r\n");
  send(fs_sock, buffer, sizeof(buffer), 0);
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
  char buffer[1024];
  bool error = false;
  
  if(strlen(name) > MAX_FNAME_SIZE + 1){
    strcpy(buffer, "504: File name is too long\r\n");
    error = true;
  }

  // read current directory for duplicate name
  dirblock_t* curr_block_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE + 1];
  char curr_file_name[MAX_FNAME_SIZE + 1];
  strcpy(file_name, name);
  bfs.read_block(curr_dir, (void*) &curr_block_ptr);
  if(!error && (curr_block_ptr->num_entries == MAX_DIR_ENTRIES)){
    strcpy(buffer, "506: Directory is full\r\n");
    error = true;
    delete curr_block_ptr;
  }

  for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);
    if(strcmp(curr_file_name, file_name) == 0){
      if(!error){
        strcpy(buffer, "502: File exists\r\n");
        error = true;
        delete curr_block_ptr;
      }
    }
  }

  // make new free block
  short block_num = bfs.get_free_block();
  if(block_num == 0){
    block_num = bfs.get_free_block();
    if(block_num == 0){
      if(!error){
        strcpy(buffer, "505: Disk is full\r\n");
        error = true;
        delete curr_block_ptr;
      }
    }
  }

  // fill new inode block_num to hold 0 to show blocks are unused
  if(!error){
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
    strcpy(buffer, "200 OK\r\n Length: 0\r\n");
    strcpy(curr_block_ptr->dir_entries[curr_block_ptr->num_entries].name,
           name);
    curr_block_ptr->dir_entries[curr_block_ptr->num_entries].block_num =
      block_num;
    curr_block_ptr->num_entries++;

    //write block and delete
    bfs.write_block(curr_dir, (void*) &curr_block_ptr);
    delete curr_block_ptr;
  }
  send(fs_sock, buffer, sizeof(buffer), 0);
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
  char buffer[MAX_FNAME_SIZE+256];
  dirblock_t* curr_block_ptr = new dirblock_t;
  bool error = false;
  bool found = false;
  bfs.read_block(curr_dir, curr_block_ptr);

  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    if (strcmp(curr_block_ptr->dir_entries[i].name, name) == 0){
      if (!is_directory(curr_block_ptr->dir_entries[i].block_num)){
        found = true;
        inode_t* inode_ptr = new inode_t;
        bfs.read_block(curr_block_ptr->dir_entries[i].block_num, inode_ptr);
        for (size_t j = 0; j < (inode_ptr->size / BLOCK_SIZE) + 1; j++){
          bfs.reclaim_block(inode_ptr->blocks[i]);
          inode_ptr->blocks[i] = 0;
          inode_ptr->size = 0;
        }

        delete inode_ptr;

        bfs.reclaim_block(curr_block_ptr->dir_entries[i].block_num);

        curr_block_ptr->dir_entries[i].name[0] = '\0';
        curr_block_ptr->dir_entries[i].block_num = 0;

        curr_block_ptr->num_entries--;
        bfs.write_block(curr_dir, curr_block_ptr);
        //write success message into buffer
        //delete curr_block_ptr;
      }
      else{
        strcpy(buffer, "501: File is a directory");
        error = true;
      }
    }
  }
  if(!found && !error){
    strcpy(buffer, "503 File does not exist");
  }else{
    strcpy(buffer, "200 OK\r\n Length: 0\r\n");
  }
  //send buffer to client
  delete curr_block_ptr;
  send(fs_sock, buffer, sizeof(buffer), 0);
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
  bool found = false;
  int found_index;
  struct dirblock_t *curr_block_ptr = new dirblock_t;
  bfs.read_block(curr_dir, (void*)&curr_block_ptr);
  struct dirblock_t *found_dir_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE+1];
  char curr_file_name[MAX_FNAME_SIZE+1];
  strcpy(file_name, name);
  bool dir_check;
  char bufferStart[] = "200 OK\r\n";
  char msgLength[80];
  char message[2048];

  int counter = 0;
  int first_block;
  bool check_first_block = false;
  char buffer[1024];
  char directory_name_label[16];
  char directory_name[MAX_FNAME_SIZE];
  char directory_block_label[17];
  short directory_block_num =
    curr_block_ptr->dir_entries[curr_block_ptr->num_entries].block_num;
  char* directory_block = new char[MAX_DATA_BLOCKS];
  char* inode_block = new char[MAX_DATA_BLOCKS];
  char* bytes_in_file = new char[MAX_FILE_SIZE];
  char* number_of_blocks = new char[MAX_DATA_BLOCKS];
  char* first_block_num = new char[MAX_DATA_BLOCKS];

  for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);
    if (strcmp(curr_file_name, file_name) == 0){
      found = true;
      found_index = i;
      //found_block = curr_block_ptr->dir_entries[i].block_num;
      bfs.read_block(curr_block_ptr->dir_entries[i].block_num,
                     (void*)&found_dir_ptr);
    }
  }
  if(found){
    // call is_directory
    dir_check = is_directory(curr_dir);
    if(dir_check){
      strcat(buffer, "Directory name: ");
      strcat(buffer, name);
      strcat(buffer, "Directory block: ");
      sprintf(directory_block, "%d", directory_block_num);
      strcat(buffer, directory_block);
    }
    else{
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
      
        strcat(buffer, "Inode block: ");
        sprintf(directory_block, "%d", directory_block_num);
        strcat(buffer, directory_block);
        strcat(buffer, "Bytes in file: ");
        sprintf(bytes_in_file, "%d", new_inode->size);
        strcat(buffer, bytes_in_file);
        strcat(buffer, "Number of blocks: ");
        sprintf(number_of_blocks, "%d", counter);
        strcat(buffer, number_of_blocks);
        strcat(buffer, "First block: ");
        sprintf(first_block_num, "%d", first_block);
        strcat(buffer, first_block_num);
      }
    }
    strcpy(message, bufferStart);
    sprintf(msgLength, "Length: %d \r\n", sizeof(buffer));
    strcat(message, msgLength);
    strcat(message, buffer);
  }else{
    strcpy(message, "503 File does not exist\r\n");
  }

  send(fs_sock, message, sizeof(message), 0);
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
