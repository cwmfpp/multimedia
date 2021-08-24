
#include <stdint.h>
#include <sys/uio.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>

#include "media_mpegenc.h"
#include "media_psmux.h"
#include "vlc_bits.h"
#include "media_log.h"
#include "media_video_h264.h"
#include "media_video_h265.h"
#include "media_svc.h"

#define AUTIO_TIMESTAMP_ABS_FIRST   ~0
#define VIDEO_TIMESTAMP_ABS_FIRST   ~0
#define WHEN_TIMESTAMP_OFFSET   3600

#define IOV_NUM_VIDEO     100

typedef struct _StuffingByte {
    unsigned char m_ucStuffingByteVps[PES_STUFFING_BYTE_VPS_LEN];
    unsigned char m_ucStuffingByteSps[PES_STUFFING_BYTE_SPS_LEN];
    unsigned char m_ucStuffingBytePps[PES_STUFFING_BYTE_PPS_LEN];
    unsigned char m_ucStuffingByteIdrS[PES_STUFFING_BYTE_IDRS_LEN];
    unsigned char m_ucStuffingByteIdrC[PES_STUFFING_BYTE_IDRC_LEN];
    unsigned char m_ucStuffingByteP[PES_STUFFING_BYTE_P_LEN];
    unsigned char m_ucStuffingByteG711A[PES_STUFFING_BYTE_G711A_LEN];
    unsigned char m_ucStuffingByteG711U[PES_STUFFING_BYTE_G711U_LEN];
} StuffingByte;


typedef struct _VideoEsInfo {
    AVCodecID m_eVCodecID;
    unsigned int m_uiTimeScaleV;
    PSDuration m_sampleDurationV;
    unsigned int m_uiBitRateV;
    unsigned int m_uiWidthV;
    unsigned int m_uiHeightV;
    int m_iIsSendSEIV;
} VideoEsInfo;

typedef struct _AudioEsInfo {
    AVCodecID m_eACodecID;
    unsigned int m_uiTimeScaleA;
    PSDuration m_sampleDurationA;
    unsigned int m_uiBitRateA;
} AudioEsInfo;

typedef struct _VideoPackage {
    char *m_pcVideoAddr;
    int m_iVideoLen;
    PSTimestamp m_VideoWhenTimeStampAbsFirst;
    PSTimestamp m_VideoWhenTimeStampAbs;/*�������ps�ļ�ʱ�����Ҳ������Ϊ��һ�ε�ʱ���*/
    PSTimestamp m_VideoWhenTimeStampOffset;/*��¼��һ��ʱ���ͻ��ʱ�����е���ʱ��*/
    int m_iIsSyncFrame;
} VideoPackage;

typedef struct _AudioPackage {
    char *m_pcAudioAddr;
    int m_iAudioLen;
    PSTimestamp m_AudioWhenTimeStampAbsFirst;
    PSTimestamp m_AudioWhenTimeStampAbs;    
    PSTimestamp m_AudioWhenTimeStampOffset;/*��¼��һ��ʱ���ͻ��ʱ�����е���ʱ��*/
} AudioPackage;

typedef struct PsHeaderInfo{
    VideoEsInfo m_stVideoEsInfo;
    AudioEsInfo m_stAudioEsInfo;
    VideoPackage m_stVideoPackage;
    AudioPackage m_stAudioPackage;
    unsigned char m_ucPsHeaderBuf[PS_HEADER_LEN_MAX];
    unsigned char m_ucPsSysHeaderBuf[PS_SYSTEM_HEADER_LEN_MAX];
    unsigned char m_ucPsMapHeaderBuf[PS_MAP_HEADER_LEN_MAX];
    unsigned char m_ucPsPesHeaderArray[PS_PES_PACKET_NUM][PS_PES_HEADER_LEN_MAX];
    int m_iPesIndex;//ÿ�λ�ȡ����ʱ��0
    unsigned int  m_uiStuffValue;//0xba
    DataInfo m_stDIArray[IOV_NUM_VIDEO];
    int m_iDICount;//ÿ�θ�������ʱ��0
    void *m_pvPri;
    
    PSMHeader m_stPSMHeader; 
    StuffingByte m_stStuffingByte;
    int m_iIsEncrypt;
    int m_iIsMkhCCTV;
    int m_iMkhCCTVDone;
	int m_iIsSystemHeader;
}PsHeaderInfo;

static int PsInitStuffingByte(StuffingByte *_pStuffingByte)
{
    int iRet = 0;
    
    uint8_t StuffingByteVps[PES_STUFFING_BYTE_VPS_LEN] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    uint8_t StuffingByteSps[PES_STUFFING_BYTE_SPS_LEN] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    //pes pps
    uint8_t StuffingBytePps[PES_STUFFING_BYTE_PPS_LEN] = {0xff, 0xff, 0xfc};
    uint8_t StuffingByteIdrS[PES_STUFFING_BYTE_IDRS_LEN] = {0xff, 0xff, 0xff, 0xfd};
    uint8_t StuffingByteIdrC[PES_STUFFING_BYTE_IDRC_LEN] = {0xff, 0xff, 0xfa};
    //pes p
    uint8_t StuffingByteP[PES_STUFFING_BYTE_P_LEN] = {0xff, 0xff, 0xff, 0xf8};
    //g711a
    uint8_t StuffingByteG711A[PES_STUFFING_BYTE_G711A_LEN] = {0xff, 0xf8};
    //g711u
    uint8_t StuffingByteG711U[PES_STUFFING_BYTE_G711A_LEN] = {0xff, 0xf8};
    if(NULL == _pStuffingByte)
    {
        iRet = -1;
        goto end;
    }

    (void)memcpy(_pStuffingByte->m_ucStuffingByteVps, StuffingByteVps, PES_STUFFING_BYTE_VPS_LEN);
    (void)memcpy(_pStuffingByte->m_ucStuffingByteSps, StuffingByteSps, PES_STUFFING_BYTE_SPS_LEN);
    (void)memcpy(_pStuffingByte->m_ucStuffingBytePps, StuffingBytePps, PES_STUFFING_BYTE_PPS_LEN);
    (void)memcpy(_pStuffingByte->m_ucStuffingByteIdrS, StuffingByteIdrS, PES_STUFFING_BYTE_IDRS_LEN);
    (void)memcpy(_pStuffingByte->m_ucStuffingByteIdrC, StuffingByteIdrC, PES_STUFFING_BYTE_IDRC_LEN);
    (void)memcpy(_pStuffingByte->m_ucStuffingByteP, StuffingByteP, PES_STUFFING_BYTE_P_LEN);
    (void)memcpy(_pStuffingByte->m_ucStuffingByteG711A, StuffingByteG711A, PES_STUFFING_BYTE_G711A_LEN);
    (void)memcpy(_pStuffingByte->m_ucStuffingByteG711U, StuffingByteG711U, PES_STUFFING_BYTE_G711U_LEN);

end:
    return iRet;
}


static int InitVideoEsInfo(VideoEsInfo *_pstVideoEsInfo, AVCodecID _eAVCodecID, unsigned int _uiTimeScale, PSDuration _sampleDuration, unsigned int _uiBitRate, unsigned int _uiWidth, unsigned int _uiHeight, int _iIsSendSEI)
{
    int iRet = 0;
    
    if(NULL == _pstVideoEsInfo)
    {
        Media_Error("invalid _pstVideoEsInfo(%p)", _pstVideoEsInfo);
        iRet = -1;
        goto end;
    }

    _pstVideoEsInfo->m_eVCodecID = _eAVCodecID;
    _pstVideoEsInfo->m_uiTimeScaleV = _uiTimeScale;
    _pstVideoEsInfo->m_sampleDurationV = _sampleDuration;
    _pstVideoEsInfo->m_uiBitRateV = _uiBitRate;
    _pstVideoEsInfo->m_uiWidthV = _uiWidth;
    _pstVideoEsInfo->m_uiHeightV = _uiHeight;
    _pstVideoEsInfo->m_iIsSendSEIV = _iIsSendSEI;
    
end:
    return iRet;

}

static AVCodecID PsGetVideoCodec(PSHandle *_ph)
{
    AVCodecID eRet = AV_CODEC_ID_NONE;
    PsHeaderInfo *pstPHI = NULL;
    
    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        goto end;
    }
    
    pstPHI = (PsHeaderInfo *)_ph;
    eRet = pstPHI->m_stVideoEsInfo.m_eVCodecID;
    
end:
    return eRet;

}

static int PsIsSendSEI(PSHandle *_ph)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    
    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        goto end;
    }
    
    pstPHI = (PsHeaderInfo *)_ph;
    iRet = pstPHI->m_stVideoEsInfo.m_iIsSendSEIV;
    
end:
    return iRet;

}

/*static int PsIsEncrypt(PSHandle *_ph)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    
    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        goto end;
    }
    
    pstPHI = (PsHeaderInfo *)_ph;
    iRet = pstPHI->m_iIsEncrypt;
    
    Media_Error("m_iIsEncrypt iRet=%d", iRet);
end:
    return iRet;

}*/

