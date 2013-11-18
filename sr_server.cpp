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
#include "sr_my_global.h"
#include <iostream>
#include <fstream>
#include <time.h>
#include <stdlib.h>     /* srand, rand */
#include <pthread.h>

using namespace std;

unsigned short servPort; //server,port
int max_swnd, seed, available_cells, base, available_cells, swnd, ssthresh;
float loss_prob;
int sock;
struct sockaddr_in servAddr;
struct sockaddr_in clientAddr;
int recv_msg_size;
char recv_msg[MAX_PACKET_SIZE];
char* filename;
int *window;
bool t_listen;

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

	base = 0;
	ssthresh = max_swnd / 2;
	window = (int*) (malloc(swnd * sizeof(int)));
	//initialize random number generator
	srand(seed);
}

void *listener_thread(void *in) {
	int cur_sock;
	cur_sock = (int) ((long) in);
	while (t_listen) {
		unsigned int cliAddrLen = sizeof(clientAddr);
		if ((recv_msg_size = recvfrom(cur_sock, recv_msg, MAX_PACKET_SIZE, 0,
				(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
			error("recvfrom() failed");
		ack_packet * ack = (ack_packet*) recv_msg;
		//new ack
		if (ack->ackno >= base) {
			window[(ack->ackno) % max_swnd] = ACKED;
			if (ack->ackno == base) {
				int c = 0;
				while (window[(base) % max_swnd] = ACKED) {
					window[(base) % max_swnd] = NOT_SENT_AVAIL;
					base++;
					c++;
				}
				available_cells += c;
			}
			if (swnd < max) {
				swnd = (2 * swnd <= ssthresh) ? 2 * swnd : swnd + 1;
			}
		}
	}
}

void *timer_thread(void *in) {
	int cur_sock;
	cur_sock = (int) ((long) in);

}

void handle_request() {
	//receive request
	/* Set the size of the in-out parameter */
	unsigned int cliAddrLen = sizeof(clientAddr);
	if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
			(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
		error("recvfrom() failed");
	printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));

	pthread_t listening_thread, timer_thread;
	pthread_create(&listening_thread, NULL, listener_thread, (void*) sock);
	pthread_create(&timer_thread, NULL, listener_thread, (void*) sock);

	//reply with the file
	packet *request_packet = (packet*) recv_msg;
	filename = request_packet->data;
	ifstream file(filename);
	uint32_t last_seqno = request_packet->seqno - 1;
	while (!file.eof()) {
		packet * p = new packet();
		p->seqno = ++last_seqno;
		file.read(p->data, MAX_DATA_SIZE);
		p->len = strlen(p->data);
		if (file.eof()) {
			p->data[p->len] = EOF;
			p->len = p->len + 1;
		}
		bool sent = false;
		while (!sent) {
			//if there is a cell in window size
			if (available_cells > 0) {
				//send packet
				if ((t = sendto(sock, p, sizeof(packet), 0,
						(struct sockaddr *) &clientAddr, sizeof(clientAddr)))
						!= sizeof(packet))
					error("send different number of bytes");
				sent = true;
			} else {
				//if no cells sleep
				usleep(WAIT_FOR_CELL);
			}
		}
	}

	cout << recv_msg_size << endl;
	cout << recv_msg << endl;
}

bool packet_fall() {
	int r = rand() % 11; //generate random number 0-10
	double d = r / 10.0; //d 0..1
	if (d < loss_prob)
		return true;
	return false;
}

int main(int argc, char *argv[]) {
	initialize_server();
	//TODO Multiple Clients
	handle_request();
	reply();
	return 0;
}

