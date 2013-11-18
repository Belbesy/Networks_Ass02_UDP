/*
 * sr_client.cpp
 *
 *  Created on: Nov 18, 2013
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

using namespace std;

int recv_msg_size;
char recv_msg[MAX_PACKET_SIZE];

string filename;
unsigned short servPort, clientPort; //server, Client port
char* serverIP;
int swnd, base, waiting_seq;
int* window;
char* chunck;//store out of order packets in it to store in file after receiving in order packets

ofstream output;

char* to_char_pointer(string str) {
	char* ch = (char*) malloc(MAX_DATA_SIZE * sizeof(char));
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
	base = 0;
	window = (int*) (malloc(swnd * sizeof(int)));
	chunck = (char*) (malloc)(swnd * MAX_DATA_SIZE * sizeof(char));
	output.open(filename.c_str());
	waiting_seq = 0;
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

	fd_set readfds;
	FD_SET(sock, &readfds);
	struct timeval tv;
	tv.tv_sec = 1;
	int n = sock + 1;

	int last_seqno = 0;
	bool acked = false;
	packet* p = new packet;

	/*// wait for first packet
	 while (!acked) {

	 p->seqno = 0;
	 strcpy(p->data, filename.c_str());
	 p->len = strlen(filename.c_str());
	 int bytes_sent = sendto(sock, (char*) p, sizeof(packet), 0,
	 (struct sockaddr *) &servAddr, sizeof(servAddr));
	 cout << "this is the file name (" << filename.c_str() << endl;
	 if (bytes_sent != sizeof(packet))
	 error("sendto() sent a different number of bytes than expected");

	 int rv = select(n, &readfds, NULL, NULL, &tv);

	 if (rv == -1) {
	 error("select"); // error occurred in select()
	 } else if (rv == 0) {// time out
	 cout << "Time Out Reciving the first packet" << endl;
	 // will repeat loop and resend request
	 } else {
	 unsigned int cliAddrLen = sizeof(clientAddr);
	 if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
	 (struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
	 error("recvfrom() failed");
	 else
	 acked = true;
	 }
	 }
	 */
	p->seqno = 0;
	strcpy(p->data, filename.c_str());
	p->len = strlen(filename.c_str());
	int bytes_sent = sendto(sock, (char*) p, sizeof(packet), 0,
			(struct sockaddr *) &servAddr, sizeof(servAddr));
	cout << "this is the file name (" << filename.c_str() << endl;
	/* Send the request to the server */
	if (bytes_sent != sizeof(packet))
		error("sendto() sent a different number of bytes than expected");

	int count = 0;//received packets till an order in-order one
	bool end = false;//reach end of file
	while (!end) {
		unsigned int cliAddrLen = sizeof(clientAddr);
		if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
				(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
			error("recvfrom() failed");
		//		cout << "receive packet" << endl;
		packet* p = (packet *) recv_msg;
		cout << "Receive packet len " << p->len << "  seq" << p->seqno << endl;
		if (p->seqno < waiting_seq || window[(p-> seqno) % swnd] == ACKED) {
			cout << "Ack out of date" << endl;
			//neglect it
		} else {
			count++;
			window[(p-> seqno) % swnd] = ACKED;
			// send ack of recieved packet
			ack_packet * ack = new ack_packet;
			ack->ackno = p->seqno;
			ack->len = ACK_PACKET_SIZE;
			if (sendto(sock, (char*) ack, sizeof(ack_packet), 0,
					(struct sockaddr *) &servAddr, sizeof(servAddr))
					!= sizeof(ack_packet)) {
				error("sendto() failed while sending an ack packet");
			}
			cout << ack->ackno << " Sent" << endl;
			int offset = (p->seqno - waiting_seq) * MAX_DATA_SIZE;
			//store received packet
			memcpy(&chunck[offset], p->data, p->len);
			if (p->seqno == waiting_seq) {
				cout << "Saving to file" << endl;
				//save to file
				int total_bytes = (count - 1) * MAX_DATA_SIZE + p->len - 1;
				output.write(chunck, total_bytes);
				for (int i = waiting_seq; i < waiting_seq + count; i++) {
					window[(i) % swnd] = NOT_ACKED;
				}
				waiting_seq += count;
				count = 0;
				end = (p->data[p->len - 1] == EOF);
			}
		}

	}

	cout << "recieved" << endl;
	output.close();
	close(sock);
	return 0;
}

