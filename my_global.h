/*
 * global.h
 *
 *  Created on: Nov 16, 2013
 *      Author: raymond
 */


#define  MAX_DATA_SIZE  10
#define  ACK_PACKET_SIZE 8


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

#define  MAX_PACKET_SIZE  MAX_DATA_SIZE + sizeof(packet)
/* Ack-only packets are only 8 bytes */
struct ack_packet {
	uint16_t cksum; /* Optional bonus part */
	uint16_t len;
	uint32_t ackno;
};