static int PsPackHeader(PsHeaderInfo *_pstPHI, int64_t _i64Timestamp, int _iIsSyncFrame)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    
    //ps 
    int iPsHeaderLen = 0;
    
    //sys
    int iPsSysHeaderLen = 0;
    
    //map
    int iPsMapHeaderLen = 0;
    PSMHeader *pstPSMHeader = NULL;


    pstPHI = _pstPHI;
    
    pstPSMHeader = &pstPHI->m_stPSMHeader;

    if(1 == _pstPHI->m_iIsMkhCCTV)
    {
        if(0 == _pstPHI->m_iMkhCCTVDone)
        {
            //write
            static unsigned char ucIMKHArr[4] = {0x49, 0x4D, 0x4B, 0x48};
            _pstPHI->m_iMkhCCTVDone = 1;
            pstPHI->m_stDIArray[pstPHI->m_iDICount].m_pvAddr = ucIMKHArr;
            pstPHI->m_stDIArray[pstPHI->m_iDICount].m_uiLen = (unsigned int)sizeof(ucIMKHArr);
            pstPHI->m_iDICount++;
        }
    }
    //ba            
    iPsHeaderLen = Mpeg_PutPackHeader(pstPHI->m_ucPsHeaderBuf, (uint64_t)_i64Timestamp, pstPHI->m_uiStuffValue++);
    if(iPsHeaderLen < 0)
    {
        Media_Error("call Mpeg_PutPackHeader failed! iPsHeaderLen(%d)", iPsHeaderLen);
        iRet = -1;
        goto end;
    }
    pstPHI->m_stDIArray[pstPHI->m_iDICount].m_pvAddr = pstPHI->m_ucPsHeaderBuf;
    pstPHI->m_stDIArray[pstPHI->m_iDICount].m_uiLen = (unsigned int)iPsHeaderLen;
    pstPHI->m_iDICount++;
    
    if(1 == _iIsSyncFrame)
    {           
        //bb       
        if(1 == _pstPHI->m_iIsSystemHeader)
        {
            iPsSysHeaderLen = Mpeg_PutSystemHeader(pstPHI->m_ucPsSysHeaderBuf);
            pstPHI->m_stDIArray[pstPHI->m_iDICount].m_pvAddr = pstPHI->m_ucPsSysHeaderBuf;
            pstPHI->m_stDIArray[pstPHI->m_iDICount].m_uiLen = (unsigned int)iPsSysHeaderLen;
            pstPHI->m_iDICount++;
        }
		
        //bc           
        iPsMapHeaderLen = Mpeg_PutPsmHeader(pstPSMHeader, pstPHI->m_ucPsMapHeaderBuf);
        pstPHI->m_stDIArray[pstPHI->m_iDICount].m_pvAddr = pstPHI->m_ucPsMapHeaderBuf;
        pstPHI->m_stDIArray[pstPHI->m_iDICount].m_uiLen = (unsigned int)iPsMapHeaderLen;
        pstPHI->m_iDICount++;
    }

end:

    return iRet;
}

static int PsPackPes(PsHeaderInfo *_pstPHI, unsigned char *_ucData, int _iDataLen, uint64_t _ullTimestampAbs, unsigned char _ucStreamId, AVCodecID _eAVCodecID, int _iFrameType)
{
    int iRet = 0;
    uint64_t ullTimestamp = 0;
    int iIovIndex = 0;
    PsHeaderInfo *pstPHI = NULL;
    StuffingByte *pstStuffingByte = NULL;
    
    unsigned char *pucStuffingByteDataS = NULL;
    int iStuffingByteLenS = 0;
    unsigned char *pucStuffingByteDataC = NULL;
    int iStuffingByteLenC = 0;
    int iTimestampFlag = 0;
    
    pstPHI = _pstPHI;        
    //pes
    int iPesIndex = 0;
    
    //pes sps
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
    int iPaylodLen = 0;
    int iPayloadUsed = 0;
    uint8_t *pucPayloadAddr = NULL;
        
    ullTimestamp = _ullTimestampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%ld\n", ullTimestamp);
    pstStuffingByte = &pstPHI->m_stStuffingByte;

    iPaylodLen = _iDataLen;
    pucPayloadAddr = _ucData;
    iPayloadUsed = 0;
    iPesIndex = pstPHI->m_iPesIndex;
    iIovIndex = _pstPHI->m_iDICount;

    if(AV_CODEC_ID_H264 == _eAVCodecID)
    {
        if(NAL_AVCC_SLICE == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else if(NAL_AVCC_SPS == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteSps;
            iStuffingByteLenS = PES_STUFFING_BYTE_SPS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteSps;
            iStuffingByteLenC = PES_STUFFING_BYTE_SPS_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else if(NAL_AVCC_PPS == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingBytePps;
            iStuffingByteLenS = PES_STUFFING_BYTE_PPS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingBytePps;
            iStuffingByteLenC = PES_STUFFING_BYTE_PPS_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }else if(NAL_AVCC_IDR_SLICE == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteIdrS;
            iStuffingByteLenS = PES_STUFFING_BYTE_IDRS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteIdrC;
            iStuffingByteLenC = PES_STUFFING_BYTE_IDRC_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }else if(NAL_AVCC_SEI == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }
    }else if(AV_CODEC_ID_HEVC == _eAVCodecID)
    {
        if(NAL_TRAIL_R == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else if(NAL_VPS == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteVps;
            iStuffingByteLenS = PES_STUFFING_BYTE_VPS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteVps;
            iStuffingByteLenC = PES_STUFFING_BYTE_VPS_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else if(NAL_SPS == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteSps;
            iStuffingByteLenS = PES_STUFFING_BYTE_SPS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteSps;
            iStuffingByteLenC = PES_STUFFING_BYTE_SPS_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }else if(NAL_PPS == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingBytePps;
            iStuffingByteLenS = PES_STUFFING_BYTE_PPS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingBytePps;
            iStuffingByteLenC = PES_STUFFING_BYTE_PPS_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }else if(NAL_IDR_W_RADL == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteIdrS;
            iStuffingByteLenS = PES_STUFFING_BYTE_IDRS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteIdrC;
            iStuffingByteLenC = PES_STUFFING_BYTE_IDRC_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }else if(NAL_SEI_PREFIX == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteIdrS;
            iStuffingByteLenS = PES_STUFFING_BYTE_IDRS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteIdrC;
            iStuffingByteLenC = PES_STUFFING_BYTE_IDRC_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }
    }else if(AV_CODEC_ID_MJPEG == _eAVCodecID)
    {
        //todo
        pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
        iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
        pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
        iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
        iTimestampFlag = PES_PTS_DTS_0B10;
    }else if(AV_CODEC_ID_SVC == _eAVCodecID)
    {
        //todo czd
        if(NAL_SVAC_SLICE == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else if(NAL_SVAC_SPS == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteSps;
            iStuffingByteLenS = PES_STUFFING_BYTE_SPS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteSps;
            iStuffingByteLenC = PES_STUFFING_BYTE_SPS_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else if(NAL_SVAC_PPS == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingBytePps;
            iStuffingByteLenS = PES_STUFFING_BYTE_PPS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingBytePps;
            iStuffingByteLenC = PES_STUFFING_BYTE_PPS_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }else if(NAL_SVAC_IDR_SLICE == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteIdrS;
            iStuffingByteLenS = PES_STUFFING_BYTE_IDRS_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteIdrC;
            iStuffingByteLenC = PES_STUFFING_BYTE_IDRC_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }else if(NAL_SVAC_SEI == _iFrameType)
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else if(NAL_SVAC_SUR == _iFrameType)
        {   //not sure ?
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B10;
        }else
        {
            pucStuffingByteDataS = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenS = PES_STUFFING_BYTE_P_LEN;
            pucStuffingByteDataC = pstStuffingByte->m_ucStuffingByteP;
            iStuffingByteLenC = PES_STUFFING_BYTE_P_LEN;
            iTimestampFlag = PES_PTS_DTS_0B00;
        }
    }
    
    while(iPaylodLen > 0)
    {
       if(iPesIndex >= (PS_PES_PACKET_NUM - 1))
       {
            Media_Debug("iPesIndex(%d) >= (PS_PES_PACKET_NUM - 1)(%d)", iPesIndex, (PS_PES_PACKET_NUM - 1));
            Media_Debug("iIovIndex=%d\n", iIovIndex);
            break;
       }
       
       if(iIovIndex >= (IOV_NUM_VIDEO - 2))
       {
            Media_Debug("iIovIndex(%d) >= (IOV_NUM_VIDEO - 2)(%d)", iIovIndex, (IOV_NUM_VIDEO - 2));
            Media_Debug("iPesIndex=%d\n", iPesIndex);
            break;
       }
       
       if(iPaylodLen == _iDataLen)
       {
           //first
           iPesHeaderLen = Mpeg_PutPesHeader(pstPHI->m_ucPsPesHeaderArray[iPesIndex], _ucStreamId, PES_DATA_ALIGNMENT_INDICATOR_START, (unsigned int)iPaylodLen, iTimestampFlag, ullPts, ullDts, iStuffingByteLenS, pucStuffingByteDataS);
       }else
       {
           //next
           iPesHeaderLen = Mpeg_PutPesHeader(pstPHI->m_ucPsPesHeaderArray[iPesIndex], _ucStreamId, PES_DATA_ALIGNMENT_INDICATOR_CONTINUE, (unsigned int)iPaylodLen, PES_PTS_DTS_0B00, ullPts, ullDts, iStuffingByteLenC, pucStuffingByteDataC);
       }
       
       if(iPaylodLen > (PES_MAX_LEN - (iPesHeaderLen - PES_HEADER_BASE_LEN)))
       {
           iPayloadUsed = PES_MAX_LEN - (iPesHeaderLen - PES_HEADER_BASE_LEN);
           Media_Debug("iPayloadUsed=%d\n", iPayloadUsed);
       }else
       {
           iPayloadUsed = iPaylodLen;
           Media_Debug("iPayloadUsed=%d end\n", iPayloadUsed);
       }
       pstPHI->m_stDIArray[iIovIndex].m_pvAddr = pstPHI->m_ucPsPesHeaderArray[iPesIndex];
       pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)iPesHeaderLen;
       iIovIndex++;
       iPesIndex++;
       
       pstPHI->m_stDIArray[iIovIndex].m_pvAddr = pucPayloadAddr;
       pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)iPayloadUsed;
       iIovIndex++;

       pucPayloadAddr += iPayloadUsed;
       iPaylodLen -= iPayloadUsed;
       iPayloadUsed = 0;
    }

    pstPHI->m_iPesIndex = iPesIndex;
    _pstPHI->m_iDICount = iIovIndex;

    return iRet;
}

static int PsPackVideoSVC(char * _pvData, int _iDataLen, unsigned int _uiTimeStampAbs, PsHeaderInfo *_pstPHI)
{
    int iRet = 0;
    char* pcSVC = NULL;
    int iSVCSize = 0;
    int iPos = 0;
    int iAnalyzeRes = 0;
    NaluInfo stNaluInfo = {0};
    uint64_t ullTimestamp = 0;
    int iIsSendSEI = 0;
    int iIsEncrypt = 0;
    
    PsHeaderInfo *pstPHI = NULL;

    //pes sps
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    pstPHI = _pstPHI;

    ullTimestamp = (uint64_t)_uiTimeStampAbs;
    Media_Debug("ullTimestamp=%lu\n", ullTimestamp);
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    pcSVC = _pvData;
    iSVCSize = _iDataLen;

    iIsSendSEI = PsIsSendSEI(_pstPHI);
    iIsEncrypt = pstPHI->m_iIsEncrypt;

    if(1 == iIsEncrypt)
    {
        Media_Debug("iIsEncrypt=%d\n", iIsEncrypt);
        //e0            
        if(PsPackPes(pstPHI, (unsigned char *)pcSVC, iSVCSize, ullPts, VIDEO_ID, AV_CODEC_ID_H264, NAL_AVCC_SPS) < 0)
        {
            Media_Error("call PsPackPes failed!");
            iRet = -1;
            goto end;
        }
    }else
    {
        while (0 != (iAnalyzeRes = SVAC_AnalyzeNalu((unsigned char *)pcSVC, (unsigned int)iSVCSize, (unsigned int)iPos, &stNaluInfo)))   
        {
            //todo czd
            iPos += iAnalyzeRes;

            stNaluInfo.m_pucVedioData -= 4;
            stNaluInfo.m_iVedioSize += 4;

            if(NAL_SVAC_SLICE == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_SVC, NAL_SVAC_SLICE) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_SVAC_IDR_SLICE == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                //e0            
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_SVC, NAL_SVAC_IDR_SLICE) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_SVAC_SPS == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                //e0            
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_SVC, NAL_SVAC_SPS) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_SVAC_SUR == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                //e0            
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_SVC, NAL_SVAC_SUR) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_SVAC_PPS == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                //e0            
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_SVC, NAL_SVAC_PPS) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_SVAC_SEI == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                if(0 == iIsSendSEI)
                {
                    Media_Debug("not send sei stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                    continue;
                }

                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_SVC, NAL_SVAC_SEI) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }

            }else
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                continue;
            }
        }
    }
