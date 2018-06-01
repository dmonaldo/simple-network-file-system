#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "FileSys.h"
#include <string.h>
using namespace std;

// report error
void error(char* message) {
	perror(message);
	exit(0);
}

int main(int argc, char* argv[]) {
	const int BACKLOG = 5;
	int sockfd, newsockfd, clilen;
	char buffer[256];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    struct sockaddr_in serv_addr, cli_addr;
	int n;

	if (argc < 2) {
      cout << "Usage: ./nfsserver port#\n";
      return -1;
    }

    int port = atoi(argv[1]);
    cout << "Connecting to port " << port << endl;
      
	// create socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==0){
      perror("ERROR opening socket");
      exit(EXIT_FAILURE);
    } 
   	// bind socket to address and port number
    //	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr))
       < 0){
      perror("ERROR on bind");
    }

    if(p == NULL){
      fprintf(stderr, "server: failed to bind\n");
      exit(1);
    }
   	// listen for client to make a connection
   	if (listen(sockfd, BACKLOG) == -1){
      perror("ERROR on listen");
      exit(1);
    }

    // accept a request from client
   	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                             (socklen_t*)&clilen);
		if (newsockfd < 0)
			error((char*)"ERROR on accept");

		bzero(buffer, 256);

		cout << "SOCK: " << sockfd << endl;
		cout << "NEW SOCKET: " << newsockfd << endl;

		// mount the file system
		FileSys fs;
		fs.mount(newsockfd);
		//assume that sock is the new socket created
		//for a TCP connection between the client and the server.

		//loop: get the command from the client and invoke the file
		//system operation which returns the results or error messages back to the clinet
		//until the client closes the TCP connection.
        while(1) {  // main accept() loop
          sin_size = sizeof their_addr;
          new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
          if (new_fd == -1) {
            perror("accept");
            continue;
          }

          inet_ntop(their_addr.ss_family,
                    get_in_addr((struct sockaddr *)&their_addr),
                    s, sizeof s);
          printf("server: got connection from %s\n", s);

          if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1) //send info to
              //server here
              perror("send");
            close(new_fd);
            exit(0);
          }
          close(new_fd); 
		//close the listening socket
        }
		//unmout the file system
		fs.unmount();

		return 0;
}
