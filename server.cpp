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
#include <pthread.h>
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

//----------------------------------------------
//Selective Repeat
bool is_sr;
int initial_swindow, window_base, available_cells, window_size, next_cell;
int *window;
pthread_t *threads;
//Selective Repeat
//----------------------------------------------

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
	if (is_sr) {
		initial_swindow = 1;//TODO
		window = (int*) (malloc(max_swnd * sizeof(int)));
		threads = (pthread_t*) (malloc(max_swnd * sizeof(pthread_t)));
		available_cells = initial_swindow;
		window_size = initial_swindow;
		window_base = 0;
		next_cell = 0;
		for (int i = 0; i < max_swnd; i++)
			if (i < initial_swindow)
				window[i] = NOT_SENT_AVAIL;
			else
				window[i] = NOT_AVAIL;
	}
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
	int r = rand() % 11; //generate random number 0-10
	cout << "random " << r << endl;
	double d = r / 10.0; //d 0..1
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

	//divide file into segments and send them
	while (!file.eof()) {
		packet * p = new packet();
		p->seqno = ++last_seqno;
		file.read(p->data, MAX_DATA_SIZE);
		p->len = strlen(p->data);
		if (file.eof()) {
			p->data[p->len] = EOF;
			p->len = p->len + 1;
		}
		bool acked = false;
		while (!acked) {
			//Check if packet should fall (loss simulation)
			if (!packet_fall()) {
				cout << "Sending seq " << p->seqno << "   len " << p->len
						<< endl;
				//						<< "  " << p->data << endl;
				//send the segment
				int t;
				if ((t = sendto(sock, p, sizeof(packet), 0,
						(struct sockaddr *) &clientAddr, sizeof(clientAddr)))
						!= sizeof(packet))
					error("send different number of bytes");
			} else {
				cout << "PAcket fall " << p->seqno << endl;
			}
			//wait for ack or time out
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(sock, &readfds);
			struct timeval tv;
			tv.tv_sec = 1;
			tv.tv_usec = 500000;
			int n = sock + 1;
			int rv = select(n, &readfds, NULL, NULL, &tv);
			if (rv == -1) {
				error("select"); // error occurred in select()
			} else if (rv == 0) {// time out
				cout << "Time Out" << endl << "Retransmission" << endl;
			} else {
				unsigned int cliAddrLen = sizeof(clientAddr);
				if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE,
						0, (struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
					error("recvfrom() failed");
				ack_packet * ack = (ack_packet*) recv_msg;
				if (ack->ackno == last_seqno)//check whether this is the waited ack.
					acked = true;
				cout << "Receive Ack" << ack->ackno << "  " << acked << endl;
			}
		}

	}
	file.close();
}

int main(int argc, char *argv[]) {
	is_sr = false;//not selective repeat for now
	initialize_server();
	receive_request();
	reply();
	return 0;
}
