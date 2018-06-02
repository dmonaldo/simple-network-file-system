#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "FileSys.h"

using namespace std;

// report error
void error(char* message) {
	perror(message);
	exit(0);
}

int main(int argc, char* argv[]) {
		const int BACKLOG = 5;
		const int BUFFER_LENGTH = 1024;
		int sockfd, newsockfd, port, clilen;
		char buffer[BUFFER_LENGTH];
		struct sockaddr_in serv_addr, cli_addr;

		if (argc < 2) {
			cout << "Usage: ./nfsserver port#\n";
			return -1;
		}

		port = atoi(argv[1]);
		cout << "Connecting to port " << port << endl;

		// create socket
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
			error((char*)"ERROR opening socket");

		// bind socket to address and port number
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		if (::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
			error((char*)"ERROR on bind");

		// listen for client to make a connection
		if (::listen(sockfd, BACKLOG) < 0)
			error((char*)"ERROR on listen");

		// accept a request from client
		newsockfd = ::accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen);
		if (newsockfd < 0)
			error((char*)"ERROR on accept");

		bzero(buffer, BUFFER_LENGTH);

		// mount the file system
		FileSys fs;
		fs.mount(newsockfd);
		// assume that sock is the new socket created
		// for a TCP connection between the client and the server.

		// loop: get the command from the client and invoke the file
		// system operation which returns the results or error messages back to the client
		// until the client closes the TCP connection.
		int response = 1;
		while (response != 0) {
			response = read(newsockfd, buffer, BUFFER_LENGTH);
			cout << "RESPONSE:" << endl;
			cout << response << endl;
			cout << buffer << endl;
			bzero(buffer, BUFFER_LENGTH);
		}

		// close the listening socket
		close(sockfd);

		// unmount the file system
		fs.unmount();

		return 0;
}
