/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Xiang Rong - 442039 Add makefile to Embedded C client
 *******************************************************************************/

 /******************************************************************************
  * Modified by Nikolaos Papakonstantopoulos for load-balancer-eBPF project
  * 
  * This header file is used by eBPF - xdp program to parse MQTT packets
  * Due to eBPF limitations, some modifications were made to the original
  * MQTTPacket.h file from the Eclipse Paho Embedded C client,
  * such as 
  * - 	Functions modified to take less parameters,
  *   	as eBPF uses only registers r1-r5 for function parameters
  * - 	Inline functions to avoid function call overhead
  * - 	Strict bounds checking to avoid out-of-packet memory access (EACCESS errors),
  *     because eBPF verifier accepts only pointer dereferences within packet bounds
  ******************************************************************************/

#ifndef MQTTPACKET_H_
#define MQTTPACKET_H_

#define MQTT_PORT 1883

#include "MQTTPacket.h"

enum errors
{
	MQTTPACKET_BUFFER_TOO_SHORT = -2,
	MQTTPACKET_READ_ERROR = -1,
	MQTTPACKET_READ_COMPLETE
};

enum msgTypes
{
	CONNECT = 1, CONNACK, PUBLISH, PUBACK, PUBREC, PUBREL,
	PUBCOMP, SUBSCRIBE, SUBACK, UNSUBSCRIBE, UNSUBACK,
	PINGREQ, PINGRESP, DISCONNECT
};

/**
 * Bitfields for the MQTT header byte.
 */
// TODO: check REVERSED definition for eBPF - replace with 
/* 	#if defined(__LITTLE_ENDIAN_BITFIELD)
	#elif defined(__BIG_ENDIAN_BITFIELD)
	#else
	#error	"Adjust your <asm/byteorder.h> defines"
	#endif
*/
typedef union
{
	unsigned char byte;	                /**< the whole byte */
#if defined(REVERSED)
	struct
	{
		unsigned int type : 4;			/**< message type nibble */
		unsigned int dup : 1;				/**< DUP flag bit */
		unsigned int qos : 2;				/**< QoS value, 0, 1 or 2 */
		unsigned int retain : 1;		/**< retained flag bit */
	} bits;
#else
	struct
	{
		unsigned int retain : 1;		/**< retained flag bit */
		unsigned int qos : 2;				/**< QoS value, 0, 1 or 2 */
		unsigned int dup : 1;				/**< DUP flag bit */
		unsigned int type : 4;			/**< message type nibble */
	} bits;
#endif
} MQTTHeader;

typedef struct
{
	int len;
	char* data;
} MQTTLenString;

typedef struct
{
	char* cstring;
	MQTTLenString lenstring;
} MQTTString;

#define MQTTString_initializer {NULL, {0, NULL}}



//
// Parsing Functions from MQTTPacket.c 
//


/**
 * Calculates an 16 bit unsigned integer from two bytes read from the input buffer
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the integer value calculated
 * returns -1 on error
 */
__u16 readInt(unsigned char** pptr, unsigned char* enddata)
{
	unsigned char* ptr = *pptr;
	if(ptr + 2 > enddata)
		return -1;
	int len = 256*(*ptr) + (*(ptr+1));
	*pptr += 2;
	return len;
}

/**
 * Reads one character from the input buffer.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the character read
 */
char readChar(unsigned char** pptr)
{
	char c = **pptr;
	(*pptr)++;
	return c;
}


/**
 * @param mqttstring the MQTTString structure into which the data is to be read
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the data: do not read beyond
 * @return 1 if successful, 0 if not
 */
int readMQTTLenString(MQTTString* mqttstring, unsigned char** pptr, unsigned char* enddata)
{
	int rc = 0;

	/* the first two bytes are the length of the string */
	if (enddata - (*pptr) > 1) /* enough length to read the integer? */
	{
		mqttstring->lenstring.len = readInt(pptr, enddata); /* increments pptr to point past length */
		if ((mqttstring->lenstring.len > 0)   &&   (&(*pptr)[mqttstring->lenstring.len] <= enddata) )
		{
			mqttstring->lenstring.data = (char*)*pptr;
			*pptr += mqttstring->lenstring.len;
			rc = 1;
		}
	}
	mqttstring->cstring = NULL;
	return rc;
}


/**
 * Decodes the message length according to the MQTT algorithm
 * @param getcharfn pointer to function to read the next character from the data source
 * @param value the decoded length returned
 * @return the number of bytes read from the socket
 */
__attribute__((__always_inline__)) static inline int MQTTPacket_decode(int (*getcharfn)(unsigned char*, int), int* value)
{
	unsigned char c;
	int multiplier = 1;
	int len = 0;
#define MAX_NO_OF_REMAINING_LENGTH_BYTES 4

	*value = 0;
	do
	{
		int rc = MQTTPACKET_READ_ERROR;

		if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
		{
			rc = MQTTPACKET_READ_ERROR;	/* bad data */
			goto exit;
		}
		rc = (*getcharfn)(&c, 1);
		if (rc != 1)
			goto exit;
		*value += (c & 127) * multiplier;
		multiplier *= 128;
	} while ((c & 128) != 0);
exit:
	return len;
}


static unsigned char* bufptr;
static unsigned char* bufptr_end;

int bufchar(unsigned char* c, int count)
{
	int i;

	if(bufptr + count > bufptr_end)
		return MQTTPACKET_READ_ERROR;
	for (i = 0; i < count; ++i)
		*c = *bufptr++;
	return count;
}

__attribute__((__always_inline__)) static inline int MQTTPacket_decodeBuf(unsigned char* buf, unsigned char* buf_end, int* value)
{
	bufptr = buf;
	bufptr_end = buf_end;
	return MQTTPacket_decode(bufchar, value);
}



#endif /* MQTTPACKET_H_ */
