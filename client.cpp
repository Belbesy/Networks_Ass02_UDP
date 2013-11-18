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

ofstream output;
string filename;
unsigned short servPort, clientPort; //server, Client port
char* serverIP;
int swnd;

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
	output.open(filename.c_str(), ios_base::app);
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

	// wait for first packet
	while (!acked) {

		p->seqno = 0;
		strcpy(p->data, filename.c_str());
		p->len = strlen(filename.c_str());
		int bytes_sent = sendto(sock, (char*) p, sizeof(packet), 0,
				(struct sockaddr *) &servAddr, sizeof(servAddr));
		cout << "this is the file name (" << filename.c_str() << endl;
		/* Send the request to the server */
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
	bool repeat = false;
	do {
		p = (packet*) recv_msg;
		bool stop = (p->data[p->len - 1] == EOF);
		if (!repeat) {
			//			for (int i = 0; i < recv_msg_size && recv_msg[i] != EOF; i++) {
			//				cout << recv_msg[i];
			//			}
			//			cout << endl;
			int total_bytes = (stop) ? p->len - 1 : p->len;
			output.write(p->data, total_bytes);
		}
		cout << "Receive Msg: seq " << p->seqno << "   len " << p->len << endl
				<< endl;
		//				<< p->data << endl;

		// send ack of recieved packet
		ack_packet * ack = new ack_packet;
		ack->ackno = last_seqno;
		ack->len = p->len;

		if (sendto(sock, (char*) ack, sizeof(ack_packet), 0,
				(struct sockaddr *) &servAddr, sizeof(servAddr))
				!= sizeof(ack_packet)) {
			error("sendto() failed while sending an ack packet");
		}
		cout << "is EOF" << stop << endl;
		if (stop)
			break;
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
			if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
					(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
				error("recvfrom() failed");
			else {
				packet* p = (packet *) recv_msg;
				if (p->seqno == last_seqno + 1) {
					acked = true;
					repeat = false;
					last_seqno++;
					cout << "Receive Msg: seq " << p->seqno << "   len "
							<< p->len << endl << p->data << endl;
				} else {
					repeat = true;
				}
			}
		}

		/* code */
	} while (true);
	output.close();
	cout << "recieved" << endl;
	close(sock);
	return 0;
}
