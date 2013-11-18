/*
 * global.h
 *
 *  Created on: Nov 16, 2013
 *      Author: raymond
 */

#define  MAX_PACKET_SIZE  508
#define  MAX_DATA_SIZE  500
#define  ACK_PACKET_SIZE 8
#define WAIT_FOR_CELL 1000
#define WAIT_ACK_TIME 1
static int SENT_ACKETD = 1, SENT_NOT_ACKED = 2, NOT_SENT_AVAIL = 3, NOT_AVAIL =
		4, ACKED = 5, NOT_ACKED = 6;

/* Data-only packets */
struct packet {
	/* Header */
	uint16_t cksum; /* Optional bonus part */
	uint16_t len;
	uint32_t seqno;
	/* Data */
	char data[MAX_DATA_SIZE];
	/* Not always 500 bytes, can be less */
};

/* Ack-only packets are only 8 bytes */
struct ack_packet {
	uint16_t cksum; /* Optional bonus part */
	uint16_t len;
	uint32_t ackno;
};
