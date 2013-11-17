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
#include <time.h>
#include <stdlib.h>     /* srand, rand */

using namespace std;

unsigned short servPort; //server,port
int max_swnd, seed;
float loss_prob;
int sock;
struct sockaddr_in servAddr;
struct sockaddr_in clientAddr;
int recv_msg_size;
char recv_msg[MAX_PACKET_SIZE];
char* filename;

void error(string str) {
	cout << str << endl;
	exit(1);
}

void initialize_server() {
	ifstream init("server.in");
	init >> servPort;//Well-known port number of server.
	init >> max_swnd;//Maximum sending sliding-window size
	init >> seed;//Random generator seed value
	init >> loss_prob;//Probability p of datagram loss
	init.close();
	cout << servPort << "  " << max_swnd << "  " << seed << "  " << loss_prob
			<< endl;

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

	//initialize random number generator
	srand(seed);
}

void receive_request() {
	//receive request
	/* Set the size of the in-out parameter */
	unsigned int cliAddrLen = sizeof(clientAddr);
	if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
			(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
		error("recvfrom() failed");
	printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));
	cout << recv_msg_size << endl;
	cout << recv_msg << endl;
}

bool packet_fall() {
	int r = rand % 11; //generate random number 0-10
	double d = r / 10; //d 0..1
	if (d < loss_prob)
		return true;
	return false;
}

void reply() {
	//reply with the file
	packet *request_packet = (packet*) recv_msg;
	filename = request_packet->data;
	ifstream file(filename);
	uint32_t last_seqno = request_packet->seqno - 1;
	char write_buff[MAX_DATA_SIZE];

	fd_set readfds;
	FD_SET(sock, &readfds);
	struct timeval tv;
	tv.tv_sec = 1;
	int n = sock + 1;
	//divide file into segments and send them
	while (!file.eof()) {
		packet * p = new packet();
		p->seqno = ++last_seqno;
		file.read(write_buff, MAX_DATA_SIZE);
		p->len = strlen(write_buff);
		bool acked = false;
		while (!acked) {
			//Check if packet should fall (loss simulation)
			if (!packet_fall()) {
				//send the segment
				if (sendto(sock, write_buff, p->len, 0,
						(struct sockaddr *) &clientAddr, sizeof(clientAddr))
						!= p->len)
					error(
							"sendto() sent a different number of bytes than expected");
			}
			//wait for ack or time out
			int rv = select(n, &readfds, NULL, NULL, &tv);
			if (rv == -1) {
				error("select"); // error occurred in select()
			} else if (rv == 0) {// time out
				cout << "Time Out" << endl;
			} else {
				unsigned int cliAddrLen = sizeof(clientAddr);
				if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE,
						0, (struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
					error("recvfrom() failed");
				ack_packet * ack = (ack_packet*) recv_msg;
				if (ack->ackno == last_seqno)//check whether this is the waited ack.
					acked = true;
			}
		}

		//		clock_t start_time = clock();
		//		clock_t curr_time = clock();
		//		double diff = (start_time - curr_time) / (CLOCKS_PER_SEC / 1000);
		//		bool sent = false;
		//		while (diff < time_out && !sent) {
		//
		//		}
	}
	file.close();
}

int main(int argc, char *argv[]) {
	initialize_server();
	receive_request();
	reply();
	return 0;
}
