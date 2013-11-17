/*
 * server.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: raymond
 */
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

unsigned short servPort; //server,port
int max_swnd, seed;
float loss_prob;

void initialize() {
	ifstream init("server.in");
	init >> servPort;//Well-known port number of server.
	init >> max_swnd;//Maximum sending sliding-window size
	init >> seed;//Random generator seed value
	init >> loss_prob;//Probability p of datagram loss
	init.close();
	cout << servPort << "  " << max_swnd << "  " << seed << "  " << loss_prob
			<< endl;
}

void error(string str) {
	cout << str << endl;
	exit(1);
}

int main(int argc, char *argv[]) {
	initialize();

	int sock;
	struct sockaddr_in servAddr;
	struct sockaddr_in clientAddr;
	int recv_msg_size;
	char recv_msg[MAX_PACKET_SIZE];

	/* Create socket for sending/receiving datagrams */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		error("socket() failed");

	/* Construct local address structure */
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(servPort);

	/* Bind to the local address */
	if (bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		error("bind() failed");

	//receive
	/* Set the size of the in-out parameter */
	unsigned int cliAddrLen = sizeof(clientAddr);
	if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
			(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
		error("recvfrom() failed");

	printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));
	cout << recv_msg_size << endl;
	cout << recv_msg << endl;
	return 0;
}
