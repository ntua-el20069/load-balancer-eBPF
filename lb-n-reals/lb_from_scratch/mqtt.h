#ifndef __MQTT_H
#define __MQTT_H

#define MQTT_PORT 1883

#include "StackTrace.h"
#include "MQTTPacket.h"

#ifndef __BPF_PROG_INCLUDES
#include <bpf_helpers.h>
#endif

/**
  * Deserializes the supplied (wire) buffer into publish data
  * @param dup returned integer - the MQTT dup flag
  * @param qos returned integer - the MQTT QoS value
  * @param retained returned integer - the MQTT retained flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param topicName returned MQTTString - the MQTT topic in the publish
  * @param payload returned byte buffer - the MQTT publish payload
  * @param payloadlen returned integer - the length of the MQTT payload
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */

//   removed params dup, qos, retained, packetid, payloadlen due to eBPF calling convention limit

/*
The function was limited to take only 5 params due to the calling convention of eBPF.
https://docs.ebpf.io/linux/concepts/functions/
""" A function can take up to 5 arguments, limited by the calling convention. """"
*/
__attribute__((__always_inline__)) static inline int MQTTDeserialize_publish(unsigned int * packet_type,  MQTTString* topicName,
		unsigned char* buf, unsigned char* enddata)
{	
	unsigned char* curdata = buf;
	
	int rc = 0;
	int mylen = 0;

	FUNC_ENTRY;
	MQTTHeader * header = (MQTTHeader *) curdata;
	if ((void *)(header + 1) > (void *)enddata){
		bpf_printk("Not enough data for MQTT header");
		goto exit;
	}
	// forward pointer by a char
	readChar(&curdata);
    *packet_type = header->bits.type;
	if (header->bits.type != PUBLISH){
		bpf_printk("Not a PUBLISH packet type: %d", header->bits.type);
		goto exit;
	}

	curdata += (rc = MQTTPacket_decodeBuf(curdata, enddata, &mylen)); /* read remaining length */
	if(enddata != curdata + mylen)
	{
		bpf_printk("Remaining length does not match data length");
		goto exit;
	}

	int ret = 0;
	if (!(ret = readMQTTLenString(topicName, &curdata, enddata)) ||
		enddata - curdata < 0){ /* do we have enough data to read the protocol version byte? */
		bpf_printk("Failed to read topic name or not enough data");
		bpf_printk("ret: %d, topic len: %d, curdata-enddata: %d", ret, topicName->lenstring.len, enddata - curdata);
		goto exit;
	}

	rc = 1;
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


#endif // of __MQTT_H