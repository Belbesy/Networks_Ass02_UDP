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

int recv_msg_size;
char recv_msg[MAX_PACKET_SIZE];

string filename;
unsigned short servPort, clientPort; //server, Client port
char* serverIP;
int swnd;

char* to_char_pointer(string str) {
	char* ch = (char*) malloc(MAX_DATA_SIZE  * sizeof(char));
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

	int msgLen = strlen(to_char_pointer(filename)) + 1;
	
	fd_set readfds;
	FD_SET(sock, &readfds);
	struct timeval tv;
	tv.tv_sec = 1;
	int n = sock + 1;

	int last_seqno = 0;
	bool acked = false;
	// wait for first packet
	while(!acked){

		packet* p = new packet;
		p->seqno = 0;
		strcpy(p->data, filename.c_str());
		p->len = sizeof(p);
		int bytes_sent = sendto(sock, (char*) p, msgLen, 0, (struct sockaddr *) &servAddr, sizeof(servAddr));
		cout <<  "this is the file name (" << to_char_pointer(filename) <<  " sent bytes no:" << bytes_sent << endl;
		/* Send the request to the server */
		if (bytes_sent != msgLen)
			error("sendto() sent a different number of bytes than expected");

		int rv = select(n, &readfds, NULL, NULL, &tv);	

		if (rv == -1) {
			error("select"); // error occurred in select()
		} else if (rv == 0) {// time out
			cout << "Time Out Reciving the first packet" << endl;
			// will repeat loop and resend request
		} else {
			unsigned int cliAddrLen = sizeof(clientAddr);
			if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0, (struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
				error("recvfrom() failed");
			else
				acked = true;
		}
	}


	bool repeat =false;
	do
	{
		if(!repeat)
	{
		for(int i=0; i<recv_msg_size; i++){
			cout << (int) recv_msg[i] ;
		}
	}
		if(recv_msg[recv_msg_size-1]==EOF)
			break;

		// send ack of recieved packet
		ack_packet * ack = new ack_packet;
		ack->ackno = last_seqno;


		if(sendto(sock, (char*) ack, msgLen, 0, (struct sockaddr *) &servAddr, sizeof(servAddr))){
			error("sendto() failed while sending an ack packet");
		}


		// TODO check for last char if end of file break;

		FD_SET(sock, &readfds);
		int rv = select(n, &readfds, NULL, NULL, &tv);	

		if (rv == -1) {
			error("select"); // error occurred in select()
		} else if (rv == 0) {// time out
			cout << "Time Out Reciving the first packet" << endl;
			repeat = true;
			// will repeat loop and resend request
		} else {
			unsigned int cliAddrLen = sizeof(clientAddr);
			if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0, (struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
				error("recvfrom() failed");
			else{
				packet* p = (packet *) recv_msg;
				if(p->seqno ==last_seqno+1){
					acked = true;
					repeat = false;
					last_seqno++;
				}
				else{
					repeat = true;
				}
			}
		}

		/* code */
	} while (true);

	cout << "recieved" << endl;
	//response
	//	fromSize = sizeof(clientAddr);
	//	if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
	//			(struct sockaddr *) &fromAddr, &fromSize)) != echoStringLen)
	//		error("recvfrom() failed");
	close(sock);
	return 0;
}
