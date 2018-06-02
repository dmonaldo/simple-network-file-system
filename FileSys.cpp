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
    bool error = false;
    bool found = false;
    char buffer[256];
    read(fs_sock, buffer, 256);

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
                found = true;
                if(!is_directory(curr_dir_block_ptr->dir_entries[curr_sub_dir].block_num))
                {
                    error = true;
                    strcat(buffer, "500 File is not a directory");
                }
                else
                {
                  curr_dir = curr_dir_block_ptr->dir_entries[curr_sub_dir].block_num;
                }
              }
          }
      }
      // if this point reached, no matching directory found
      if(found && !error)
      {
        strcat(buffer, "503 File does not exist");
      }
      delete curr_dir_block_ptr;
      send(fs_sock, buffer, strlen(buffer), 0);
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
    bool found = false;

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
            found = true;
            if(is_directory(curr_dir_block_ptr->dir_entries[curr_dir_entry].block_num))
              {
                strcpy(buffer, "501: File is a directory\r\n");
                error = true;
                delete curr_dir_block_ptr;
                delete cat_file_contents;
                break;
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
      if(!found && !error)
      {
        strcpy(buffer, "503: File does not exist\r\n");
      }

    delete cat_file_contents;
    delete curr_dir_block_ptr;
    send(fs_sock, buffer, strlen(buffer), 0);
  }

  // display the first N bytes of the file
  void FileSys::head(const char *name, unsigned int n)
  {
    bool found = false;
    bool error = false;
    char buffer [MAX_FILE_SIZE + 256];
    read(fs_sock, buffer, MAX_FILE_SIZE + 256);

    datablock_t* cat_file_contents = new datablock_t;
    dirblock_t* curr_dir_block_ptr = new dirblock_t;

    // read contents of current directory's directory node into curr_dir_block_ptr
    bfs.read_block(curr_dir, curr_dir_block_ptr);

    // look through all elements of dir_entries held in current directory blocks
    for(int curr_dir_entry = 0; curr_dir_entry < MAX_DIR_ENTRIES; curr_dir_entry++)
      {
        //target file found in current element in dir_entries
        if(strcmp(name,curr_dir_block_ptr->dir_entries[curr_dir_entry].name)==0)
          {
            found = true;
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
                unsigned int bytes_to_write;
                //read inode data for targeted file
                bfs.read_block(curr_dir_block_ptr->dir_entries[curr_dir_entry].block_num, cat_file_inode);

                if(n > cat_file_inode->size)
                {
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
                for(int curr_data_block = 0; curr_data_block < sizeof(cat_file_inode->blocks); curr_data_block++)
                  {
                    bfs.read_block(cat_file_inode->blocks[curr_data_block], (void *) &cat_file_contents);
                    if(bytes_to_write < BLOCK_SIZE)
                    {
                      strncat(buffer, cat_file_contents->data, bytes_to_write);
                    }
                    else
                    {
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
    if(!found && !error)
    {
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
