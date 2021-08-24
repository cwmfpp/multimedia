
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include "media_rtppacket.h"
#include "media_log.h"

typedef struct RTPHeader
{
#ifdef RTP_BIG_ENDIAN
    uint32_t version:2;
    uint32_t padding:1;
    uint32_t extension:1;
    uint32_t csrccount:4;
    
    uint32_t marker:1;
    uint32_t payloadtype:7;
#else // little endian
    uint32_t csrccount:4;
    uint32_t extension:1;
    uint32_t padding:1;
    uint32_t version:2;
    
    uint32_t payloadtype:7;
    uint32_t marker:1;
#endif // RTP_BIG_ENDIAN
    
    uint16_t sequencenumber;
    uint32_t timestamp;
    uint32_t ssrc;
}RTPHeader;

typedef struct RTPHeaderTCP
{
    uint32_t length:16;
#ifdef RTP_BIG_ENDIAN
    uint32_t version:2;
    uint32_t padding:1;
    uint32_t extension:1;
    uint32_t csrccount:4;
    
    uint32_t marker:1;
    uint32_t payloadtype:7;
#else // little endian
    uint32_t csrccount:4;
    uint32_t extension:1;
    uint32_t padding:1;
    uint32_t version:2;
    
    uint32_t payloadtype:7;
    uint32_t marker:1;
#endif // RTP_BIG_ENDIAN
    
    uint16_t sequencenumber;
    uint32_t timestamp;
    uint32_t ssrc;

}__attribute__((packed)) RTPHeaderTCP ;

static int RtpHeaderPack(RTPHeader *rtphdr, uint8_t marker, uint8_t payloadtype, uint16_t sequencenumber, uint32_t timestamp, uint32_t ssrc)
{
    if(NULL == rtphdr)
    {
        Media_Error("rtphdr(%p) is invalid\n", rtphdr);
        return -1;
    }
    
    rtphdr->version = 2;// 1
    rtphdr->padding = 0;
    rtphdr->extension = 0;
    rtphdr->csrccount = 0;
    rtphdr->marker = marker & 1;
    rtphdr->payloadtype = payloadtype & 0x7f;
    rtphdr->sequencenumber = htons(sequencenumber);
    rtphdr->timestamp = htonl(timestamp);
    rtphdr->ssrc = htonl(ssrc);
    //dbgprint("sequencenumber = %d", sequencenumber);
    //dbgprint("timestamp = %d", timestamp);
    return 0;
}


static int RtpHeaderPackMark(RTPHeader *rtphdr)
{
    rtphdr->marker = 1;
    return 0;
}

static int RtpHeaderPackUpdateSeq(RTPHeader *rtphdr, uint16_t sequencenumber)
{
    static unsigned short usSeqOld = 0;

    if(0 == usSeqOld)
    {
        usSeqOld = sequencenumber;
    }else
    {
        if((usSeqOld + 1) != sequencenumber)
        {
            Media_Error("usSeqOld=%d sequencenumber=%d", usSeqOld, sequencenumber);
        }

        usSeqOld = sequencenumber;
    }
//    printf("%d %s sequencenumber=%d  \n", __LINE__, __FUNCTION__, sequencenumber);
    rtphdr->sequencenumber = htons(sequencenumber);
    return 0;    
}

#if 0
static int RtpHeaderPackGetSeq(RTPHeader *rtphdr)
{
    return htons(rtphdr->sequencenumber);    
}
#endif

///////////////tcp
static int RtpHeaderPackTcp(RTPHeaderTCP *rtphdr, uint8_t marker, uint8_t payloadtype, uint16_t sequencenumber, uint32_t timestamp, uint32_t ssrc)
{
    if(NULL == rtphdr)
    {
        Media_Error("rtphdr(%p) is invalid\n", rtphdr);
        return -1;
    }
    
    rtphdr->version = 2;// 1
    rtphdr->padding = 0;
    rtphdr->extension = 0;
    rtphdr->csrccount = 0;
    rtphdr->marker = marker & 1;
    rtphdr->payloadtype = payloadtype & 0x7f;
    rtphdr->sequencenumber = htons(sequencenumber);
    rtphdr->timestamp = htonl(timestamp);
    rtphdr->ssrc = htonl(ssrc);

    return 0;
}


