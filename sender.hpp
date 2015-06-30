#ifndef	_SENDER_H_
#define	_SENDER_H_

#include <jrtplib3/rtpsourcedata.h>
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtcpapppacket.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtperrors.h>

#include <arpa/inet.h>

#include <cstdlib>
#include <string>
#include <iostream>

#define	MAX_RTP_PKT_LENGTH	1360
#define	H264			96

#define SD_Debug	0

using namespace jrtplib;

/*void checkerror(int rtperr)
{
	if(rtperr < 0){
		std::cout << "Error:" << RTPGetErrorString(rtperr) << std::endl;
		exit(-1);
	}
}
*/

class Sender : public RTPSession
{
public:
	Sender(void);
	~Sender(void);

	void OnAPPPacket(RTCPAPPPacket *apppacket, const RTPTime &receivetime, const RTPAddress *senderaddress);
	void OnBYEPacket(RTPSourceData *srddat);
	void OnBYETimeout(RTPSourceData *srcdat);

	void sendNalu(unsigned char *buf, int buflen);
	void setParamForH264();
	void setRTPParams(void);

	void set_dst_ip(std::string ip);
	uint32_t get_dst_ip(void);	
	void set_destport(uint16_t destport);
	uint16_t get_destport(void);
	void set_baseport(uint16_t baseport);
	uint16_t get_baseport(void);
	
	
protected:
	uint32_t dst_ip;	
	uint16_t destport;
	uint16_t baseport;
};

#endif		// _SENDER_H_