end:

    return iRet;
}

static int PsPackVideoH264(char * _pvData, int _iDataLen, unsigned int _uiTimeStampAbs, PsHeaderInfo *_pstPHI)
{
    int iRet = 0;
    char* pcH264 = NULL;
    int iH264Size = 0;
    int iPos = 0;
    int iAnalyzeRes = 0;
    NaluInfo stNaluInfo = {0};
    uint64_t ullTimestamp = 0;
    int iIsSendSEI = 0;
    
    PsHeaderInfo *pstPHI = NULL;
    int iIsEncrypt = 0;

    pstPHI = _pstPHI;
            
    //pes
    
    //pes sps
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
        
    ullTimestamp = (uint64_t)_uiTimeStampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%lu\n", ullTimestamp); 
    pcH264 = _pvData;
    iH264Size = _iDataLen;
    iIsSendSEI = PsIsSendSEI(_pstPHI);
    iIsEncrypt = pstPHI->m_iIsEncrypt;

    if(1 == iIsEncrypt)
    {
        Media_Debug("iIsEncrypt=%d\n", iIsEncrypt);
        //e0            
        if(PsPackPes(pstPHI, (unsigned char *)pcH264, iH264Size, ullPts, VIDEO_ID, AV_CODEC_ID_H264, NAL_AVCC_SPS) < 0)
        {
            Media_Error("call PsPackPes failed!");
            iRet = -1;
            goto end;
        }
    }else
    {
        while (0 != (iAnalyzeRes = H264_AnalyzeNalu((unsigned char *)pcH264, (unsigned int)iH264Size, (unsigned int)iPos, &stNaluInfo)))   
        {        
            iPos += iAnalyzeRes;

            stNaluInfo.m_pucVedioData -= 4;
            stNaluInfo.m_iVedioSize += 4;

            if(NAL_AVCC_SLICE == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_H264, NAL_AVCC_SLICE) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_AVCC_SPS == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                //e0            
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_H264, NAL_AVCC_SPS) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_AVCC_PPS == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_H264, NAL_AVCC_PPS) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_AVCC_IDR_SLICE == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_H264, NAL_AVCC_IDR_SLICE) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }             
            }else if(NAL_AVCC_SEI == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                if(0 == iIsSendSEI)
                {
                    Media_Debug("not send sei stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                    continue;
                }

                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_H264, NAL_AVCC_SEI) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }

            }else
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                continue;
            }
            
        }

    }

end:

    return iRet;
}


static int PsPackVideoH265(char * _pvData, int _iDataLen, unsigned int _uiTimeStampAbs, PsHeaderInfo *_pstPHI)
{
    int iRet = 0;
    char* pcH265 = NULL;
    int iH265Size = 0;
    int iPos = 0;
    int iAnalyzeRes = 0;
    NaluInfo stNaluInfo = {0};
    uint64_t ullTimestamp = 0;
    int iIsSendSEI = 0;
 
    PsHeaderInfo *pstPHI = NULL;
    int iIsEncrypt = 0;

    pstPHI = _pstPHI;
            
    //pes
    
    //pes sps
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
        
    ullTimestamp = (uint64_t)_uiTimeStampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%ld\n", ullTimestamp); 
    pcH265 = _pvData;
    iH265Size = _iDataLen;
    iIsSendSEI = PsIsSendSEI(_pstPHI);
    iIsEncrypt = pstPHI->m_iIsEncrypt;

    if(1 == iIsEncrypt)
    {
        Media_Debug("iIsEncrypt=%d\n", iIsEncrypt);
        //e0            
        if(PsPackPes(pstPHI, (unsigned char *)pcH265, iH265Size, ullPts, VIDEO_ID, AV_CODEC_ID_HEVC, NAL_VPS) < 0)
        {
            Media_Error("call PsPackPes failed!");
            iRet = -1;
            goto end;
        }
    }else
    {
        while (0 != (iAnalyzeRes = H265_AnalyzeNalu((unsigned char *)pcH265, (unsigned int)iH265Size, (unsigned int)iPos, &stNaluInfo)))   
        {        
            iPos += iAnalyzeRes;

            stNaluInfo.m_pucVedioData -= 4;
            stNaluInfo.m_iVedioSize += 4;

            if(NAL_TRAIL_R == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_HEVC, NAL_TRAIL_R) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_VPS == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                
                //e0            
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_HEVC, NAL_VPS) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_SPS == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_HEVC, NAL_SPS) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_PPS == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_HEVC, NAL_PPS) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }
            }else if(NAL_IDR_W_RADL == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_HEVC, NAL_IDR_W_RADL) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }             
            }else if(NAL_SEI_PREFIX == stNaluInfo.m_iType)
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                if(0 == iIsSendSEI)
                {
                    Media_Debug("not send sei stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                    continue;
                }

                //e0       
                if(PsPackPes(pstPHI, stNaluInfo.m_pucVedioData, stNaluInfo.m_iVedioSize, ullPts, VIDEO_ID, AV_CODEC_ID_HEVC, NAL_SEI_PREFIX) < 0)
                {
                    Media_Error("call PsPackPes failed!");
                    iRet = -1;
                    goto end;
                }

            }else
            {
                Media_Debug("stNaluInfo.m_iType=%d\n", stNaluInfo.m_iType);
                continue;
            }
            
        }

    }


end:

    return iRet;
}

static int PsPackVideoMjpeg(char * _pvData, int _iDataLen, unsigned int _uiTimeStampAbs, PsHeaderInfo *_pstPHI)
{
    int iRet = 0;
    char* pcH265 = NULL;
    int iH265Size = 0;
    uint64_t ullTimestamp = 0;
 
    PsHeaderInfo *pstPHI = NULL;
    int iIsEncrypt = 0;

    pstPHI = _pstPHI;
            
    //pes
    
    //pes sps
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
        
    ullTimestamp = (uint64_t)_uiTimeStampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%ld\n", ullTimestamp); 
    pcH265 = _pvData;
    iH265Size = _iDataLen;
    iIsEncrypt = pstPHI->m_iIsEncrypt;

    if(1 == iIsEncrypt)
    {
        Media_Debug("iIsEncrypt=%d\n", iIsEncrypt);
        //e0            
        if(PsPackPes(pstPHI, (unsigned char *)pcH265, iH265Size, ullPts, VIDEO_ID, AV_CODEC_ID_MJPEG, 0/*todo*/) < 0)
        {
            Media_Error("call PsPackPes failed!");
            iRet = -1;
            goto end;
        }
    }else
    {
        //todo
        //e0            
        if(PsPackPes(pstPHI, (unsigned char *)pcH265, iH265Size, ullPts, VIDEO_ID, AV_CODEC_ID_MJPEG, 0/*todo*/) < 0)
        {
            Media_Error("call PsPackPes failed!");
            iRet = -1;
            goto end;
        }
    }


end:

    return iRet;
}

