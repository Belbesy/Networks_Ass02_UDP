/*
 * global.h
 *
 *  Created on: Nov 16, 2013
 *      Author: raymond
 */

#define  MAX_PACKET_SIZE  500

/* Data-only packets */
struct packet {
	/* Header */
	uint16_t cksum; /* Optional bonus part */
	uint16_t len;
	uint32_t seqno;
	/* Data */
	char data[MAX_PACKET_SIZE];
	/* Not always 500 bytes, can be less */
};

/* Ack-only packets are only 8 bytes */
struct ack_packet {
	uint16_t cksum; /* Optional bonus part */
	uint16_t len;
	uint32_t ackno;
};
