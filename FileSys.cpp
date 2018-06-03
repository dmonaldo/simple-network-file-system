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
    dirblock_t* dir_ptr = new dirblock_t;
    bfs.read_block(curr_dir, (void *) &dir_ptr);

    //check if any sub directories exist in current directory
    if(dir_ptr->num_entries > 0)
      {
        //check each sub directory and check directory names for match
        for(int i = 1; i <= dir_ptr->num_entries; i++)
          {
            if(strcmp(dir_ptr->dir_entries[i].name, name) == 0)
              {
                found = true;
                if(!is_directory(dir_ptr->dir_entries[i].block_num))
                {
                    error = true;
                    strcat(buffer, "500 File is not a directory");
                }
                else
                {
                  curr_dir = dir_ptr->dir_entries[i].block_num;
                }
              }
          }
      }
      // if this point reached, no matching directory found
      if(found && !error)
      {
        strcat(buffer, "503 File does not exist");
      }
      delete dir_ptr;
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

    datablock_t* file_contents = new datablock_t;
    dirblock_t* dir_ptr = new dirblock_t;
    bfs.read_block(curr_dir, dir_ptr);
    for(int i = 0; i < MAX_DIR_ENTRIES; i++)
      {
        //target file found in current element in dir_entries
        if(strcmp(name, dir_ptr->dir_entries[i].name)==0)
          {
            found = true;
            if(is_directory(dir_ptr->dir_entries[i].block_num))
              {
                strcpy(buffer, "501: File is a directory\r\n");
                error = true;
                delete dir_ptr;
                delete file_contents;
                break;
              }
            else if(!is_directory(dir_ptr->dir_entries[i].block_num))
              {
                inode_t* file_inode = new inode_t;
                bfs.read_block(dir_ptr->dir_entries[i].block_num, file_inode);


                string file_size_str = to_string(file_inode->size);
                int byte_count = (file_size_str.length() + 1);
                char* file_size = new char[byte_count];
                strcpy(file_size, file_size_str.c_str());

                strcat(buffer, "200 OK\r\n Length:");
                strcat(buffer, file_size);
                strcat(buffer, "\r\n");
                strcat(buffer, "\r\n");
                delete file_size;

                //append each data block pointed to by inode_t.blocks[] to our buffer
                for(int j = 0; j < sizeof(file_inode->blocks); j++)
                  {
                    bfs.read_block(file_inode->blocks[j], (void *) &file_contents);
                    strcat(buffer, file_contents->data);
                  }
                delete dir_ptr;
                delete file_contents;
                delete file_inode;
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

    delete file_contents;
    delete dir_ptr;
    send(fs_sock, buffer, strlen(buffer), 0);
  }

  // display the first N bytes of the file
  void FileSys::head(const char *name, unsigned int n)
  {
    bool found = false;
    bool error = false;
    char buffer [MAX_FILE_SIZE + 256];
    read(fs_sock, buffer, MAX_FILE_SIZE + 256);

    datablock_t* file_contents = new datablock_t;
    dirblock_t* dir_ptr = new dirblock_t;

    // read contents of current directory's directory node into dir_ptr
    bfs.read_block(curr_dir, dir_ptr);

    // look through all elements of dir_entries held in current directory blocks
    for(int i = 0; i < MAX_DIR_ENTRIES; i++)
      {
        //target file found in current element in dir_entries
        if(strcmp(name,dir_ptr->dir_entries[i].name)==0)
          {
            found = true;
            if(is_directory(dir_ptr->dir_entries[i].block_num))
              {
                strcpy(buffer, "501: File is a directory\r\n");
                error = true;
                delete dir_ptr;
                delete file_contents;
              }
            else if(!is_directory(dir_ptr->dir_entries[i].block_num))
              {
                inode_t* file_inode = new inode_t;
                unsigned int bytes_left;
                //read inode data for targeted file
                bfs.read_block(dir_ptr->dir_entries[i].block_num, file_inode);

                if(n > file_inode->size)
                {
                  bytes_left == file_inode->size;
                }


                string size_str = to_string(file_inode->size);
                int byte_count = (size_str.length() + 1);
                char* file_size = new char[16];
                strcpy(file_size, size_str.c_str());

                strcat(buffer, "200 OK\r\n Length:");
                strcat(buffer, file_size);
                strcat(buffer, "\r\n");
                strcat(buffer, "\r\n");
                delete file_size;


                //append each data block pointed to by inode_t.blocks[] to our buffer
                for(int j = 0; j < sizeof(file_inode->blocks); j++)
                  {
                    bfs.read_block(file_inode->blocks[j], (void *) &file_contents);
                    if(bytes_left < BLOCK_SIZE)
                    {
                      strncat(buffer, file_contents->data, bytes_left);
                    }
                    else
                    {
                      strcat(buffer, file_contents->data);
                      bytes_left -= BLOCK_SIZE;
                    }
                  }
                  delete file_inode;
              }
          }
      }
    // if point reached, file not found.
    // ERROR 503 File does not exist
    if(!found && !error)
    {
      strcpy(buffer, "503: File does not exist\r\n");
    }
    delete file_contents;
    delete dir_ptr;
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