static int PsPackAudioG711A(char *_pAData, int _iADataLen, PSTimestamp _whenTimeStampAbs, PsHeaderInfo *_pstPHI, void *_pPutDataPrivate)
{
    int iRet = 0;
    if(NULL == _pAData)
    {
        Media_Error("invalid _pAData(%p) ", _pAData);
        iRet = -1;
        goto end;
    }

    uint64_t ullTimestamp = 0;

    int iIovIndex = 0;
    PsHeaderInfo *pstPHI = NULL;
    StuffingByte *pstStuffingByte = NULL;

    pstPHI = _pstPHI;
    _pPutDataPrivate = _pPutDataPrivate;
    
    //pes
    int iPesIndex = 0;
    
    //pes sps
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
    ullTimestamp = (uint64_t)_whenTimeStampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%ld\n", ullTimestamp);
    pstStuffingByte = &pstPHI->m_stStuffingByte;

    iPesHeaderLen = Mpeg_PutPesHeader(pstPHI->m_ucPsPesHeaderArray[iPesIndex], AUDIO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, (unsigned int)_iADataLen, PES_PTS_DTS_0B10, ullPts, ullDts, sizeof(pstStuffingByte->m_ucStuffingByteG711A), pstStuffingByte->m_ucStuffingByteG711A);
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = pstPHI->m_ucPsPesHeaderArray[iPesIndex];
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)iPesHeaderLen;
    iIovIndex++;
    iPesIndex++;
    
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = _pAData;
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)_iADataLen;
    iIovIndex++;
    
    _pstPHI->m_iDICount = iIovIndex;
    
end:
    
    return iRet;

}


static int PsPackAudioG711U(char *_pAData, int _iADataLen, PSTimestamp _whenTimeStampAbs, PsHeaderInfo *_pstPHI, void *_pPutDataPrivate)
{
    int iRet = 0;
    if(NULL == _pAData)
    {
        Media_Error("invalid _pAData(%p) ", _pAData);
        iRet = -1;
        goto end;
    }

    uint64_t ullTimestamp = 0;

    int iIovIndex = 0;
    PsHeaderInfo *pstPHI = NULL;
    StuffingByte *pstStuffingByte = NULL;

    pstPHI = _pstPHI;
    _pPutDataPrivate = _pPutDataPrivate;
    
    //pes
    int iPesIndex = 0;
    
    //pes sps
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
    ullTimestamp = (uint64_t)_whenTimeStampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%ld\n", ullTimestamp);
    pstStuffingByte = &pstPHI->m_stStuffingByte;

    iPesHeaderLen = Mpeg_PutPesHeader(pstPHI->m_ucPsPesHeaderArray[iPesIndex], AUDIO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, (unsigned int)_iADataLen, PES_PTS_DTS_0B10, ullPts, ullDts, sizeof(pstStuffingByte->m_ucStuffingByteG711U), pstStuffingByte->m_ucStuffingByteG711U);
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = pstPHI->m_ucPsPesHeaderArray[iPesIndex];
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)iPesHeaderLen;
    iIovIndex++;
    iPesIndex++;
    
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = _pAData;
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)_iADataLen;
    iIovIndex++;
    
    _pstPHI->m_iDICount = iIovIndex;
    
end:
    
    return iRet;

}

static int PsPackAudioAAC(char *_pAData, int _iADataLen, PSTimestamp _whenTimeStampAbs, PsHeaderInfo *_pstPHI, void *_pPutDataPrivate)
{
    int iRet = 0;
    if(NULL == _pAData)
    {
        Media_Error("invalid _pAData(%p) ", _pAData);
        iRet = -1;
        goto end;
    }

    uint64_t ullTimestamp = 0;

    int iIovIndex = 0;
    PsHeaderInfo *pstPHI = NULL;
    StuffingByte *pstStuffingByte = NULL;

    pstPHI = _pstPHI;
    _pPutDataPrivate = _pPutDataPrivate;
    
    //pes
    int iPesIndex = 0;
    
    //pes sps
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
    ullTimestamp = (uint64_t)_whenTimeStampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%ld\n", ullTimestamp);
    pstStuffingByte = &pstPHI->m_stStuffingByte;

    iPesHeaderLen = Mpeg_PutPesHeader(pstPHI->m_ucPsPesHeaderArray[iPesIndex], AUDIO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, (unsigned int)_iADataLen, PES_PTS_DTS_0B10, ullPts, ullDts, sizeof(pstStuffingByte->m_ucStuffingByteG711U), pstStuffingByte->m_ucStuffingByteG711U);
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = pstPHI->m_ucPsPesHeaderArray[iPesIndex];
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)iPesHeaderLen;
    iIovIndex++;
    iPesIndex++;
    
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = _pAData;
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)_iADataLen;
    iIovIndex++;
    
    _pstPHI->m_iDICount = iIovIndex;
    
end:
    
    return iRet;

}
#if 0
static int PsPackAudioSAC(char *_pAData, int _iADataLen, PSTimestamp _whenTimeStampAbs, PsHeaderInfo *_pstPHI, void *_pPutDataPrivate)
{
    //todo czd
    return 0;
}
#endif
static int PsPackAudioAdpcm(char *_pAData, int _iADataLen, PSTimestamp _whenTimeStampAbs, PsHeaderInfo *_pstPHI, void *_pPutDataPrivate)
{
    int iRet = 0;
    if(NULL == _pAData)
    {
        Media_Error("invalid _pAData(%p) ", _pAData);
        iRet = -1;
        goto end;
    }

    uint64_t ullTimestamp = 0;

    int iIovIndex = 0;
    PsHeaderInfo *pstPHI = NULL;
    StuffingByte *pstStuffingByte = NULL;

    pstPHI = _pstPHI;
    _pPutDataPrivate = _pPutDataPrivate;
    
    //pes
    int iPesIndex = 0;
    
    //pes sps
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;

    //pes idr
    ullTimestamp = (uint64_t)_whenTimeStampAbs;
    ullDts = ullTimestamp;
    ullPts = ullDts;
    
    Media_Debug("ullTimestamp=%ld\n", ullTimestamp);
    pstStuffingByte = &pstPHI->m_stStuffingByte;

    iPesHeaderLen = Mpeg_PutPesHeader(pstPHI->m_ucPsPesHeaderArray[iPesIndex], AUDIO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, (unsigned int)_iADataLen, PES_PTS_DTS_0B10, ullPts, ullDts, sizeof(pstStuffingByte->m_ucStuffingByteG711U), pstStuffingByte->m_ucStuffingByteG711U);
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = pstPHI->m_ucPsPesHeaderArray[iPesIndex];
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)iPesHeaderLen;
    iIovIndex++;
    iPesIndex++;
    
    pstPHI->m_stDIArray[iIovIndex].m_pvAddr = _pAData;
    pstPHI->m_stDIArray[iIovIndex].m_uiLen = (unsigned int)_iADataLen;
    iIovIndex++;
    
    _pstPHI->m_iDICount = iIovIndex;
    
end:
    
    return iRet;

}

static int PsmAddEs(PSMHeader *_pstPSMHeader)
{
    int iRet = 0;
    char cPSInfoData[64] = {0};
    
    if(NULL == _pstPSMHeader)
    {
        Media_Error("invalid _pstPSMHeader(%p)", _pstPSMHeader);
        iRet = -1;
        goto end;
    }    
    
    if(Mpeg_InitPsmHeader(_pstPSMHeader) < 0)
    {
        Media_Error("call Mpeg_InitPsmHeader failed!");
        iRet = -1;
        goto end;
    }
    
    if(Mpeg_SetProgramStreamInfo(_pstPSMHeader, cPSInfoData, 0x24) < 0)
    {
        Media_Error("call Mpeg_InitPsmHeader failed!");
        iRet = -1;
        goto end;
    }
    
    Media_Trace("PSM add video ; add audio successful");

end:
    return iRet;
}

static int PsResetVideoPackage(VideoPackage *_pstVideoPackage)
{
    int iRet = 0;

    if(NULL == _pstVideoPackage)
    {
        Media_Error("invalid _pstVideoPackage(%p)", _pstVideoPackage);
        iRet = -1;
        goto end;
    }
    
    _pstVideoPackage->m_pcVideoAddr = NULL;
    _pstVideoPackage->m_iVideoLen = 0;
    _pstVideoPackage->m_VideoWhenTimeStampAbsFirst = VIDEO_TIMESTAMP_ABS_FIRST;
    _pstVideoPackage->m_VideoWhenTimeStampAbs = 0;
    _pstVideoPackage->m_VideoWhenTimeStampOffset = WHEN_TIMESTAMP_OFFSET;
    _pstVideoPackage->m_iIsSyncFrame = 0;
end:
    return iRet;

}