static int RtpHeaderPackMarkTcp(RTPHeaderTCP *rtphdr)
{
    rtphdr->marker = 1;
    return 0;
}

static int RtpHeaderPackUpdateSeqTcp(RTPHeaderTCP *rtphdr, uint16_t sequencenumber)
{
    static unsigned short usSeqOld = 0;

    if(0 == usSeqOld)
    {
        usSeqOld = sequencenumber;
    }else
    {
        if((usSeqOld + 1) != sequencenumber)
        {
            Media_Error("usSeqOld=%d sequencenumber=%d", usSeqOld, sequencenumber);
        }

        usSeqOld = sequencenumber;
    }
//    printf("%d %s sequencenumber=%d  \n", __LINE__, __FUNCTION__, sequencenumber);
    rtphdr->sequencenumber = htons(sequencenumber);
    return 0;    
}

#if 0
static int RtpHeaderPackGetSeqTcp(RTPHeaderTCP *rtphdr)
{
    return htons(rtphdr->sequencenumber);    
}
#endif

static int RtpHeaderPackSetTcpLen(RTPHeaderTCP *rtphdr, unsigned short _usLen)
{
    rtphdr->length = htons(_usLen);
    return 0;    
}

#define RTP_PAYLOAD_LEN     1400

#if 0
static void PrintHexData(unsigned char *_pData, int _pDataLen, int _displayCount)
{
    printf("start _pData _pDataLen=%d", _pDataLen);
    int i = 0;

    printf("\n");
    for(i = 0; i < _pDataLen; i++)
    {
        if(_pData[i] < 0x10) {
            printf("0x0%x, ", _pData[i]);
        } else {
            printf("0x%x, ", _pData[i]);
        }
        if((!((i + 1) %_displayCount)) && (i > 0))
        {
            printf("\n");
        }
    }
    printf("\n");
    printf("\n");
    printf("end _pData\n");

    return;
}
#endif

#if 0
static int WriteFile(struct iovec *_piov, int _iNum)
{
    int i = 0;
    int iRet = 0;
    static int iFd = 0;
    if(0 == iFd)
    {
        iFd = open("ps_rtp", O_RDWR|O_APPEND|O_CREAT, 0644);
    }
    
    Media_Debug("====================================write file end_iNum=%d\n", _iNum);

    for(i = 0; i < _iNum; i++)
    {
        iRet = write(iFd, _piov[i].iov_base, _piov[i].iov_len);
    }

    return iRet;
}
#endif

static int SocketSend(const int _iSockFd, const char* _pcBuffer, int _iBuflen)
{
    int tmp = 0;
    int total = _iBuflen;
    const char *p = _pcBuffer;
    int iEagainCount = 0;

    if((_iSockFd <= 0) || (NULL == _pcBuffer) || (_iBuflen <= 0))
    {
        Media_Error("invalid _iSockFd(%d) _pcBuffer(%p) _iBuflen(%d)", _iSockFd, _pcBuffer, _iBuflen);
        tmp = -1;
        goto end;
    }
    

#define EAGAIN_COUNT    100
    while(1)
    {
        tmp = send(_iSockFd, p, (unsigned int)total, 0);

        if(tmp < 0)
        {
            if(errno == EAGAIN || errno == EINTR)
            {       
                Media_Error("errno == EAGAIN iEagainCount=%d", iEagainCount);
                iEagainCount++;
                if(iEagainCount > EAGAIN_COUNT)
                {
                    Media_Error("iEagainCount(%d) > EAGAIN_COUNT(%d) failed", iEagainCount, EAGAIN_COUNT);
                    tmp = -1;
                    goto end;
                }
                usleep(10 * 1000);
                continue;
            }
            
            tmp = -1;
            goto end;
        }

        if((int)tmp == total)
        {
            tmp = _iBuflen;
            goto end;
        }
        
        total -= tmp;
        p += tmp;
    }

end:

    return tmp;
}

