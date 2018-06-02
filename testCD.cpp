#include "string.h"
#include <unistd.h>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"


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
  }

int main()
{
  FileSys fs;
  return 0;
}
