
#ifndef _MEDIARTPPACKET_H
#define _MEDIARTPPACKET_H

#include "media_general.h"

int NET_SendRtpByUdp(int _iSockFd, uint8_t payloadtype, uint16_t *pusSequencenumber, uint32_t timestamp, uint32_t ssrc, DataInfo *_pstDataInfo, int _iIovNum);
int NET_SendRtpByTcp(int _iSockFd, uint8_t payloadtype, uint16_t *pusSequencenumber, uint32_t timestamp, uint32_t ssrc, DataInfo *_pstDataInfo, int _iIovNum);

#endif //_MEDIARTPPACKET_H

