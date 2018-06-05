// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#ifndef FILESYS_H
#define FILESYS_H

#include <sstream>

using namespace std;

#include "BasicFileSys.h"

class FileSys {

  public:
    // mounts the file system
    void mount(int sock);

    // unmounts the file system
    void unmount();

    // make a directory
    void mkdir(const char *name);

    // switch to a directory
    void cd(const char *name);

    // switch to home directory
    void home();

    // remove a directory
    void rmdir(const char *name);

    // list the contents of current directory
    void ls();

    // create an empty data file
    void create(const char *name);

    // append data to a data file
    void append(const char *name, const char *data);

    // display the contents of a data file
    void cat(const char *name);

    // display the first N bytes of the file
    void head(const char *name, unsigned int n);

    // delete a data file
    void rm(const char *name);

    // display stats about file or directory
    void stat(const char *name);

    // Executes the command. Returns true for quit and false otherwise.
    bool execute_command(string command_str);

  private:
    BasicFileSys bfs;	// basic file system
    short curr_dir;	// current directory
    int fs_sock;  // file server socket

    // data structure for command
    struct Command
    {
      string name;		// name of command
      string file_name;		// name of file
      string append_data;	// append data (append only)
    };

    // Parses a command into a command struct. Returned name is blank
    // for invalid command lines.
    struct Command parse_command(string command_str);

    // Additional private variables and Helper functions - if desired
    const bool is_directory(short block_num);
};

#endif