static int PsSetVideoPackage(VideoPackage *_pstVideoPackage, char * _pcVData, int _iDataLen, PSTimestamp _whenTimeStampAbs, int _iIsSyncFrame)
{
    int iRet = 0;

    if(NULL == _pstVideoPackage || NULL == _pcVData || _iDataLen <= 0)
    {
        Media_Error("invalid _pstVideoPackage(%p) _pcVData(%p) _iDataLen(%d)", _pstVideoPackage, _pcVData, _iDataLen);
        iRet = -1;
        goto end;
    }

    _pstVideoPackage->m_pcVideoAddr = _pcVData;
    _pstVideoPackage->m_iVideoLen = _iDataLen;
    if(VIDEO_TIMESTAMP_ABS_FIRST == _pstVideoPackage->m_VideoWhenTimeStampAbsFirst)
    {
        _pstVideoPackage->m_VideoWhenTimeStampAbsFirst = _whenTimeStampAbs;
        Media_Trace("_pstVideoPackage->m_VideoWhenTimeStampAbsFirst=%lld", _pstVideoPackage->m_VideoWhenTimeStampAbsFirst);
    }
    if(_whenTimeStampAbs < _pstVideoPackage->m_VideoWhenTimeStampAbsFirst)
    {
        Media_Error("invalid _whenTimeStampAbs(%lld) < _pstVideoPackage->m_VideoWhenTimeStampAbsFirst(%lld)", _whenTimeStampAbs, _pstVideoPackage->m_VideoWhenTimeStampAbsFirst);

        Media_Error("reset timestamp before m_VideoWhenTimeStampAbsFirst(%lld) m_VideoWhenTimeStampOffset(%lld)", _pstVideoPackage->m_VideoWhenTimeStampAbsFirst, _pstVideoPackage->m_VideoWhenTimeStampOffset);

        _pstVideoPackage->m_VideoWhenTimeStampOffset = _pstVideoPackage->m_VideoWhenTimeStampAbs;
        _pstVideoPackage->m_VideoWhenTimeStampAbsFirst = _whenTimeStampAbs;
        
        Media_Error("reset timestamp after m_VideoWhenTimeStampAbsFirst(%lld) m_VideoWhenTimeStampOffset(%lld)", _pstVideoPackage->m_VideoWhenTimeStampAbsFirst, _pstVideoPackage->m_VideoWhenTimeStampOffset);
    }
    
    _pstVideoPackage->m_VideoWhenTimeStampAbs = (_whenTimeStampAbs - _pstVideoPackage->m_VideoWhenTimeStampAbsFirst) + _pstVideoPackage->m_VideoWhenTimeStampOffset;
        
    //must >>
    if(_pstVideoPackage->m_VideoWhenTimeStampAbs >> 32)
    {
        Media_Error("invalid out _pstVideoPackage->m_VideoWhenTimeStampAbs(0x%llx) for vlc", _pstVideoPackage->m_VideoWhenTimeStampAbs);  
        
        _pstVideoPackage->m_VideoWhenTimeStampOffset = WHEN_TIMESTAMP_OFFSET;
        _pstVideoPackage->m_VideoWhenTimeStampAbsFirst = _whenTimeStampAbs;
        _pstVideoPackage->m_VideoWhenTimeStampAbs = (_whenTimeStampAbs - _pstVideoPackage->m_VideoWhenTimeStampAbsFirst) + _pstVideoPackage->m_VideoWhenTimeStampOffset;
    }
    _pstVideoPackage->m_iIsSyncFrame = _iIsSyncFrame;
end:
    return iRet;
}

static char *PsGetVideoAddr(VideoPackage *_pstVideoPackage)
{
    char *pcAddr = NULL;

    if(NULL == _pstVideoPackage)
    {
        Media_Error("invalid _pstVideoPackage(%p) ", _pstVideoPackage);
        goto end;
    }

    pcAddr = _pstVideoPackage->m_pcVideoAddr;
        
end:
    return pcAddr;

}

static int PsGetVideoLen(VideoPackage *_pstVideoPackage)
{
    int iRet = 0;

    if(NULL == _pstVideoPackage)
    {
        Media_Error("invalid _pstVideoPackage(%p) ", _pstVideoPackage);
        iRet = -1;
        goto end;
    }

    iRet = _pstVideoPackage->m_iVideoLen;
        
end:
    return iRet;
}

static PSTimestamp PsGetVideoTimestamp(VideoPackage *_pstVideoPackage)
{
    PSTimestamp VideoTimestamp = 0;

    if(NULL == _pstVideoPackage)
    {
        Media_Error("invalid _pstVideoPackage(%p) ", _pstVideoPackage);
        VideoTimestamp = (PSTimestamp)-1;
        goto end;
    }

    VideoTimestamp = _pstVideoPackage->m_VideoWhenTimeStampAbs;
        
end:
    return VideoTimestamp;
}

static int PsGetVideoSyncFrame(VideoPackage *_pstVideoPackage)
{
    int iSyncFrame = 0;

    if(NULL == _pstVideoPackage)
    {
        Media_Error("invalid _pstVideoPackage(%p) ", _pstVideoPackage);
        goto end;
    }

    iSyncFrame = _pstVideoPackage->m_iIsSyncFrame;
    
end:
    
    return iSyncFrame;
}

static int PsResetAudioPackage(AudioPackage *_pstAudioPackage)
{
    int iRet = 0;

    if(NULL == _pstAudioPackage)
    {
        Media_Error("invalid _pstAudioPackage(%p)", _pstAudioPackage);
        iRet = -1;
        goto end;
    }

    _pstAudioPackage->m_pcAudioAddr = NULL;
    _pstAudioPackage->m_iAudioLen = 0;
    _pstAudioPackage->m_AudioWhenTimeStampAbsFirst = AUTIO_TIMESTAMP_ABS_FIRST;
    _pstAudioPackage->m_AudioWhenTimeStampAbs = 0;
    _pstAudioPackage->m_AudioWhenTimeStampOffset = WHEN_TIMESTAMP_OFFSET;
    
end:
    return iRet;

}

static int PsSetAudioPackage(AudioPackage *_pstAudioPackage, char * _pcAData, int _iADataLen, PSTimestamp _AwhenTimeStampAbs)
{
    int iRet = 0;

    if(NULL == _pstAudioPackage || NULL == _pcAData || _iADataLen <= 0)
    {
        Media_Error("invalid _pstAudioPackage(%p) _pcAData(%p) _iDataLen(%d)", _pstAudioPackage, _pcAData, _iADataLen);
        iRet = -1;
        goto end;
    }

    _pstAudioPackage->m_pcAudioAddr = _pcAData;
    _pstAudioPackage->m_iAudioLen = _iADataLen;
    if(AUTIO_TIMESTAMP_ABS_FIRST == _pstAudioPackage->m_AudioWhenTimeStampAbsFirst)
    {
        _pstAudioPackage->m_AudioWhenTimeStampAbsFirst = _AwhenTimeStampAbs;
        Media_Trace("_pstAudioPackage->m_AudioWhenTimeStampAbsFirst=%lld", _pstAudioPackage->m_AudioWhenTimeStampAbsFirst);
    }
    if(_AwhenTimeStampAbs < _pstAudioPackage->m_AudioWhenTimeStampAbsFirst)
    {
        Media_Error("invalid _AwhenTimeStampAbs(%lld) < _pstAudioPackage->m_AudioWhenTimeStampAbsFirst(%lld)", _AwhenTimeStampAbs, _pstAudioPackage->m_AudioWhenTimeStampAbsFirst);

        Media_Error("reset timestamp before m_AudioWhenTimeStampAbsFirst(%lld) m_AudioWhenTimeStampOffset(%lld)", _pstAudioPackage->m_AudioWhenTimeStampAbsFirst, _pstAudioPackage->m_AudioWhenTimeStampOffset);

        _pstAudioPackage->m_AudioWhenTimeStampOffset = _pstAudioPackage->m_AudioWhenTimeStampAbs;
        _pstAudioPackage->m_AudioWhenTimeStampAbsFirst = _AwhenTimeStampAbs;

        Media_Error("reset timestamp after m_AudioWhenTimeStampAbsFirst(%lld) m_AudioWhenTimeStampOffset(%lld)", _pstAudioPackage->m_AudioWhenTimeStampAbsFirst, _pstAudioPackage->m_AudioWhenTimeStampOffset);
    }
    
    _pstAudioPackage->m_AudioWhenTimeStampAbs = (_AwhenTimeStampAbs - _pstAudioPackage->m_AudioWhenTimeStampAbsFirst) + _pstAudioPackage->m_AudioWhenTimeStampOffset;
    
    //must >>
    if(_pstAudioPackage->m_AudioWhenTimeStampAbs >> 32)
    {
        Media_Error("invalid out _pstVideoPackage->m_AudioWhenTimeStampAbs(0x%llx) for vlc", _pstAudioPackage->m_AudioWhenTimeStampAbs);  
        
        _pstAudioPackage->m_AudioWhenTimeStampOffset = WHEN_TIMESTAMP_OFFSET;
        _pstAudioPackage->m_AudioWhenTimeStampAbsFirst = _AwhenTimeStampAbs;
        _pstAudioPackage->m_AudioWhenTimeStampAbs = (_AwhenTimeStampAbs - _pstAudioPackage->m_AudioWhenTimeStampAbsFirst) + _pstAudioPackage->m_AudioWhenTimeStampOffset;
    }
    
    
end:
    return iRet;

}