static int  SendDataUdp(int iSockfd, struct iovec *_pIovec, int _iIovNum)
{
    int iRet = 0;

    if(0 == _iIovNum || 1 == _iIovNum)
    {
        Media_Debug("_iIovNum=%d \n", _iIovNum);
		goto end;
    }

    iRet = writev(iSockfd, _pIovec, _iIovNum);	

end:
    
    return iRet;
}

static int  SendDataTcp(int iSockfd, struct iovec *_pIovec, int _iIovNum)
{
    int iRet = 0;
    int i = 0;

    //iRet = writev(iSockfd, _pIovec, _iIovNum);

	
    //Media_Debug("send send iRet=%d\n", iRet);
    if(0 == _iIovNum || 1 == _iIovNum)
    {
        Media_Debug("_iIovNum=%d \n", _iIovNum);
    }

    
    for(i = 0; i < _iIovNum; i++)
    {
        iRet = SocketSend(iSockfd, (const char*)(_pIovec[i].iov_base), (int)(_pIovec[i].iov_len));
        if(iRet < 0)
        {
            Media_Error("call SocketSend failed!");
            iRet = -1;
            goto end;
        }
    }

end:
    
    return iRet;
    #if 0
    int i = 0;
    WriteFile(_pIovec, _iIovNum);
    
    return 0;
    for(i = 0; i < _iIovNum; i++)
    {
        PrintHexData(_pIovec[i].iov_base, _pIovec[i].iov_len, 16);
    }

    return 0;
    #endif
}

#define RTP_IOVEC_SEND_NUM      50


