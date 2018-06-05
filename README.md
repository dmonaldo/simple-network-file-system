#simple-network-file-system
##A simple client-server network file system (NFS) over a simulated disk.

The file system is built on top of a virtual disk that is simulated using a file. In this virtual disk, there are 1,024 disk blocks (numbered from 0 to 1023) and each block is 128 bytes. The server is not a multi-client program.

######Structure:
- **Client-Side Shell** (`Shell.cpp`): Processes the network file system commands from the command line.
- **Server-Side File System** (`FileSys.cpp`): Provides an interface for NFS commands received from the client via the TCP socket and sends back the responses to the client via the TCP socket.
- **Basic File System** (`BasicFileSys.cpp`): A low-level interface that interacts with the disk.
- **Disk** (`Disk.cpp`): Represents a "virtual" disk that is contained within a file.
- **Client** (`client.cpp`): NFS client main program.
- **Server** (`server.cpp`): NFS server main program.

######File System Commands:
- `mkdir <directory>` - Creates an empty subdirectory in the current directory.
- `ls` - List the contents of the current directory. Directories should have a '/' suffix such as 'myDir/'. Files do not have a suffix.
- `cd <directory>` - Change to specified directory. The directory must be a subdirectory in the current directory. No paths or ".." are allowed.
- `home` - Switch to the home directory.
- `rmdir <directory>` - Removes a subdirectory. The subdirectory must be empty.
- `create <filename>` - Creates an empty file of the filename in the current directory. An empty file consists of an inode and no data blocks.
- `append <filename> <data>` - Appends the data to the file. Data should be appended in a manner to first fill the last data block as much as possible and then allocating new block(s) ONLY if more space is needed. More information about the format of data files is described later.
- `stat <name>` - Displays stats for the given file or directory. The precise format is described later in the document.
- `cat <filename>` - Display the contents of the file to the screen. Print a newline when completed.
- `head <filename> <n>` - Display the first N bytes of the file to the screen. Print a newline when completed. (If N >= file size, print the whole file just as with the cat command.)
- `rm <filename>` - Remove a file from the directory, reclaim all of its blocks including its inode. Cannot remove directories.

######Team Members:
- Alex Runciman (@agrsu) - [Contributions]

- Dante Monaldo (@dmonaldo) - [Contributions]

- Jackie Wong (@jmhw) - [Contributions]

- Matt Harrison (@WarpFactors) - [Contributions]

######Functionality Rating
Rating: A, B, C, D, or F?
Explanation:

######Test Cases:
Test case #1: mkdir abc
Results:


Test case #2:
