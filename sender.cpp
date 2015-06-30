#include "sender.hpp"
#include <string.h>

using namespace std;

void checkerror(int rtperr){
	if(rtperr < 0){
		cout << "Error: " << RTPGetErrorString(rtperr) << endl;
		exit(-1);
	}
}

Sender::Sender(){

}

Sender::~Sender(){

}

void Sender::OnAPPPacket(RTCPAPPPacket *apppacket, const RTPTime &receivetime, const RTPAddress *senderaddress){
	cout << "Got RTCP packet from:" << senderaddress << endl;
	cout << "Got RTCP subtype:" << apppacket->GetSubType() << endl;
	cout << "Got RTCP data:" << (char *)apppacket->GetAPPData() << endl;
}

void Sender::OnBYEPacket(RTPSourceData *srcdat){
	
}

void Sender::OnBYETimeout(RTPSourceData *srcdat){

}

void Sender::sendNalu(unsigned char *buf, int buflen){
	unsigned char *pbuf;
	pbuf = buf;

	char sendbuf[1430];		// Buffer for sending data
	memset(sendbuf, 0, 1430);

	int status;
	if(SD_Debug)
		printf("send packet length %d: \n", buflen);

	if(buflen <= MAX_RTP_PKT_LENGTH){
		memcpy(sendbuf, pbuf, buflen);
		status = this->SendPacket((void *)sendbuf, buflen);

		checkerror(status);
	}
	else if(buflen > MAX_RTP_PKT_LENGTH){
		this->SetDefaultMark(false);

		int k = 0, l = 0;
		k = buflen / MAX_RTP_PKT_LENGTH;
		l = buflen % MAX_RTP_PKT_LENGTH;
		int t = 0;

		char header = pbuf[0];
		while(t < k || (t == k && l > 0)){
			if((0 == t) || (t < k && 0 != t)){
				memcpy(sendbuf, &pbuf[t * MAX_RTP_PKT_LENGTH], MAX_RTP_PKT_LENGTH);
				status = this->SendPacket((void *)sendbuf, MAX_RTP_PKT_LENGTH);
				checkerror(status);
				t++;
			}	
			else if((k == t && l > 0) || (t == (k - 1) && l == 0)){
				this->SetDefaultMark(true);
				int iSendlen;
				if(l > 0){
					iSendlen = buflen - t * MAX_RTP_PKT_LENGTH;	
				}
				else
					iSendlen = MAX_RTP_PKT_LENGTH;
				memcpy(sendbuf, &pbuf[t * MAX_RTP_PKT_LENGTH], iSendlen);
				status = this->SendPacket((void *)sendbuf, iSendlen);

				checkerror(status);
				t++;
			}
		}
	}
}

void Sender::setParamForH264(){
	this->SetDefaultPayloadType(H264);
	this->SetDefaultMark(true);
	this->SetTimestampUnit(1.0/25.0);
	this->SetDefaultTimestampIncrement(3600);
}

// Maybe this could be used here. Have a try .
void Sender::setRTPParams(){
	int status;

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0/25.0);
	sessparams.SetAcceptOwnPackets(true);
	sessparams.SetUsePredefinedSSRC(true);
//	sessparams.SetPredefinedSSRC(SSRC);

	transparams.SetPortbase(baseport);

	status = this->Create(sessparams, &transparams);
	checkerror(status);

	RTPIPv4Address addr(dst_ip, destport);

	status = this->AddDestination(addr);
	checkerror(status);
}

void Sender::set_dst_ip(string ip){
	this->dst_ip = ntohl(inet_addr(ip.c_str()));
}

uint32_t Sender::get_dst_ip(void){
	return this->dst_ip;
}

void Sender::set_destport(uint16_t destport){
	this->destport = destport;
}

uint16_t Sender::get_destport(void){
	return this->destport;
}

void Sender::set_baseport(uint16_t baseport){
	this->baseport = baseport;
}

uint16_t Sender::get_baseport(void){
	return this->baseport;
}