PSHandle* API_PUBLIC PS_Create(void)
{
    PsHeaderInfo *pstPsHeaderInfo = NULL;

    pstPsHeaderInfo = (PsHeaderInfo *)malloc(sizeof(PsHeaderInfo));

    if(NULL == pstPsHeaderInfo)
    {
        Media_Error("call malloc failed");
        goto end;
    }
    pstPsHeaderInfo->m_uiStuffValue = 0xffffffff;

    if(PsmAddEs(&pstPsHeaderInfo->m_stPSMHeader) < 0)
    {
        Media_Error("call PsmAddEs failed");
        goto end;
    }

    if(PsInitStuffingByte(&pstPsHeaderInfo->m_stStuffingByte))
    {
        Media_Error("call PsInitStuffingByte failed");
        goto end;
    }

    pstPsHeaderInfo->m_stVideoEsInfo.m_eVCodecID = AV_CODEC_ID_NONE;
    pstPsHeaderInfo->m_stAudioEsInfo.m_eACodecID = AV_CODEC_ID_NONE;
    
    if(PsResetVideoPackage(&pstPsHeaderInfo->m_stVideoPackage) < 0)
    {
        Media_Error("call PsResetVideoPackage failed!");
        goto end;
    }
    
    if(PsResetAudioPackage(&pstPsHeaderInfo->m_stAudioPackage) < 0)
    {
        Media_Error("call PsResetAudioPackage failed!");
        goto end;
    }

    pstPsHeaderInfo->m_iIsEncrypt = 0;
    pstPsHeaderInfo->m_iIsMkhCCTV = 0;
    pstPsHeaderInfo->m_iMkhCCTVDone = 0;
    pstPsHeaderInfo->m_iIsSystemHeader = 0;
    Media_Trace("Program stream create successful");

end:
    return pstPsHeaderInfo;
}

static int SetPsInsertHead(PsInsertHead *_pstPsInsertHead, PsPrivateAttribute *_pstPsPriAttr)
{
    int iRet = 0;
    //unsigned char ptr[8] = {0};
    
    //unsigned char *ptr = PRIVATE_FLAG;
    
    if(NULL == _pstPsInsertHead || NULL == _pstPsPriAttr)
    {
        Media_Error("invalid _pstPsInsertHead(%p) _pstPsPriAttr(%p)", _pstPsInsertHead, _pstPsPriAttr);
        iRet = -1;
        goto end;
    }
    
    //strncpy((char *)ptr, PRIVATE_FLAG, sizeof(ptr) - 1);
    _pstPsInsertHead->m_uiFlag = PRIVATE_FLAG;//MKTAG(*ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3));
    _pstPsInsertHead->m_uiSize = sizeof(PsInsertHead);
    _pstPsInsertHead->m_usWidth = _pstPsPriAttr->m_usWidth;
    _pstPsInsertHead->m_usHeight = _pstPsPriAttr->m_usHeight;
    _pstPsInsertHead->m_uiSampleRate = _pstPsPriAttr->m_uiSampleRate;
    _pstPsInsertHead->m_ucIsEncrypt = _pstPsPriAttr->m_ucIsEncrypt;
    _pstPsInsertHead->m_ucChannel = _pstPsPriAttr->m_ucChannel;
    _pstPsInsertHead->m_usBits = _pstPsPriAttr->m_usBits;

end:
    return iRet;
}

int API_PUBLIC PS_SetStreamPrivateAttribute(PSHandle *_ph, PsPrivateAttribute *_pstPsPriAttr)
{
    int iRet = 0;
    int iPsInfoLen = 0;    
    char cPSInfo[PROGRAM_STREAM_INFO_LEN] = {0};
    PsHeaderInfo *pstPHI = NULL;
    PSMHeader *pPSMHeader = NULL;
    PsInsertHead stPsInsertHead = {0};
    unsigned char cPSInfoHk[] = {0x40, 0x0E, 0x48, 0x4B, 0x01, 0x00, 0x11, 0xB3, 
                                    0x43, 0x9A, 0x3A, 0x40, 0x00, 0xFF, 0xFF, 0xFF, 
                                    0x41, 0x12, 0x48, 0x4B, 0x00, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00};
    
    if(NULL == _ph || NULL == _pstPsPriAttr)
    {
        Media_Error("invalid _ph(%p) _pstPsPriAttr(%p)", _ph, _pstPsPriAttr);
        iRet = -1;
        goto end;
    }
    
    pstPHI = (PsHeaderInfo *)_ph;
    pPSMHeader = &pstPHI->m_stPSMHeader;
    
    if(SetPsInsertHead(&stPsInsertHead, _pstPsPriAttr) < 0)
    {
        Media_Error("call SetPsInsertHead failed!");
        iRet = -1;
        goto end;
    }

    #if 0
    if(PROGRAM_STREAM_INFO_LEN < iPsInfoLen)
    {
        iRet = -1;
        goto end;
    }
    
    if((PROGRAM_STREAM_INFO_LEN - iPsInfoLen) < (int)sizeof(cPSInfoHk))
    {
        Media_Error("invalid (PROGRAM_STREAM_INFO_LEN - iPsInfoLen)(%d) < sizeof(cPSInfoHk)(%d)", PROGRAM_STREAM_INFO_LEN - iPsInfoLen, sizeof(cPSInfoHk));
        iRet = -1;
        goto end;
    }
    #endif
    
    memcpy(cPSInfo, cPSInfoHk, sizeof(cPSInfoHk));
    iPsInfoLen = (int)sizeof(cPSInfoHk);

    #if 0
    if((PROGRAM_STREAM_INFO_LEN - iPsInfoLen) < (int)sizeof(PsInsertHead))
    {
        Media_Error("invalid (PROGRAM_STREAM_INFO_LEN - iPsInfoLen)(%d) < sizeof(PsInsertHead)(%d)", PROGRAM_STREAM_INFO_LEN - iPsInfoLen, sizeof(PsInsertHead));
        iRet = -1;
        goto end;
    }
    #endif
    memcpy(cPSInfo + iPsInfoLen, &stPsInsertHead, sizeof(PsInsertHead));
    iPsInfoLen += (int)sizeof(PsInsertHead);
    iPsInfoLen = iPsInfoLen;
    
    if(Mpeg_SetProgramStreamInfo(pPSMHeader, cPSInfo, (unsigned int)iPsInfoLen) < 0)
    {
        Media_Error("call Mpeg_InitPsmHeader failed!");
        iRet = -1;
        goto end;
    }

    pstPHI->m_iIsEncrypt = _pstPsPriAttr->m_ucIsEncrypt;
    pstPHI->m_iIsMkhCCTV = _pstPsPriAttr->m_iIsMkhCCTV;
    pstPHI->m_iIsSystemHeader = _pstPsPriAttr->m_iIsSystemHeader;
    Media_Debug("pstPHI->m_iIsEncrypt=%d", pstPHI->m_iIsEncrypt);
    
end:
    return iRet;

}


