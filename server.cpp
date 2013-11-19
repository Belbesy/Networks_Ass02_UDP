/*
 * server.cpp
 *
 *  Created on: Nov 17, 2013
 *      Author: raymond
 */
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "my_global.h"

#include <time.h>
#include <stdlib.h>     /* srand, rand */
#include <pthread.h>
#include <vector>
using namespace std;



int max_swnd, seed;
float loss_prob;

void error(string str) {
	cout << str << endl;
	exit(1);
}


struct r_connection {
	sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	int socket; // file descriptor
	
	int last_recieved_seqno;
	int last_sent_seqno;

	char* fileName;

	int recv_msg_size;
	char recv_msg[MAX_PACKET_SIZE];

	ifstream file;
	


	bool packet_fall() {
		int r = rand() % 11; //generate random number 0-10
		cout << "random " << r << endl;
		double d = r / 10.0; //d 0..1
		if (d < loss_prob)
			return true;
		return false;
	}

	
	int recv_ack(){
		return recvfrom(socket, recv_msg, sizeof(ack_packet), 0, (struct sockaddr *) &clientAddress, &clientAddressLen);
	}

	int recv_request(){
		return recvfrom(socket, recv_msg, sizeof(packet), 0, (struct sockaddr *) &clientAddress, &clientAddressLen);
	}
	int send_packet(packet* p){
		return sendto(socket, p, sizeof(packet), 0, (struct sockaddr *) &clientAddress, clientAddressLen);
	}

	r_connection(){

	}

	r_connection(int socket_) {
	
		this->socket = socket_;
		/* Set the size of the in-out parameter */
		clientAddressLen = sizeof(clientAddress);
		if ((recv_msg_size = recv_request()) != sizeof(packet))
			error("recvfrom() failed");


		//cout << recv_msg_size << endl;
		//cout << recv_msg << endl;

		fileName = ((packet*) recv_msg)->data;
		last_recieved_seqno= ((packet*) recv_msg)->seqno - 1;
		last_sent_seqno = last_recieved_seqno;

		printf("Handling client %s, requesting file(%s)\n", inet_ntoa(clientAddress.sin_addr), fileName);
		file.open (fileName, std::ifstream::in);
	}

	bool ready_to_send(){
		return last_recieved_seqno==last_sent_seqno;
	}

	void recieve(){
		ack_packet* ack = (ack_packet*) recv_msg;

		if ((recv_msg_size = recv_ack()) != sizeof(ack_packet))
			error("recvfrom() failed");
		
		cout << "Receive Ack" << ack->ackno << endl;

		if (ack->ackno == last_sent_seqno){
			last_recieved_seqno = last_sent_seqno;
		 	cout << "acknowledged!" << endl;
		} else{
			cout << "Not acknowledged yet" << endl;
		}
	}

	void send(){
		//Check if packet should fall (loss simulation)
		if (!packet_fall()) {
			packet * p = new packet();
			p->seqno = ++last_sent_seqno;
			p->len = strlen(p->data);

			file.read(p->data, MAX_DATA_SIZE);
			if (file.eof()) {
				p->data[p->len] = EOF;
				p->len = p->len + 1;
				close();
			}

			cout << "Sending seq " << p->seqno << "   len " << p->len << endl;
			//send the segment
			int t;
			if ((t = send_packet(p)) != sizeof(packet))
				error("send different number of bytes");
		} else {
			cout << "Packet fall " << (last_sent_seqno+1) << endl;
		}
	}

	void close(){
		file.close();
		//TODO : implement close here unbind and stuff
	}

};


struct tcp_server{
	sockaddr_in servAddr;
	unsigned short servPort; //server,port
	
	r_connection* r_connections[100];
	int free_socket;
	
	fd_set readfds;
	fd_set readfds_w;
	
	int r_connections_size;
	int largest_socket;

	void create_free_socket(){
		/* Create socket for sending/receiving datagrams */
		if ((free_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			error("socket() failed");

		/* Bind to the local address */
		if (bind(free_socket, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
			error("bind() failed");

		largest_socket = free_socket > largest_socket ? free_socket : largest_socket;

		printf("--Socket(%d) created listening on port(%d)!\n", free_socket, htons(servAddr.sin_port));
	
	}	

	tcp_server(string config_file){
		ifstream init(config_file.c_str());
		init >> servPort;//Well-known port number of server.
		init >> max_swnd;//Maximum sending sliding-window size
		init >> seed;//Random generator seed value
		init >> loss_prob;//Probability p of datagram loss
		init.close();

		cout << servPort << "  " << max_swnd << "  " << seed << "  " << loss_prob << endl;

		/* Construct local address structure */
		memset(&servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servAddr.sin_port = htons(servPort);

		//initialize random number generator
		srand(seed);
		
		r_connections_size=0;
	}



	void run(){
		create_free_socket();
		int n;
		struct timeval tv;
		largest_socket =0;
		while(true){
	
			r_connection* con = new r_connection(free_socket);


			// init socket set to zeros
			// clear the set ahead of time
			FD_ZERO(&readfds_w);
			// add our descriptors to the set
			FD_SET(free_socket, &readfds_w);

			// since we got s2 second, it's the "greater", so we use that for
			// the n param in select()
			n = largest_socket + 1;

			// wait until either socket has data ready to be recv()d (timeout 10.5 secs)
			tv.tv_sec = 2;
			tv.tv_usec = 500000;
			int rv = select(n, &readfds_w, NULL, NULL, &tv);

			if (rv == -1) {
			    error(" error occured on select()"); // error occurred in select()
			} else if (rv == 0) {
			    printf("Timeout occurred!  No data after 2.5 seconds.\n");
			} else {
			    // one or both of the descriptors have data
			    if (FD_ISSET(free_socket, &readfds_w)){
			    	printf("Welcome Scoket is ready to recieve!\n");
			  		r_connections[r_connections_size++] = new r_connection(free_socket);
			    	// clear the set ahead of time
					FD_ZERO(&readfds_w);
					FD_SET(free_socket, &readfds);
					create_free_socket();
			    }
			    	
			    // check which was ready to recieve
				for(size_t c=0; c<r_connections_size; c++)
					if (FD_ISSET(r_connections[c]->socket, &readfds))
						r_connections[c]->recieve();
			}
			
			// check if any is ready to send
			for(size_t c=0; c<r_connections_size; c++)
				if (r_connections[c]->ready_to_send())
					r_connections[c]->send();
			
		}
	}
};


int main(int argc, char *argv[]) {
	tcp_server server("server.in");
	server.run();
	return 0;
}