int NET_SendRtpByUdp(int _iSockFd, uint8_t payloadtype, uint16_t *pusSequencenumber, uint32_t timestamp, uint32_t ssrc, DataInfo *_pstDataInfo, int _iIovNum)
{
    int iRet = 0;
    int iSocketFd = 0;
    RTPHeader rtphdr = {0};
    int iIovecSendIndex = 0;
    uint16_t usSeq = 0;
    int i = 0;
    unsigned int uiCurrLen = 0;
    struct iovec iovecRtpSend[RTP_IOVEC_SEND_NUM];

    iSocketFd = _iSockFd;
    usSeq = *pusSequencenumber;
    //dbgprint("timestamp %d", timestamp);
    if(RtpHeaderPack(&rtphdr, 0, payloadtype, usSeq, timestamp, ssrc) < 0)
    {
        iRet = -1;
        Media_Error("call RtpHeaderPack failed!");
        goto end;
    }

    iovecRtpSend[iIovecSendIndex].iov_base = &rtphdr;
    iovecRtpSend[iIovecSendIndex].iov_len = sizeof(RTPHeader);
    iIovecSendIndex++;
    
    Media_Debug("RTPHeader=%ld\n", sizeof(RTPHeader));
    
    Media_Debug("RTPHeaderTCP=%ld\n", sizeof(RTPHeaderTCP));
    for(i = 0; i < _iIovNum; )
    {
        if(iIovecSendIndex >= RTP_IOVEC_SEND_NUM)
        {
            Media_Error("iIovecSendIndex(%d) >= RTP_IOVEC_SEND_NUM(%d)", iIovecSendIndex, RTP_IOVEC_SEND_NUM);
            iRet = -1;
            break;
        }
        
        if((uiCurrLen + _pstDataInfo[i].m_uiLen) > RTP_PAYLOAD_LEN)
        {
            //send
            char *pDataAddr = (char *)_pstDataInfo[i].m_pvAddr;
            unsigned int uiDataLen = _pstDataInfo[i].m_uiLen;
            unsigned int uiValidLen = 0;
            
            Media_Debug("aaaaaaaaaa\n");

            while(uiDataLen > 0)
            {
                if((uiCurrLen + uiDataLen) > RTP_PAYLOAD_LEN)
                {
                    uiValidLen = RTP_PAYLOAD_LEN - uiCurrLen;
                    uiCurrLen = 0;
                    Media_Debug("1111111111\n");
                    
                }else
                {
                    if(uiDataLen > RTP_PAYLOAD_LEN)
                    {
                        uiValidLen = RTP_PAYLOAD_LEN;                        
                    }else
                    {
                        uiValidLen = uiDataLen;
                        uiCurrLen = uiDataLen;
                    }
                }
                
                iovecRtpSend[iIovecSendIndex].iov_base = pDataAddr;
                iovecRtpSend[iIovecSendIndex].iov_len = uiValidLen;
                iIovecSendIndex++;
                
                pDataAddr += uiValidLen;
                uiDataLen -= uiValidLen;
                uiValidLen = 0;

                if(0 == uiDataLen)
                {
                    if(i == (_iIovNum - 1))
                    {
                        //mark
                        //wirtev end
                        if(RtpHeaderPackMark(&rtphdr) < 0)
                        {
                            Media_Error("call RtpHeaderPackMark failed!");
                            iRet = -1;
                            goto end;
                        }
                        usSeq++;
                        if(RtpHeaderPackUpdateSeq(&rtphdr, usSeq) < 0)
                        {
                            Media_Error("call RtpHeaderPackUpdateSeq failed!");
                            iRet = -1;
                            goto end;
                        }
                        Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                        iRet = SendDataUdp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                        iIovecSendIndex = 1;
                        uiCurrLen = 0;
                    }else
                    {
                        //
                        Media_Debug("555555555\n");
                    }
                    i++;
                }else
                {
                    //writev
                    Media_Debug("66666666666\n");
                    usSeq++;
                    if(RtpHeaderPackUpdateSeq(&rtphdr, usSeq) < 0)
                    {
                        Media_Error("call RtpHeaderPackUpdateSeq failed!");
                        iRet = -1;
                        goto end;
                    }
                    Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                    iRet = SendDataUdp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                    iIovecSendIndex = 1;
                    uiCurrLen = 0;

                }
            }
            
            
        }else
        {
            uiCurrLen += _pstDataInfo[i].m_uiLen;
            
            iovecRtpSend[iIovecSendIndex].iov_base = _pstDataInfo[i].m_pvAddr;
            iovecRtpSend[iIovecSendIndex].iov_len = _pstDataInfo[i].m_uiLen;
            iIovecSendIndex++;

            if(i == (_iIovNum - 1))
            {
                //mark
                //wirtev end
                Media_Debug("7777777777777\n");
                if(RtpHeaderPackMark(&rtphdr) < 0)
                {
                    Media_Error("call RtpHeaderPackMark failed!");
                    iRet = -1;
                    goto end;
                }
                usSeq++;
                if(RtpHeaderPackUpdateSeq(&rtphdr, usSeq) < 0)
                {
                    Media_Error("call RtpHeaderPackUpdateSeq failed!");
                    iRet = -1;
                    goto end;
                }
                Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                iRet = SendDataUdp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                iIovecSendIndex = 1;
                uiCurrLen = 0;
            }

            if(RTP_PAYLOAD_LEN == uiCurrLen)
            {
                    Media_Debug("8888888888888\n");
                    usSeq++;
                    if(RtpHeaderPackUpdateSeq(&rtphdr, usSeq) < 0)
                    {
                        Media_Error("call RtpHeaderPackUpdateSeq failed!");
                        iRet = -1;
                        goto end;
                    }
                    Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                    iRet = SendDataUdp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                    iIovecSendIndex = 1;
                    uiCurrLen = 0;
            }

            i++;
        }
    }

    *pusSequencenumber = usSeq;

end:
    return iRet;
}