int API_PUBLIC PS_SetInsertHead(PSHandle *_ph, PsInsertHead *_pstPsInsertHead)
{
    int iRet = 0;
    int iPsInfoLen = 0;    
    char cPSInfo[PROGRAM_STREAM_INFO_LEN] = {0};
    PsHeaderInfo *pstPHI = NULL;
    PSMHeader *pPSMHeader = NULL;
    unsigned char cPSInfoHk[] = {0x40, 0x0E, 0x48, 0x4B, 0x01, 0x00, 0x11, 0xB3, 
                                    0x43, 0x9A, 0x3A, 0x40, 0x00, 0xFF, 0xFF, 0xFF, 
                                    0x41, 0x12, 0x48, 0x4B, 0x00, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00};
    
    if(NULL == _ph || NULL == _pstPsInsertHead)
    {
        Media_Error("invalid _ph(%p) _pstPsInsertHead(%p)", _ph, _pstPsInsertHead);
        iRet = -1;
        goto end;
    }

    if(_pstPsInsertHead->m_uiSize <= 0)
    {
        Media_Error("invalid _pstPsInsertHead->m_uiSize(%d)", _pstPsInsertHead->m_uiSize);
        iRet = -1;
        goto end;
    }
    
    pstPHI = (PsHeaderInfo *)_ph;
    pPSMHeader = &pstPHI->m_stPSMHeader;
    if((sizeof(cPSInfoHk) + _pstPsInsertHead->m_uiSize) >= sizeof(cPSInfo))
    {
        Media_Error("invalid sizeof(cPSInfoHk)(%ld) + m_uiSize(%d) > sizeof(cPSInfo)(%ld)", sizeof(cPSInfoHk), _pstPsInsertHead->m_uiSize, sizeof(cPSInfo));
        iRet = -1;
        goto end;
    }
    
    memcpy(cPSInfo, cPSInfoHk, sizeof(cPSInfoHk));
    iPsInfoLen = (int)sizeof(cPSInfoHk);

    memcpy(cPSInfo + iPsInfoLen, _pstPsInsertHead, _pstPsInsertHead->m_uiSize);
    iPsInfoLen += (int)(_pstPsInsertHead->m_uiSize);
    iPsInfoLen = iPsInfoLen;
    
    if(Mpeg_SetProgramStreamInfo(pPSMHeader, cPSInfo, (unsigned int)iPsInfoLen) < 0)
    {
        Media_Error("call Mpeg_InitPsmHeader failed!");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;

}

int API_PUBLIC PS_SetVideoEs(PSHandle *_ph, VideoEs *_pstVideoEs)
{
    int iRet = 0;
    
    PsHeaderInfo *pstPHI = NULL;
    unsigned char cVideoESInfoData[28] = {0x77, 0x65, 0x6e, 0x6d, 0x69, 0x6e, 0x63, 0x68, 
                                        0x65, 0x6e, 0x40, 0x31, 0x32, 0x36, 0x2e, 0x63, 
                                        0x6f, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00};
    AVCodecID _eAVCodecID;
    unsigned int _uiFrameRate;
    unsigned int _uiBitRate;
    unsigned int _uiWidth;
    unsigned int _uiHeight;
    int iIsSendSEI = 0;
    
    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        iRet = -1;
        goto end;
    }
    
    _eAVCodecID = _pstVideoEs->m_eVCodecID;
    _uiFrameRate = _pstVideoEs->m_uiFrameRate;
    _uiBitRate = _pstVideoEs->m_uiBitRate;
    _uiWidth = _pstVideoEs->m_uiWidth;
    _uiHeight = _pstVideoEs->m_uiHeight;
    iIsSendSEI = _pstVideoEs->m_iIsSendSEI;
    
    if(AV_CODEC_ID_H264 != _eAVCodecID && AV_CODEC_ID_HEVC != _eAVCodecID && AV_CODEC_ID_MJPEG != _eAVCodecID && AV_CODEC_ID_SVC != _eAVCodecID/*&& todo*/)
    {
        Media_Error("invalid _eAVCodecID(%d)", _eAVCodecID);
        iRet = -1;
        goto end;
    }
    
    if(NULL == _pstVideoEs)
    {
        Media_Error("invalid _pstVideoEs(%p)", _pstVideoEs);
        iRet = -1;
        goto end;
    }

            
    if(0 == _uiFrameRate)
    {
        Media_Error("invalid _uiFrameRate(%d)", _uiFrameRate);
        iRet = -1;
        goto end;
    }
    
    if(0 == _uiBitRate)
    {
        Media_Error("invalid _uiBitRate(%d)", _uiBitRate);
        iRet = -1;
        goto end;
    }
    
    if(0 == _uiWidth)
    {
        Media_Error("invalid _uiWidth(%d)", _uiWidth);
        iRet = -1;
        goto end;
    }
    
    if(0 == _uiHeight)
    {
        Media_Error("invalid _uiHeight(%d)", _uiHeight);
        iRet = -1;
        goto end;
    }

    if(0 != iIsSendSEI && 1 != iIsSendSEI)
    {
        Media_Error("invalid iIsSendSEI(%d), must 0 or 1", iIsSendSEI);
        iRet = -1;
        goto end;
    }
    
    pstPHI = (PsHeaderInfo *)_ph;
    
    if(InitVideoEsInfo(&pstPHI->m_stVideoEsInfo, _eAVCodecID, 90000 / _uiFrameRate, (PSDuration)(90000 / _uiFrameRate), _uiBitRate, _uiWidth, _uiHeight, iIsSendSEI) < 0)
    {
        Media_Error("call InitVideoEsInfo failed!");
        iRet = -1;
        goto end;
    }

    if(AV_CODEC_ID_H264 == _eAVCodecID)
    {
        Media_Trace("AV_CODEC_ID_H264(%d)", _eAVCodecID);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 0, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_H264, (char *)cVideoESInfoData, sizeof(cVideoESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_HEVC == _eAVCodecID)
    {
        Media_Trace("AV_CODEC_ID_HEVC(%d)", _eAVCodecID);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 0, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_HEVC, (char *)cVideoESInfoData, sizeof(cVideoESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_MJPEG == _eAVCodecID)
    {
        Media_Trace("AV_CODEC_ID_MJPEG(%d)", _eAVCodecID);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 0, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_MJPEG, (char *)cVideoESInfoData, sizeof(cVideoESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_SVC == _eAVCodecID)
    {
        Media_Trace("AV_CODEC_ID_SVC(%d)", _eAVCodecID);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 0, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_SVC, (char *)cVideoESInfoData, sizeof(cVideoESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }
    
end:
    return iRet;

}

static int InitAudioEsInfo(AudioEsInfo *_pstAudioEsInfo, AVCodecID _ePsACodec, unsigned int _uiTimeScale, PSDuration _sampleDuration, unsigned int _uiBitRate)
{
    int iRet = 0;
    
    if(NULL == _pstAudioEsInfo)
    {
        Media_Error("invalid _pstAudioEsInfo(%p)", _pstAudioEsInfo);
        iRet = -1;
        goto end;
    }

    _pstAudioEsInfo->m_eACodecID = _ePsACodec;
    _pstAudioEsInfo->m_uiTimeScaleA = _uiTimeScale;
    _pstAudioEsInfo->m_sampleDurationA = _sampleDuration;
    _pstAudioEsInfo->m_uiBitRateA = _uiBitRate;
    
end:
    return iRet;

}

static AVCodecID PsGetAudioCodec(PSHandle *_ph)
{
    AVCodecID eRet = AV_CODEC_ID_NONE;
    PsHeaderInfo *pstPHI = NULL;
    
    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        goto end;
    }
    
    pstPHI = (PsHeaderInfo *)_ph;
    eRet = pstPHI->m_stAudioEsInfo.m_eACodecID;
    Media_Debug("pstPHI->m_stAudioEsInfo.m_eACodecID(%d)", pstPHI->m_stAudioEsInfo.m_eACodecID);
    
end:
    return eRet;

}

static int ResetFrameOutInfo(FrameOutInfo *_pstFrameOutInfo)
{
    int iRet = 0;
    if(NULL == _pstFrameOutInfo)
    {
        Media_Error("call fopen failed");
        goto end;
    }

    _pstFrameOutInfo->m_pstDI = NULL;
    _pstFrameOutInfo->m_iDICount = 0;
    _pstFrameOutInfo->m_isEnd = 0;
    
end:
    return iRet;
}

int API_PUBLIC PS_SetAudioEs(PSHandle *_ph, AudioEs *_pstAudioEs)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    char cAudioESInfoData[12] = {0};
    AVCodecID _ePsACodec;
    unsigned int _uiSampleRate;
    unsigned int _uiBitRate;
    
    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        iRet = -1;
        goto end;
    }
    
    if(NULL == _pstAudioEs)
    {
        Media_Error("invalid _pstAudioEs(%p)", _pstAudioEs);
        iRet = -1;
        goto end;
    }

    _ePsACodec = _pstAudioEs->m_eACodecID;
    _uiSampleRate = _pstAudioEs->m_uiSampleRate;
    _uiBitRate = _pstAudioEs->m_uiBitRate;
    
    if(AV_CODEC_ID_PCM_ALAW/*G7111A*/ != _ePsACodec && AV_CODEC_ID_SAC != _ePsACodec  && AV_CODEC_ID_PCM_MULAW/*G711U*/ != _ePsACodec && AV_CODEC_ID_AAC != _ePsACodec && AV_CODEC_ID_ADPCM_IMA_APC != _ePsACodec/*&& todo*/)
    {
        Media_Error("invalid _ePsACodec(%d)", _ePsACodec);
        iRet = -1;
        goto end;
    }
    
    if(0 == _uiSampleRate)
    {
        Media_Error("invalid _uiSampleRate(%d)", _uiSampleRate);
        iRet = -1;
        goto end;
    }
        
    if(0 == _uiBitRate)
    {
        Media_Error("invalid _uiBitRate(%d)", _uiBitRate);
        iRet = -1;
        goto end;
    }

    pstPHI = (PsHeaderInfo *)_ph;

    if(InitAudioEsInfo(&pstPHI->m_stAudioEsInfo, _ePsACodec, _uiSampleRate, (PSDuration)_uiSampleRate, _uiBitRate) < 0)
    {
        Media_Error("call InitAudioEsInfo failed!");
        iRet = -1;
        goto end;
    }

    if(AV_CODEC_ID_PCM_ALAW == _ePsACodec)
    {
        Media_Trace("AV_CODEC_ID_PCM_ALAW(%d)", _ePsACodec);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 1, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_PCM_ALAW, cAudioESInfoData, sizeof(cAudioESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_PCM_MULAW == _ePsACodec)
    {
        Media_Trace("AV_CODEC_ID_PCM_MULAW(%d)", _ePsACodec);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 1, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_PCM_MULAW, cAudioESInfoData, sizeof(cAudioESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_AAC == _ePsACodec)
    {
        Media_Trace("AV_CODEC_ID_AAC(%d)", _ePsACodec);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 1, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_AAC, cAudioESInfoData, sizeof(cAudioESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_ADPCM_IMA_APC == _ePsACodec)
    {
        Media_Trace("AV_CODEC_ID_ADPCM_IMA_APC(%d)", _ePsACodec);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 1, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_ADPCM_IMA_APC, cAudioESInfoData, sizeof(cAudioESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_SAC == _ePsACodec)
    {
        Media_Trace("AV_CODEC_ID_SAC(%d)", _ePsACodec);
        if(Mpeg_SetElementaryStream(&pstPHI->m_stPSMHeader, 1, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_SAC, cAudioESInfoData, sizeof(cAudioESInfoData)) < 0)
        {
            Media_Error("call Mpeg_InitPsmHeader failed!");
            iRet = -1;
            goto end;
        }
    }

    
end:
    return iRet;

}

int API_PUBLIC PS_PutVideoData(PSHandle *_ph, char * _pcVData, int _iDataLen, PsFrameInfo *_pstPsFrameInfo)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    
    if(NULL == _ph || NULL == _pcVData || _iDataLen <= 0 || NULL == _pstPsFrameInfo)
    {
        Media_Error("invalid _ph(%p) _pcVData(%p) _iDataLen(%d) _pstPsFrameInfo(%p)", _ph, _pcVData, _iDataLen, _pstPsFrameInfo);
        iRet = -1;
        goto end;
    }
    pstPHI = (PsHeaderInfo *)_ph;

    if(PsSetVideoPackage(&pstPHI->m_stVideoPackage, _pcVData, _iDataLen, _pstPsFrameInfo->m_TimestampMs * 90LL, _pstPsFrameInfo->m_isSyncFrame) < 0)
    {
        Media_Error("call PsSetVideoPackage failed _ph(%p) _pcVData(%p) _iDataLen(%d)", _ph, _pcVData, _iDataLen);
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;

}

int API_PUBLIC PS_GetVideoPackage(PSHandle *_ph, FrameOutInfo *_pstInfo, void *_pvPri)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    AVCodecID eVCodecID = AV_CODEC_ID_NONE;
    char * pcVData = NULL;
    int iVDataLen = 0;
    PSTimestamp whenTimeStampAbs = 0;
    int iIsSyncFrame = 0;
    
    if(NULL == _ph || NULL == _pstInfo)
    {
        Media_Error("invalid _ph(%p) _pstInfo(%p)", _ph, _pstInfo);
        iRet = -1;
        goto end;
    }

    pstPHI = (PsHeaderInfo *)_ph;
    _pvPri = _pvPri;
    
    if(ResetFrameOutInfo(_pstInfo) < 0)
    {
        Media_Error("call ResetFrameOutInfo failed!");
        iRet = -1;
        goto end;
    }
    
    eVCodecID = PsGetVideoCodec(_ph);
    pcVData = PsGetVideoAddr(&pstPHI->m_stVideoPackage);
    if(NULL == pcVData)
    {
        Media_Error("call PsGetVideoAddr failed pcVData(%p)", pcVData);
        iRet = -1;
        goto end;
    }
    
    iVDataLen = PsGetVideoLen(&pstPHI->m_stVideoPackage);
    if(iVDataLen <= 0)
    {
        Media_Error("call PsGetVideoLen failed iVDataLen(%d)", iVDataLen);
        iRet = -1;
        goto end;
    }

    whenTimeStampAbs = PsGetVideoTimestamp(&pstPHI->m_stVideoPackage);
    
    if((PSTimestamp)-1 == whenTimeStampAbs)
    {
        Media_Error("call PsGetVideoTimestamp failed iVDataLen(%d)", iVDataLen);
        iRet = -1;
        goto end;
    }

    iIsSyncFrame = PsGetVideoSyncFrame(&pstPHI->m_stVideoPackage);
    
    pstPHI->m_iPesIndex = 0;
    pstPHI->m_iDICount = 0;


    if(PsPackHeader(pstPHI, (int64_t)whenTimeStampAbs, iIsSyncFrame) < 0)
    {
        Media_Error("call PsPackHeader failed iIsSyncFrame(%d)", iIsSyncFrame);
        iRet = -1;
        goto end;
    }
    
    if(AV_CODEC_ID_H264 == eVCodecID)
    {
        if(PsPackVideoH264(pcVData, iVDataLen, (unsigned int)whenTimeStampAbs, pstPHI) < 0)
        {
            Media_Error("call PsPackVideoH264 failed");        
        }
    }else if(AV_CODEC_ID_HEVC == eVCodecID)
    {
        if(PsPackVideoH265(pcVData, iVDataLen, (unsigned int)whenTimeStampAbs, pstPHI) < 0)
        {
            Media_Error("call PsPackVideoH265 failed");        
        }
    }else if(AV_CODEC_ID_MJPEG == eVCodecID)
    {
        if(PsPackVideoMjpeg(pcVData, iVDataLen, (unsigned int)whenTimeStampAbs, pstPHI) < 0)
        {
            Media_Error("call PsPackVideoMjpeg failed");        
        }
    }
    else if(AV_CODEC_ID_SVC == eVCodecID)
    {
        if(PsPackVideoSVC(pcVData, iVDataLen, (unsigned int)whenTimeStampAbs, pstPHI) < 0)
        {
            Media_Error("call PsPackVideoSVC failed");        
        }
    }else
    {
        Media_Error("set video codec error or no set eVCodecID=%d", eVCodecID);
        iRet = -1;
        goto end;

    }

    _pstInfo->m_pstDI = pstPHI->m_stDIArray;
    _pstInfo->m_iDICount = pstPHI->m_iDICount;
    _pstInfo->m_isEnd = 1;

end:
    return iRet;
}



int API_PUBLIC PS_PutAudioData(PSHandle *_ph, char * _pcAData, int _iADataLen, PsFrameInfo *_pstPsFrameInfo)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    
    if(NULL == _ph || NULL == _pcAData || _iADataLen <= 0 || NULL == _pstPsFrameInfo)
    {
        Media_Error("invalid _ph(%p) _pcAData(%p) _iADataLen(%d) _pstPsFrameInfo(%p)", _ph, _pcAData, _iADataLen, _pstPsFrameInfo);
        iRet = -1;
        goto end;
    }
    pstPHI = (PsHeaderInfo *)_ph;

    if(PsSetAudioPackage(&pstPHI->m_stAudioPackage, _pcAData, _iADataLen, _pstPsFrameInfo->m_TimestampMs * 90LL) < 0)
    {
        Media_Error("call PsSetAudioPackage failed _ph(%p) _pcAData(%p) _iADataLen(%d)", _ph, _pcAData, _iADataLen);
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;

}


int API_PUBLIC PS_GetAudioPackage(PSHandle *_ph, FrameOutInfo *_pstInfo, void *_pvPri)
{
    int iRet = 0;
    PsHeaderInfo *pstPHI = NULL;
    AVCodecID ePsACodec = AV_CODEC_ID_NONE;

    if(NULL == _ph || NULL == _pstInfo)
    {
        Media_Error("invalid _ph(%p) _pstInfo(%p)", _ph, _pstInfo);
        iRet = -1;
        goto end;
    }

    _ph = _ph;
    _pstInfo = _pstInfo;
    _pvPri = _pvPri;
    
    pstPHI = (PsHeaderInfo *)_ph;    

    if(ResetFrameOutInfo(_pstInfo) < 0)
    {
        Media_Error("call ResetFrameOutInfo failed!");
        iRet = -1;
        goto end;
    }

    ePsACodec = PsGetAudioCodec(_ph);
    if(AV_CODEC_ID_PCM_ALAW == ePsACodec)
    {
        if(PsPackAudioG711A(pstPHI->m_stAudioPackage.m_pcAudioAddr, pstPHI->m_stAudioPackage.m_iAudioLen, pstPHI->m_stAudioPackage.m_AudioWhenTimeStampAbs, pstPHI, _pvPri) < 0)
        {
            Media_Error("call PsPackAudioG711A failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_PCM_MULAW == ePsACodec)
    {
        if(PsPackAudioG711U(pstPHI->m_stAudioPackage.m_pcAudioAddr, pstPHI->m_stAudioPackage.m_iAudioLen, pstPHI->m_stAudioPackage.m_AudioWhenTimeStampAbs, pstPHI, _pvPri) < 0)
        {
            Media_Error("call PsPackAudioG711U failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_AAC == ePsACodec)
    {
        if(PsPackAudioAAC(pstPHI->m_stAudioPackage.m_pcAudioAddr, pstPHI->m_stAudioPackage.m_iAudioLen, pstPHI->m_stAudioPackage.m_AudioWhenTimeStampAbs, pstPHI, _pvPri) < 0)
        {
            Media_Error("call PsPackAudioAAC failed!");
            iRet = -1;
            goto end;
        }
    }else if(AV_CODEC_ID_ADPCM_IMA_APC == ePsACodec)
    {
        if(PsPackAudioAdpcm(pstPHI->m_stAudioPackage.m_pcAudioAddr, pstPHI->m_stAudioPackage.m_iAudioLen, pstPHI->m_stAudioPackage.m_AudioWhenTimeStampAbs, pstPHI, _pvPri) < 0)
        {
            Media_Error("call PsPackAudioAdpcm failed!");
            iRet = -1;
            goto end;
        }
    }
    #if 0
    else if(AV_CODEC_ID_SAC == ePsACodec)
    {
        if(PsPackAudioSAC(pstPHI->m_stAudioPackage.m_pcAudioAddr, pstPHI->m_stAudioPackage.m_iAudioLen, pstPHI->m_stAudioPackage.m_AudioWhenTimeStampAbs, pstPHI, _pvPri) < 0)
        {
            Media_Error("call PsPackAudioSAC failed!");
            iRet = -1;
            goto end;
        }
    }
    #endif
    else
    {
        Media_Error("set audio codec error or no set ePsACodec=%d", ePsACodec);
        iRet = -1;
        goto end;

    }
    
    _pstInfo->m_pstDI = pstPHI->m_stDIArray;
    _pstInfo->m_iDICount = pstPHI->m_iDICount;
    _pstInfo->m_isEnd = 1;
    
end:
    return iRet;
}

int API_PUBLIC PS_SetPrivateData(PSHandle *_ph, void *_pvPri)
{
    int iRet = 0;
    PsHeaderInfo *pstPsHeaderInfo = NULL;

    if(NULL == _ph || NULL == _pvPri)
    {
        Media_Error("invalid _ph(%p) _pvPri(%p)", _ph, _pvPri);
        iRet = -1;
        goto end;
    }
    
    pstPsHeaderInfo = (PsHeaderInfo *)_ph;

    pstPsHeaderInfo->m_pvPri = _pvPri;

end:
    return iRet;
}

void* API_PUBLIC PS_GetPrivateData(PSHandle *_ph)
{
    void *pPri = NULL;
    PsHeaderInfo *pstPsHeaderInfo = NULL;

    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        goto end;
    }

    pstPsHeaderInfo = (PsHeaderInfo *)_ph;
    pPri = pstPsHeaderInfo->m_pvPri;

end:
    return pPri;
}

int API_PUBLIC PS_Destroy(PSHandle *_ph)
{
    int iRet = 0;

    if(NULL == _ph)
    {
        Media_Error("invalid _ph(%p)", _ph);
        iRet = -1;
        goto end;
    }
    
    _ph = _ph;

    free(_ph);
    _ph = NULL;

end:
    return iRet;
}

