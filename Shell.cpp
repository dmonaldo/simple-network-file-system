// CPSC 3500: Shell
// Implements a basic shell (command line interface) for the file system
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#include "Shell.h"

static const string PROMPT_STRING = "NFS> ";	// shell prompt

// Mount the network file system with server name and port number in the
// format of server:port
void Shell::mountNFS(string fs_loc) {
  struct sockaddr_in server;

  // parse filesystem location into array with servername and port
  vector<string> fs_address;
  size_t pos = 0, found;
  while((found = fs_loc.find_first_of(':', pos)) != string::npos) {
    fs_address.push_back(fs_loc.substr(pos, found - pos));
    pos = found+1;
  }
  fs_address.push_back(fs_loc.substr(pos));

  // create the socket
  cs_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (cs_sock < 0) {
    perror("ERROR creating socket");
    exit(0);
  }
  cout << "Socket created\n";

  // construct server address
  server.sin_addr.s_addr = inet_addr(fs_address[0].c_str());
  server.sin_family = AF_INET;
  server.sin_port = htons(stoi(fs_address[1]));

  // connect to remote server
  if (connect(cs_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("ERROR connection failed");
    exit(0);
  }

  cout << "Connected\n";
  is_mounted = true;
}

// Unmount the network file system if it was mounted
void Shell::unmountNFS() {
  if (is_mounted) {
    close(cs_sock);
    is_mounted = false;
  }
}

// Remote procedure call on mkdir
void Shell::mkdir_rpc(string dname) {
  string command = "mkdir " + dname + "\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("mkdir", received);
}

// Remote procedure call on cd
void Shell::cd_rpc(string dname) {
  string command = "cd " + dname + "\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("cd", received);
}

// Remote procedure call on home
void Shell::home_rpc() {
  string command = "home\r\n";
  char message[200];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("home", received);
}

// Remote procedure call on rmdir
void Shell::rmdir_rpc(string dname) {
  string command = "rmdir " + dname + "\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("rmdir", received);
}

// Remote procedure call on ls
void Shell::ls_rpc() {
  string command = "ls\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("ls", received);
}

// Remote procedure call on create
void Shell::create_rpc(string fname) {
  string command = "create " + fname + "\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("create", received);
}

// Remote procedure call on append
void Shell::append_rpc(string fname, string data) {
  // to implement
}

// Remote procesure call on cat
void Shell::cat_rpc(string fname) {
  // to implement
  string command = "cat " + fname + "\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("cat", received);
}

// Remote procedure call on head
void Shell::head_rpc(string fname, int n) {
  // to implement
  string command = "head " + fname + to_string(n) + "\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("head", received);
}

// Remote procedure call on rm
void Shell::rm_rpc(string fname) {
  string command = "rm " + fname + "\r\n";
  char message[2048];
  char received[2048];
  strcpy(message, command.c_str());

  // send to server
  send(cs_sock, message, sizeof(message), 0);
  recv(cs_sock, received, sizeof(received), 0);

  // print
  print_response("rm", received);
}

// Remote procedure call on stat
void Shell::stat_rpc(string fname)
{
  string command = "stat " + fname + "\r\n";
  char* buffer[2048];
  send(cs_sock, command.c_str(), strlen(command.c_str()), 0);
  recv(cs_sock, buffer, sizeof(buffer), 0);
}

// Executes the shell until the user quits.
void Shell::run()
{
  // make sure that the file system is mounted
  if (!is_mounted)
 	return;

  // continue until the user quits
  bool user_quit = false;
  while (!user_quit) {

    // print prompt and get command line
    string command_str;
    cout << PROMPT_STRING;
    getline(cin, command_str);

    // execute the command
    user_quit = execute_command(command_str);
  }

  // unmount the file system
  unmountNFS();
}

// Execute a script.
void Shell::run_script(char *file_name)
{
  // make sure that the file system is mounted
  if (!is_mounted)
  	return;
  // open script file
  ifstream infile;
  infile.open(file_name);
  if (infile.fail()) {
    cerr << "Could not open script file" << endl;
    return;
  }


  // execute each line in the script
  bool user_quit = false;
  string command_str;
  getline(infile, command_str, '\n');
  while (!infile.eof() && !user_quit) {
    cout << PROMPT_STRING << command_str << endl;
    user_quit = execute_command(command_str);
    getline(infile, command_str);
  }

  // clean up
  unmountNFS();
  infile.close();
}


// Executes the command. Returns true for quit and false otherwise.
bool Shell::execute_command(string command_str)
{
  // parse the command line
  struct Command command = parse_command(command_str);

  // look for the matching command
  if (command.name == "") {
    return false;
  }
  else if (command.name == "mkdir") {
    mkdir_rpc(command.file_name);
  }
  else if (command.name == "cd") {
    cd_rpc(command.file_name);
  }
  else if (command.name == "home") {
    home_rpc();
  }
  else if (command.name == "rmdir") {
    rmdir_rpc(command.file_name);
  }
  else if (command.name == "ls") {
    ls_rpc();
  }
  else if (command.name == "create") {
    create_rpc(command.file_name);
  }
  else if (command.name == "append") {
    append_rpc(command.file_name, command.append_data);
  }
  else if (command.name == "cat") {
    cat_rpc(command.file_name);
  }
  else if (command.name == "head") {
    errno = 0;
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    if (0 == errno) {
      head_rpc(command.file_name, n);
    } else {
      cerr << "Invalid command line: " << command.append_data;
      cerr << " is not a valid number of bytes" << endl;
      return false;
    }
  }
  else if (command.name == "rm") {
    rm_rpc(command.file_name);
  }
  else if (command.name == "stat") {
    stat_rpc(command.file_name);
  }
  else if (command.name == "quit") {
    return true;
  }

  return false;
}

// Parses a command line into a command struct. Returned name is blank
// for invalid command lines.
Shell::Command Shell::parse_command(string command_str)
{
  // empty command struct returned for errors
  struct Command empty = {"", "", ""};

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

// prints the response from the server
void Shell::print_response(string command, string response)
{
  stringstream ss(response);
  string item;
  vector<string> splitResponse;

  // split response by line breaks into an array of strings
  while (getline(ss, item, '\n')) {
     splitResponse.push_back(item);
  }

  // check if response was successful
  if (stoi(response.substr(0,3)) == 200) {
    if (command == "ls" ||
        command == "head" ||
        command == "stat" ||
        command == "cat") {
      cout << splitResponse[3] << endl;
    }
  } else {
    cout << response << endl;
  }
}
