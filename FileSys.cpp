// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
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
  if (strlen(name) > MAX_FNAME_SIZE + 1){
    cout << "File name is too long.\n";
    return;
  }

  // read current directory for duplicate name
  dirblock_t* curr_block_ptr = new dirblock_t;
  char file_name[MAX_FNAME_SIZE + 1];
  char curr_file_name[MAX_FNAME_SIZE + 1];
  strcpy(file_name, name);
  bfs.read_block(curr_dir, curr_block_ptr);

  if (curr_block_ptr->num_entries == MAX_DIR_ENTRIES){
    delete curr_block_ptr;
    cout << "Directory is full.\n";
    return;
  }

  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);

    if (strcmp(curr_file_name, file_name) == 0){
      delete curr_block_ptr;
      cout << "mkdir: cannot create directory '" << name
           << "’: File exists\n";
      return;
    }
  }

  // make new free block
  short block_num = bfs.get_free_block();
  if (block_num == 0){
    block_num = bfs.get_free_block();
    if (block_num == 0){
      cout << "Disk full.\n";
      delete curr_block_ptr;
      return;
    }
  }
}
  // switch to a directory
  void FileSys::cd(const char *name)
  {
    //retrieve current directory data block
    dirblock_t* curr_dir_block_ptr = new dirblock_t;
    bfs.read_block(curr_dir, (void *) &curr_dir_block_ptr);

    //check if any sub directories exist in current directory
    if(curr_dir_block_ptr->num_entries > 0)
      {
        //check each sub directory and check directory names for match
        for(int curr_sub_dir = 1; curr_sub_dir <= curr_dir_block_ptr->num_entries; curr_sub_dir++)
          {
            if(strcmp(curr_dir_block_ptr->dir_entries[curr_sub_dir].name, name) == 0)
              {
                curr_dir = curr_dir_block_ptr->dir_entries[curr_sub_dir].block_num;
                delete curr_dir_block_ptr;
                return;
              }
          }
      }
    cout << "cd: No matching directory found for " << name << "\n";
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
    bool error = false;

    // may need to increase buffer size to account for terminal messages
    char buffer [MAX_FILE_SIZE + 256];

    read(fs_sock, buffer, MAX_FILE_SIZE + 256);

    // check if name is too long
    if (strlen(name) > MAX_FNAME_SIZE + 1){
      cout << "File name is too long.\n";
      return;
    }

    datablock_t* cat_file_contents = new datablock_t;
    dirblock_t* curr_dir_block_ptr = new dirblock_t;
    bfs.read_block(curr_dir, curr_dir_block_ptr);
    for(int curr_dir_entry = 0; curr_dir_entry < MAX_DIR_ENTRIES; curr_dir_entry++)
      {
        //target file found in current element in dir_entries
        if(strcmp(name,curr_dir_block_ptr->dir_entries[curr_dir_entry].name)==0)
          {
            if(is_directory(curr_dir_block_ptr->dir_entries[curr_dir_entry].block_num))
              {
                strcpy(buffer, "501: File is a directory\r\n");
                error = true;
                delete curr_dir_block_ptr;
                delete cat_file_contents;
              }
            else if(!is_directory(curr_dir_block_ptr->dir_entries[curr_dir_entry].block_num))
              {
                inode_t* cat_file_inode = new inode_t;
                bfs.read_block(curr_dir_block_ptr->dir_entries[curr_dir_entry].block_num, cat_file_inode);


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
                for(int curr_data_block = 0; curr_data_block < sizeof(cat_file_inode->blocks); curr_data_block++)
                  {
                    bfs.read_block(cat_file_inode->blocks[curr_data_block], (void *) &cat_file_contents);
                    strcat(buffer, cat_file_contents->data);
                  }
                delete curr_dir_block_ptr;
                delete cat_file_contents;
                delete cat_file_inode;
                // send buffer to socket here
              }
          }
      }
    // if point reached, file not found.
    // ERROR 503 File does not exist
    strcpy(buffer, "503: File does not exist\r\n");
    error = true;
    delete cat_file_contents;
    delete curr_dir_block_ptr;
    send(fs_sock, buffer, strlen(buffer), 0);
  }
  
  // display the first N bytes of the file
  void FileSys::head(const char *name, unsigned int n)
  {
    // check if name is too long
    if (strlen(name) > MAX_FNAME_SIZE + 1)
      {
        cout << "File name is too long.\n";
        return;
      }

    // Search for matching filename

    // Find associated inode for file (check for inode_t.magic == INODE_MAGIC_NUM)

    // If file empty, send error message

    // If file has data, check inode for file size in bytes (inode_t.size)

    // If n > inode_t.size print every block in inode_t.blocks[]

    // Else if 0 < n < inode_t.size, check if BLOCK_SIZE|bytes
    // if n / BLOCK_SIZE is a whole number, print the first n/BLOCK_SIZE blocks
    // in inode_t.blocks[]

    // if n / BLOCK_SIZE not whole number, print byte by byte (1 byte = 1 char)

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
  const bool FileSys::is_directory(short block_num)
  {
    //create dirblock_t to read block into
    dirblock_t* target_dir = new dirblock_t;
    bfs.read_block(block_num, (void *) &target_dir);

    if(target_dir->magic == DIR_MAGIC_NUM)
      {
        delete target_dir;
        return true;
      }
    else
      {
        delete target_dir;
        return false;
      }
  }