int NET_SendRtpByTcp(int _iSockFd, uint8_t payloadtype, uint16_t *pusSequencenumber, uint32_t timestamp, uint32_t ssrc, DataInfo *_pstDataInfo, int _iIovNum)
{
    int iRet = 0;
    int iSocketFd = 0;
    RTPHeaderTCP rtphdr = {0};
    int iIovecSendIndex = 0;
    uint16_t usSeq = 0;
    int i = 0;
    unsigned int uiCurrLen = 0;
    struct iovec iovecRtpSend[RTP_IOVEC_SEND_NUM];

    iSocketFd = _iSockFd;
    usSeq = *pusSequencenumber;
    if(RtpHeaderPackTcp(&rtphdr, 0, payloadtype, usSeq, timestamp, ssrc) < 0)
    {
        Media_Error("call RtpHeaderPackTcp failed!");
        iRet = -1;
        goto end;

    }

    iovecRtpSend[iIovecSendIndex].iov_base = &rtphdr;
    iovecRtpSend[iIovecSendIndex].iov_len = sizeof(RTPHeaderTCP);
    iIovecSendIndex++;
    
    for(i = 0; i < _iIovNum; )
    {
        if(iIovecSendIndex >= RTP_IOVEC_SEND_NUM)
        {
            Media_Error("iIovecSendIndex(%d) >= RTP_IOVEC_SEND_NUM(%d)", iIovecSendIndex, RTP_IOVEC_SEND_NUM);
            iRet = -1;
            break;
        }
        
        if((uiCurrLen + _pstDataInfo[i].m_uiLen) > RTP_PAYLOAD_LEN)
        {
            //send
            char *pDataAddr = (char*)_pstDataInfo[i].m_pvAddr;
            unsigned int uiDataLen = _pstDataInfo[i].m_uiLen;
            unsigned int uiValidLen = 0;
            
            Media_Debug("aaaaaaaaaa\n");

            while(uiDataLen > 0)
            {
                if((uiCurrLen + uiDataLen) > RTP_PAYLOAD_LEN)
                {
                    uiValidLen = RTP_PAYLOAD_LEN - uiCurrLen;
                    uiCurrLen = 0;                   
                }else
                {
                    if(uiDataLen > RTP_PAYLOAD_LEN)
                    {
                        uiValidLen = RTP_PAYLOAD_LEN;                        
                    }else
                    {
                        uiValidLen = uiDataLen;
                        uiCurrLen = uiDataLen;
                    }
                }
                
                iovecRtpSend[iIovecSendIndex].iov_base = pDataAddr;
                iovecRtpSend[iIovecSendIndex].iov_len = uiValidLen;
                iIovecSendIndex++;
                
                pDataAddr += uiValidLen;
                uiDataLen -= uiValidLen;
                uiValidLen = 0;

                if(0 == uiDataLen)
                {
                    if(i == (_iIovNum - 1))
                    {
                        //mark
                        //wirtev end
                        Media_Debug("444444444\n");
                        if(RtpHeaderPackMarkTcp(&rtphdr) < 0)
                        {
                            Media_Error("call RtpHeaderPackMarkTcp failed!");
                            iRet = -1;
                            goto end;
                        }
                        usSeq++;
                        if(RtpHeaderPackUpdateSeqTcp(&rtphdr, usSeq) < 0)
                        {
                            Media_Error("call RtpHeaderPackUpdateSeqTcp failed!");
                            iRet = -1;
                            goto end;
                        }
                        Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                        if(0 == uiCurrLen)
                        {
                            uiCurrLen = RTP_PAYLOAD_LEN;
                        }
                        uiCurrLen += 12;
                        if(RtpHeaderPackSetTcpLen(&rtphdr, (unsigned short)uiCurrLen) < 0)
                        {
                            Media_Error("call RtpHeaderPackSetTcpLen failed!");
                            iRet = -1;
                            goto end;
                        }
                        iRet = SendDataTcp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                        iIovecSendIndex = 1;
                        uiCurrLen = 0;
                    }else
                    {
                        //
                        Media_Debug("555555555\n");
                    }
                    i++;
                }else
                {
                    //writev
                    Media_Debug("66666666666\n");
                    usSeq++;
                    if(RtpHeaderPackUpdateSeqTcp(&rtphdr, usSeq) < 0)
                    {
                        Media_Error("call RtpHeaderPackUpdateSeqTcp failed!");
                        iRet = -1;
                        goto end;
                    }
                    Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                    if(0 == uiCurrLen)
                    {
                        uiCurrLen = RTP_PAYLOAD_LEN;
                    }
                    uiCurrLen += 12;
                    if(RtpHeaderPackSetTcpLen(&rtphdr, (unsigned short)uiCurrLen) < 0)
                    {
                        Media_Error("call RtpHeaderPackSetTcpLen failed!");
                        iRet = -1;
                        goto end;
                    }
                    iRet = SendDataTcp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                    iIovecSendIndex = 1;
                    uiCurrLen = 0;

                }
            }
            
            
        }else
        {
            uiCurrLen += _pstDataInfo[i].m_uiLen;
            
            iovecRtpSend[iIovecSendIndex].iov_base = _pstDataInfo[i].m_pvAddr;
            iovecRtpSend[iIovecSendIndex].iov_len = _pstDataInfo[i].m_uiLen;
            iIovecSendIndex++;

            if(i == (_iIovNum - 1))
            {
                //mark
                //wirtev end
                Media_Debug("7777777777777\n");
                if(RtpHeaderPackMarkTcp(&rtphdr) < 0)
                {
                    Media_Error("call RtpHeaderPackMarkTcp failed!");
                    iRet = -1;
                    goto end;
                }
                usSeq++;
                if(RtpHeaderPackUpdateSeqTcp(&rtphdr, usSeq) < 0)
                {
                    Media_Error("call RtpHeaderPackUpdateSeqTcp failed!");
                    iRet = -1;
                    goto end;
                }
                Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                if(0 == uiCurrLen)
                {
                    uiCurrLen = RTP_PAYLOAD_LEN;
                }
                uiCurrLen += 12;
                if(RtpHeaderPackSetTcpLen(&rtphdr, (unsigned short)uiCurrLen) < 0)
                {
                    Media_Error("call RtpHeaderPackSetTcpLen failed!");
                    iRet = -1;
                    goto end;
                }
                iRet = SendDataTcp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                iIovecSendIndex = 1;
                uiCurrLen = 0;
            }

            if(RTP_PAYLOAD_LEN == uiCurrLen)
            {
                    Media_Debug("8888888888888\n");
                    usSeq++;
                    if(RtpHeaderPackUpdateSeqTcp(&rtphdr, usSeq) < 0)
                    {
                        Media_Error("call RtpHeaderPackUpdateSeqTcp failed!");
                        iRet = -1;
                        goto end;
                    }
                    Media_Debug("uiCurrLen=%d\n", uiCurrLen);
                    
                    uiCurrLen += 12;
                    if(RtpHeaderPackSetTcpLen(&rtphdr, (unsigned short)uiCurrLen) < 0)
                    {
                        Media_Error("call RtpHeaderPackSetTcpLen failed!");
                        iRet = -1;
                        goto end;
                    }
                    iRet = SendDataTcp(iSocketFd, &iovecRtpSend[0], iIovecSendIndex);
                    iIovecSendIndex = 1;
                    uiCurrLen = 0;
            }
            Media_Debug("999999999\n");
            i++;
        }
    }

    *pusSequencenumber = usSeq;
    
end:
    return iRet;
}


