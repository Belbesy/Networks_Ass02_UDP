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
#include <time.h>       /* clock_t, clock, CLOCKS_PER_SEC */
#include <sys/sem.h>

using namespace std;

unsigned short servPort; //server,port
int max_swnd, seed, base, available_cells, swnd, ssthresh, timer_count;
float loss_prob;
int sock;
struct sockaddr_in servAddr;
struct sockaddr_in clientAddr;
int recv_msg_size;
char recv_msg[MAX_PACKET_SIZE];
char* filename;
int *window;
packet ** packets;
//clock_t *timers;
time_t *timers;
bool t_listen, t_timer;
pthread_mutex_t mutex;
pthread_attr_t attr;

void error(string str) {
	cout << str << endl;
	exit(1);
}

bool packet_fall() {
	int r = rand() % 11; //generate random number 0-10
	double d = r / 10.0; //d 0..1
	if (d < loss_prob)
		return true;
	return false;
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
	swnd = 1;
	ssthresh = max_swnd / 2;
	window = (int*) (malloc(max_swnd * sizeof(int)));
	//	timers = (clock_t*) (malloc(max_swnd * sizeof(clock_t)));
	timers = (time_t*) (malloc(max_swnd * sizeof(clock_t)));
	packets = (packet**) (malloc(max_swnd * sizeof(packet*)));
	//initialize random number generator
	srand(seed);
	t_listen = true;
	t_timer = true;
	available_cells = swnd;
	cout << "initilze avaialble cells with " << available_cells << endl;
	for (int i = 0; i < max_swnd; i++) {
		window[i] = NOT_AVAIL;
	}
	pthread_mutex_init(&mutex, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
}

void *listener_thread(void *in) {
	cout << "ana gpwa e; listener" << endl;
	int cur_sock;
	cur_sock = (int) ((long) in);
	while (t_listen) {
		unsigned int cliAddrLen = sizeof(clientAddr);
		if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
				(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
			error("recvfrom() failed");
		//semwait
		pthread_mutex_lock(&mutex);
		ack_packet * ack = (ack_packet*) recv_msg;
		//new ack
		cout << "Ack no" << ack->ackno << endl;
		if (ack->ackno >= base) {
			int index = (ack->ackno) % max_swnd;
			window[index] = ACKED;
			if (ack->ackno == base) {
				int c = 0;
				while (window[(base) % max_swnd] == ACKED) {
					index = (base) % max_swnd;
					window[(base) % max_swnd] = NOT_SENT_AVAIL;
					base++;
					c++;
				}
				available_cells += c;
			}
			if (swnd < max_swnd) {
				int delta = swnd;
				swnd = (2 * swnd <= ssthresh) ? 2 * swnd : swnd + 1;
				delta = swnd - delta;
				//				available_cells += delta;
			}
		}
		//sempost
		pthread_mutex_unlock(&mutex);
	}
}

//bool time_to_wake(clock_t t) {
//	cout << t << endl;
//	double diff = clock() - t;
//	diff /= (CLOCKS_PER_SEC);
//	cout << diff << endl;
//	return diff > WAIT_ACK_TIME;
//}

bool time_to_wake(time_t t) {
	//	cout << t << endl;
	//	double diff = clock() - t;
	//	diff /= (CLOCKS_PER_SEC);
	//	cout << diff << endl;
	time_t now_t = time(NULL);
	cout << "NOWWWWW " << now_t << endl;
	//	time(&now_t);
	double diff = difftime(now_t, t);
	cout << "Diff " << diff << endl;
	return diff > WAIT_ACK_TIME;
}

void *timer_thread(void *in) {
	cout << "inside timer thread" << endl;
	int cur_sock;
	cur_sock = (int) ((long) in);
	while (t_timer) {
		if (timer_count != 0) {
			for (int i = 0; i < swnd; i++) {
				//semwait
				pthread_mutex_lock(&mutex);
				//				for (int i = 0; i < 13; i++)
				//					cout << window[i] << "  ";
				cout << endl;
				if (window[(base + i) % max_swnd] == SENT_NOT_ACKED) {
					//					clock_t t = timers[(base + i) % max_swnd];
					time_t t = timers[(base + i) % max_swnd];
					packet *p = packets[(base + i) % max_swnd];
					if (time_to_wake(t)) {
						cout << "Timeout of packet " << p->seqno << endl;
						//						time(&timers[(base + i) % max_swnd]);
						timers[(base + i) % max_swnd] = time(NULL);
						//						timers[(base + i) % max_swnd] = clock();
						int t;
						if ((t = sendto(sock, p, sizeof(packet), 0,
								(struct sockaddr *) &clientAddr,
								sizeof(clientAddr))) != sizeof(packet))
							error("send different number of bytes");
						ssthresh = swnd / 2;
						swnd = 1;
					}
				}
				//sempost
				pthread_mutex_unlock(&mutex);
			}
		}
	}
	usleep(WAIT_ACK_TIME / 5.0);
}

void handle_request() {
	//receive request
	/* Set the size of the in-out parameter */
	unsigned int cliAddrLen = sizeof(clientAddr);
	if ((recv_msg_size = recvfrom(sock, recv_msg, MAX_PACKET_SIZE, 0,
			(struct sockaddr *) &clientAddr, &cliAddrLen)) < 0)
		error("recvfrom() failed");
	printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));

	pthread_t listening_thread, timing_thread;
	pthread_create(&listening_thread, NULL, listener_thread, (void*) sock);
	pthread_create(&timing_thread, NULL, timer_thread, (void*) sock);

	//reply with the file
	packet *request_packet = (packet*) recv_msg;
	filename = request_packet->data;
	ifstream file(filename);
	//	uint32_t last_seqno = request_packet->seqno - 1;
	uint32_t last_seqno = -1;
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
				cout << "swnd : " << swnd << " sshtreash : " << ssthresh
						<< " avail : " << available_cells << " max : "
						<< max_swnd << endl;
				//send packet
				if (!packet_fall()) {
					//semwait
					pthread_mutex_lock(&mutex);
					int t;
					if ((t
							= sendto(sock, p, sizeof(packet), 0,
									(struct sockaddr *) &clientAddr,
									sizeof(clientAddr))) != sizeof(packet))
						error("send different number of bytes");
					cout << "Send a new packet len " << p->len << "  seq "
							<< p->seqno << endl;
				} else {
					cout << " Fall new packet len " << p->len << "  seq "
							<< p->seqno << endl;
					//							<< " data " << p->data << endl;
				}
				//				if(seq)
				//				clock_t now_t = clock();
				//				cout << now_t << endl;
				//				timers[(p->seqno) % max_swnd] = now_t;
				//				time(&timers[(p->seqno) % max_swnd]);
				time_t t = time(NULL);
				cout << "time " << t << endl;
				timers[(p->seqno) % max_swnd] = t;
				packets[(p->seqno) % max_swnd] = p;
				timer_count++;
				sent = true;
				available_cells--;
				window[(p->seqno) % max_swnd] = SENT_NOT_ACKED;
				pthread_mutex_unlock(&mutex);
				//sempost
			} else {
				//				cout << "waiting for a cell " << endl;
				//if no cells sleep
				usleep(WAIT_FOR_CELL);
			}
		}
	}

}

int main(int argc, char *argv[]) {
	initialize_server();
	//TODO Multiple Clients
	handle_request();
	return 0;
}

