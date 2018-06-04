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
  cout << "Making Directory " << endl;

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
           << "â€™: File exists\n";
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

  //fill new directory block_num to hold 0 to show blocks are unused
  dirblock_t* new_block = new dirblock_t;
  new_block->magic = DIR_MAGIC_NUM;
  new_block->num_entries = 0;
  for (int i = 0; i < MAX_DIR_ENTRIES; i++){
    new_block->dir_entries[i].block_num = 0;
  }
  bfs.write_block(block_num, new_block);
  delete new_block;

  strcpy(curr_block_ptr->dir_entries[curr_block_ptr->num_entries].name, name);
  curr_block_ptr->dir_entries[curr_block_ptr->num_entries].block_num =
    block_num;
  curr_block_ptr->num_entries++;

  // write block and delete
  bfs.write_block(curr_dir, curr_block_ptr);
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
    //read each file name entry as long a block for file is not equal to 0
    //strcpy(curr_file_name, curr_block_ptr->dir_entries[i].name);
    //put the curr_file_name into socket buffer to send text format to client
  }
  delete curr_block_ptr;
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

// Executes the command. Returns true for quit and false otherwise.
bool FileSys::execute_command(string command_str)
{
  // parse the command line
  struct Command command = parse_command(command_str);

  cout << "NAME: " << command.name << endl;
  cout << "FILENAME: " << command.file_name << endl;
  cout << "APPEND DATA: " << command.append_data << endl;

  // copy the contents of the string to char array
  const char* command_name = command.name.c_str();
  const char* file_name = command.file_name.c_str();
  const char* append_data = command.append_data.c_str();

  // look for the matching command
  if (command.name == "") {
    return false;
  }
  else if (command.name == "mkdir") {
    mkdir(file_name);
  }
  else if (command.name == "cd") {
    cd(file_name);
  }
  else if (command.name == "home") {
    home();
  }
  else if (command.name == "rmdir") {
    rmdir(file_name);
  }
  else if (command.name == "ls") {
    ls();
  }
  else if (command.name == "create") {
    create(file_name);
  }
  else if (command.name == "append") {
    append(file_name, append_data);
  }
  else if (command.name == "cat") {
    cat(file_name);
  }
  else if (command.name == "head") {
    errno = 0;
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    if (0 == errno) {
      head(file_name, n);
    } else {
      cerr << "Invalid command line: " << command.append_data;
      cerr << " is not a valid number of bytes" << endl;
      return false;
    }
  }
  else if (command.name == "rm") {
    rm(file_name);
  }
  else if (command.name == "stat") {
    stat(file_name);
  }
  else if (command.name == "quit") {
    return true;
  }

  return false;
}


// Parses a command line into a command struct. Returned name is blank
// for invalid command lines.
FileSys::Command FileSys::parse_command(string command_str)
{
  // empty command struct returned for errors
  struct Command empty = {"", "", ""};

  // Remove \r\n
  command_str = command_str.substr(0, command_str.size() - 6);

  // grab each of the tokens (if they exist)
  struct Command command;
  istringstream ss(command_str);
  int num_tokens = 0;
  if (ss >> command.name) {
    num_tokens++;
    if (ss >> command.file_name) {
      num_tokens++;
      if (ss >> command.append_data) {
        num_tokens++;
        string junk;
        if (ss >> junk) {
          num_tokens++;
        }
      }
    }
  }

  // Check for empty command line
  if (num_tokens == 0) {
    return empty;
  }

  // Check for invalid command lines
  if (command.name == "ls" ||
      command.name == "home" ||
      command.name == "quit")
  {
    if (num_tokens != 1) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "mkdir" ||
      command.name == "cd"    ||
      command.name == "rmdir" ||
      command.name == "create"||
      command.name == "cat"   ||
      command.name == "rm"    ||
      command.name == "stat")
  {
    if (num_tokens != 2) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "append" || command.name == "head")
  {
    if (num_tokens != 3) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else {
    cerr << "Invalid command line: " << command.name;
    cerr << " is not a command" << endl;
    return empty;
  }

  return command;
}
