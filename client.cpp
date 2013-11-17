#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "my_global.h"
#include <iostream>
#include <fstream>

using namespace std;

string filename;
unsigned short servPort, clientPort; //server, Client port
char* serverIP;
int swnd;

char* to_char_pointer(string str) {
	char* ch = (char*) malloc(str.length() * sizeof(char));
	strcpy(ch, str.c_str());
	return ch;
}

//Initialize Client Parameters from file
void initialize() {
	ifstream init("client.in");
	string line;
	init >> line;
	serverIP = to_char_pointer(line);
	init >> servPort;//Well-known port number of server.
	init >> clientPort;//Port number of client.
	init >> filename;//Filename to be transferred
	init >> swnd;//Initial receiving sliding-window size
	init.close();
	cout << serverIP << "  " << servPort << "  " << clientPort << "  "
			<< filename << "  " << swnd << endl;
}

void error(string str) {
	cout << str;
	exit(1);
}

int main(int argc, char *argv[]) {
	initialize();

	int sock;
	struct sockaddr_in servAddr;
	struct sockaddr_in clientAddr;
	unsigned int fromSize;// in-out of address size for recvfrom
	char recvBuffer[MAX_PACKET_SIZE + 1];

	/* Create a datagram/UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		error("socket () failed");

	/* Construct the server address structure */
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(serverIP);
	servAddr.sin_port = htons(servPort);

	int msgLen = strlen(to_char_pointer(filename));
	/* Send the request to the server */
	if (sendto(sock, to_char_pointer(filename), msgLen, 0,
			(struct sockaddr *) &servAddr, sizeof(servAddr)) != msgLen)
		error("sendto() sent a different number of bytes than expected");

	cout << "send" << endl;
	//response
	//	fromSize = sizeof(clientAddr);
	//	if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
	//			(struct sockaddr *) &fromAddr, &fromSize)) != echoStringLen)
	//		error("recvfrom() failed");
	close(sock);
	return 0;
}
