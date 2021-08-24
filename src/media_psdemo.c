
#if 0
#ifndef PS_SDK
#include <stdio.h>
#include <stdint.h>
#include <sys/uio.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "media_psdemo.h"
#include "vlc_bits.h"
#include "media_log.h"
#include "media_video_h264.h"
#include "media_video_h265.h"
#include "media_mpegenc.h"
#include "media_psmux.h"
#include "public_def.h"
#include "media_psfile.h"

#if 0

static void PrintHexData(unsigned char *_pData, int _pDataLen, int _displayCount)
{
    (void)printf("start _pData _pDataLen=%d", _pDataLen);
    int i = 0;

    (void)printf("\n");
    for(i = 0; i < _pDataLen; i++)
    {
        if(_pData[i] < 0x10) {
            (void)printf("0x0%x, ", _pData[i]);
        } else {
            (void)printf("0x%x, ", _pData[i]);
        }
        if((!((i + 1) %_displayCount)) && (i > 0))
        {
            (void)printf("\n");
        }
    }
    (void)printf("\n");
    (void)printf("\n");
    (void)printf("end _pData\n");

    return;
}

static void ShowFrameHeader(S_FrameHeader *_pstHeader)
{
    Media_Debug("u32FrameID=0x%x", _pstHeader->u32FrameID);
    Media_Debug("u32FrameSize=%d", _pstHeader->u32FrameSize);
    Media_Debug("u8FrameType=%d", _pstHeader->u8FrameType);
    Media_Debug("u32FrameNO=%d", _pstHeader->u32FrameNO);
    Media_Debug("u32TimeStamp=%d", _pstHeader->u32TimeStamp);
    Media_Debug("u32VStreamLen=%d", _pstHeader->u32VStreamLen);
    Media_Debug("u16AFrames=%d", _pstHeader->u16AFrames);
    Media_Debug("u16AFrameSize=%d", _pstHeader->u16AFrameSize);
    Media_Debug("u8ACoder=%d", _pstHeader->u8ACoder);
    Media_Debug("reserved=%d", _pstHeader->reserved);
}


static int SetFileHeader(S_FileHeader *_pstFileHeader, int _iMode, int _iAudioSample)
{
    _pstFileHeader->FrameRate = 25;
    _pstFileHeader->Width = 1920;
    _pstFileHeader->Height = 1080;
    _pstFileHeader->Mode = _iMode;
    _pstFileHeader->bAudio = (_iAudioSample > 1 ? 1 : 0);
    _pstFileHeader->Year = 2017;
    _pstFileHeader->Month = 7;
    _pstFileHeader->Day = 27;
    _pstFileHeader->Hour = 9;
    _pstFileHeader->Minute = 29;
    _pstFileHeader->Second = 5;
    _pstFileHeader->CoverMask = 255;
    strncpy(_pstFileHeader->cCovWord, "S MULTI-MEDIA STREAM (H.264)", sizeof(_pstFileHeader->cCovWord) - 1);
    _pstFileHeader->m_uiFrameNoDiff = 0xffffffff;
    _pstFileHeader->TriggerHigh = 0;
    _pstFileHeader->TriggerLow = 0;
    _pstFileHeader->reserved = 0;
    _pstFileHeader->AChannels = 1;
    _pstFileHeader->BitsPerSample = 16;
    _pstFileHeader->AudioSample = _iAudioSample;
    _pstFileHeader->TotalSize = 0;
    _pstFileHeader->FrameCount = 0;

    return 0;
}

static void ShowSDVFileHeadInfo(S_FileHeader *pstSDVFileHeadBuf)
{
    S_FileHeader* pstFileHead = NULL;

    pstFileHead = pstSDVFileHeadBuf;
    
    if(NULL == pstSDVFileHeadBuf)
    {
        Media_Debug("invalid pstSDVFileHeadBuf(%p)", pstSDVFileHeadBuf);
        goto end;
    }
    
    if(NULL != pstFileHead)
    {
        Media_Debug("\nSDVFileHead Info begin:\n");
        Media_Debug(" AChannels      =%d\n",pstFileHead->AChannels);
        Media_Debug(" AudioSample    =%u\n",pstFileHead->AudioSample);
        Media_Debug(" bAudio         =%d\n",pstFileHead->bAudio);
        Media_Debug(" BitsPerSample  =%d\n",pstFileHead->BitsPerSample);
        Media_Debug(" cCovWord       =%s\n",pstFileHead->cCovWord);
        Media_Debug(" CoverMask      =%u\n",pstFileHead->CoverMask);
        Media_Debug(" Day            =%d\n",pstFileHead->Day);
        Media_Debug(" FrameCount     =%u\n",pstFileHead->FrameCount);
        Media_Debug(" FrameRate      =%d\n",pstFileHead->FrameRate);
        Media_Debug(" Height         =%d\n",pstFileHead->Height);
        Media_Debug(" Hour           =%d\n",pstFileHead->Hour);
        Media_Debug(" Minute         =%d\n",pstFileHead->Minute);
        Media_Debug(" Mode           =%d\n",pstFileHead->Mode);
        Media_Debug(" Month          =%d\n",pstFileHead->Month);
        Media_Debug(" m_uiFrameNoDiff=%u\n",pstFileHead->m_uiFrameNoDiff);
        Media_Debug(" reserved       =%d\n",pstFileHead->reserved);
        Media_Debug(" Second         =%d\n",pstFileHead->Second);
        Media_Debug(" TotalSize      =%d\n",pstFileHead->TotalSize);
        Media_Debug(" TriggerHigh    =%d\n",pstFileHead->TriggerHigh);
        Media_Debug(" TriggerLow     =%d\n",pstFileHead->TriggerLow);
        Media_Debug(" Width          =%d\n",pstFileHead->Width);
        Media_Debug(" Year           =%d\n",pstFileHead->Year);
        Media_Debug("\nSDVFileHead Info end.\n");
    }
    
end:
    return; 
}


#if 0
static int ps_ba()
{
    uint8_t pBuf[PS_HEADER_LEN_MAX] = {0};
    int64_t timestamp = 0;
    unsigned int uiStuffValue = 0;
    int iPsHeaderLen = 0;
    
    iPsHeaderLen = Mpeg_PutPackHeader(pBuf, timestamp, uiStuffValue);
    Media_Debug("iPsHeaderLen=%d\n", iPsHeaderLen);

    return 0;
}

static int ps_bb()
{
    uint8_t pBuf[PS_SYSTEM_HEADER_LEN_MAX] = {0};
    int only_for_stream_id = 0;
    int audio_bound = 0;
    int video_bound = 0;
    int iPsSysHeaderLen = 0;
    
    iPsSysHeaderLen = Mpeg_PutSystemHeader(pBuf);
    Media_Debug("iPsSysHeaderLen=%d\n", iPsSysHeaderLen);
    PrintHexData(pBuf, iPsSysHeaderLen, 16);
    return 0;
}

static int ps_bc()
{ 
    uint8_t pBuf[PS_MAP_HEADER_LEN_MAX] = {0};
    char cPSInfoData[64] = {0};
    char cESInfoData[64] = {0};
    PSMHeader stPSMHeader = {0};
    int iPsMapHeaderLen = 0;
    
    Mpeg_InitPsmHeader(&stPSMHeader);

    Mpeg_SetProgramStreamInfo(&stPSMHeader, cPSInfoData, 0x24);
    Mpeg_SetElementaryStream(&stPSMHeader, 0, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_SVC, cESInfoData, 0x1c);
    Mpeg_SetElementaryStream(&stPSMHeader, 1, AVMEDIA_TYPE_AUDIO, AV_CODEC_ID_SAC, cESInfoData, 0x0c);
    iPsMapHeaderLen = Mpeg_PutPsmHeader(&stPSMHeader, pBuf);
    Media_Debug("iPsMapHeaderLen=%d\n", iPsMapHeaderLen);
    PrintHexData(pBuf, iPsMapHeaderLen, 16);

    return 0;
}

static int pes_sps()
{
    uint8_t pBuf[PS_PES_HEADER_LEN_MAX] = {0};
    int iPaylodLen = 0;
    int iPayloadUsed = 0;
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;
    uint8_t StuffingByte[PES_STUFFING_BYTE_SPS_LEN] = {0xff, 0xff, 0xff, 0xff, 0xfc};
    
    iPesHeaderLen = Mpeg_PutPesHeader(pBuf, VIDEO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, iPaylodLen, PES_PTS_DTS_0B10, ullPts, ullDts, PES_STUFFING_BYTE_SPS_LEN, StuffingByte);
    Media_Debug("iPesHeaderLen=%d\n", iPesHeaderLen);
    PrintHexData(pBuf, iPesHeaderLen, 16);
    
    return 0;
}

static int pes_pps()
{
    uint8_t pBuf[64] = {0};
    int iPaylodLen = 0;
    int iPayloadUsed = 0;
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;
    uint8_t StuffingByte[PES_STUFFING_BYTE_PPS_LEN] = {0xff, 0xff, 0xfc};
    
    iPesHeaderLen = Mpeg_PutPesHeader(pBuf, VIDEO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, iPaylodLen, PES_PTS_DTS_0B00, ullPts, ullDts, PES_STUFFING_BYTE_PPS_LEN, StuffingByte);
    Media_Debug("iPesHeaderLen=%d\n", iPesHeaderLen);
    PrintHexData(pBuf, iPesHeaderLen, 16);
    
    return 0;
}

static int pes_idrs()
{
    uint8_t pBuf[64] = {0};
    int iPaylodLen = 0;
    int iPayloadUsed = 0;
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;
    uint8_t StuffingByte[PES_STUFFING_BYTE_IDRS_LEN] = {0xff, 0xff, 0xff, 0xfd};
    
    iPesHeaderLen = Mpeg_PutPesHeader(pBuf, VIDEO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, iPaylodLen, PES_PTS_DTS_0B00, ullPts, ullDts, PES_STUFFING_BYTE_IDRS_LEN, StuffingByte);
    Media_Debug("iPesHeaderLen=%d\n", iPesHeaderLen);
    PrintHexData(pBuf, iPesHeaderLen, 16);
    
    return 0;
}

static int pes_idrc()
{
    uint8_t pBuf[64] = {0};
    int iPaylodLen = 0;
    int iPayloadUsed = 0;
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;
    uint8_t StuffingByte[PES_STUFFING_BYTE_IDRC_LEN] = {0xff, 0xff, 0xfa};
    
    iPesHeaderLen = Mpeg_PutPesHeader(pBuf, VIDEO_ID, PES_DATA_ALIGNMENT_INDICATOR_CONTINUE, iPaylodLen, PES_PTS_DTS_0B00, ullPts, ullDts, PES_STUFFING_BYTE_IDRC_LEN, StuffingByte);
    Media_Debug("iPesHeaderLen=%d\n", iPesHeaderLen);
    PrintHexData(pBuf, iPesHeaderLen, 16);
    
    return 0;
}

static int pes_p()
{
    uint8_t pBuf[64] = {0};
    int iPaylodLen = 0;
    int iPayloadUsed = 0;
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;
    uint8_t StuffingByte[PES_STUFFING_BYTE_P_LEN] = {0xff, 0xff, 0xff, 0xf8};
    
    iPesHeaderLen = Mpeg_PutPesHeader(pBuf, VIDEO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, iPaylodLen, PES_PTS_DTS_0B10, ullPts, ullDts, PES_STUFFING_BYTE_P_LEN, StuffingByte);
    Media_Debug("iPesHeaderLen=%d\n", iPesHeaderLen);
    PrintHexData(pBuf, iPesHeaderLen, 16);
    
    return 0;
}

static int pes()
{
    pes_sps();
    Media_Debug("===========================================");
    pes_pps();
    Media_Debug("===========================================");
    pes_idrs();
    Media_Debug("===========================================");
    pes_idrc();
    Media_Debug("===========================================");
    pes_p();
    Media_Debug("===========================================");
    
    return 0;
}


uint8_t data[65556] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x01, 0x02, 0x03, 0x04, 
                      0x00, 0x00, 0x00, 0x01, 0x68, 0x05, 0x06, 0x07, 0x08,
                      0x00, 0x00, 0x00, 0x01, 0x65, 0x09, 0x0a, 0x0b, 0x0c,};


static uint64_t read_scr(uint8_t *_pBuf, int _iSize)
{
    int iSize = 0;
    bs_t pb;
    int iBits = 0;
    
    int value = 0;
    uint64_t scr = 0;
    
    bs_init(&pb, _pBuf, _iSize);

    bs_skip(&pb, 34);
    value = bs_read(&pb, 3);
    scr |= (value << 30);
    bs_skip(&pb, 1);
    value = bs_read(&pb, 15);
    scr |= (value << 15);
    bs_skip(&pb, 1);
    value = bs_read(&pb, 15);
    scr |= (value << 0);
    bs_skip(&pb, 1);
    Media_Debug("scr=%lld", scr);

    return scr;
}

static uint64_t read_pts(uint8_t *_pBuf, int _iSize)
{
    int iSize = 0;
    bs_t pb;
    int iBits = 0;
    
    int value = 0;
    uint64_t pts = 0;
    int flag = 0;
    bs_init(&pb, _pBuf, _iSize);

    bs_skip(&pb, 32);
    bs_skip(&pb, 16);
    bs_skip(&pb, 8);
    flag = bs_read(&pb, 2);//flag pts-dts
    Media_Debug("flag=0x%x", flag);
    bs_skip(&pb, 18);
    value = bs_read(&pb, 3);//flag pts-dts
    pts |= (value << 30);
    bs_skip(&pb, 1);
    value = bs_read(&pb, 15);
    pts |= (value << 15);
    bs_skip(&pb, 1);
    value = bs_read(&pb, 15);
    pts |= (value << 0);
    bs_skip(&pb, 1);
    Media_Debug("pts=%lld", pts);

    return pts;
}

static void get_scr_ba_hk(void)
{
    uint64_t prev = 0;
    uint64_t curr = 0;
    
    uint8_t _pBuf1[] = {0x00, 0x00, 0x01, 0xBA, 0x6F, 0x80, 0x24, 0x60, 0xF4, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x1A, 0x2F, 0xD1};
    curr = read_scr(_pBuf1, sizeof(_pBuf1));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf2[] = {0x00, 0x00, 0x01, 0xBA, 0x6F, 0x80, 0x24, 0xCB, 0xD4, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x1A, 0x2F, 0xD2};
    curr = read_scr(_pBuf2, sizeof(_pBuf2));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf3[] = {0x00, 0x00, 0x01, 0xBA, 0x6F, 0x80, 0x2C, 0x4F, 0xD4, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x1A, 0x2F, 0xD3};
    curr = read_scr(_pBuf3, sizeof(_pBuf3));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf4[] = {0x00, 0x00, 0x01, 0xBA, 0x6F, 0x80, 0x2C, 0xC0, 0x54, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x1A, 0x2F, 0xD4};
    curr = read_scr(_pBuf4, sizeof(_pBuf4));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf5[] = {0x00, 0x00, 0x01, 0xBA, 0x6F, 0x80, 0x2D, 0x30, 0xD4, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x1A, 0x2F, 0xD5};
    curr = read_scr(_pBuf5, sizeof(_pBuf5));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf6[] = {0x00, 0x00, 0x01, 0xBA, 0x6F, 0x80, 0x2D, 0xA1, 0x54, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x1A, 0x2F, 0xD6};
    curr = read_scr(_pBuf6, sizeof(_pBuf6));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf7[] = {0x00, 0x00, 0x01, 0xBA, 0x6F, 0x80, 0x2E, 0x17, 0x74, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x1A, 0x2F, 0xD7};
    curr = read_scr(_pBuf7, sizeof(_pBuf7));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
}


static void get_pts_e0_hk(void)
{
    uint64_t prev = 0;
    uint64_t curr = 0;
    
    uint8_t _pBuf1[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x26, 0x8C, 0x80, 0x0A, 0x2B, 0xE0, 0x09, 0x18, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    curr = read_pts(_pBuf1, sizeof(_pBuf1));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf2[] = {0x00, 0x00, 0x01, 0xE0, 0x19, 0x9E, 0x8C, 0x80, 0x09, 0x2B, 0xE0, 0x09, 0x32, 0xF5, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf2, sizeof(_pBuf2));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf3[] = {0x00, 0x00, 0x01, 0xE0, 0x19, 0x9E, 0x8C, 0x80, 0x09, 0x2B, 0xE0, 0x09, 0x32, 0xF5, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf3, sizeof(_pBuf3));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf4[] = {0x00, 0x00, 0x01, 0xE0, 0x18, 0x5E, 0x8C, 0x80, 0x09, 0x2B, 0xE0, 0x0B, 0x30, 0x15, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf4, sizeof(_pBuf4));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf5[] = {0x00, 0x00, 0x01, 0xE0, 0x11, 0x9A, 0x8C, 0x80, 0x07, 0x2B, 0xE0, 0x0B, 0x4C, 0x35, 0xFF, 0xF8};
    curr = read_pts(_pBuf5, sizeof(_pBuf5));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf6[] = {0x00, 0x00, 0x01, 0xE0, 0x10, 0x76, 0x8C, 0x80, 0x07, 0x2B, 0xE0, 0x0B, 0x68, 0x55, 0xFF, 0xF8};
    curr = read_pts(_pBuf6, sizeof(_pBuf6));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf7[] = {0x00, 0x00, 0x01, 0xE0, 0x0F, 0xFE, 0x8C, 0x80, 0x09, 0x2B, 0xE0, 0x0B, 0x85, 0xDD, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf7, sizeof(_pBuf7));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
}


static void get_scr_ba_td(void)
{
    uint64_t prev = 0;
    uint64_t curr = 0;
    
    uint8_t _pBuf1[] = {0x00, 0x00, 0x01, 0xBA, 0x6E, 0x46, 0x5C, 0xB2, 0x14, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
    curr = read_scr(_pBuf1, sizeof(_pBuf1));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf2[] = {0x00, 0x00, 0x01, 0xBA, 0x6E, 0x46, 0x5D, 0x5A, 0xD4, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01};
    curr = read_scr(_pBuf2, sizeof(_pBuf2));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf3[] = {0x00, 0x00, 0x01, 0xBA, 0x6E, 0x46, 0x5D, 0x5A, 0xD4, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01};
    curr = read_scr(_pBuf3, sizeof(_pBuf3));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf4[] = {0x00, 0x00, 0x01, 0xBA, 0x6E, 0x46, 0x5E, 0x03, 0x94, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x03};
    curr = read_scr(_pBuf4, sizeof(_pBuf4));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf5[] = {0x00, 0x00, 0x01, 0xBA, 0x6E, 0x46, 0x5F, 0x1C, 0xD4, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x04};
    curr = read_scr(_pBuf5, sizeof(_pBuf5));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf6[] = {0x00, 0x00, 0x01, 0xBA, 0x6E, 0x46, 0x5F, 0x8D, 0x54, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x05};
    curr = read_scr(_pBuf6, sizeof(_pBuf6));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf7[] = {0x00, 0x00, 0x01, 0xBA, 0x6E, 0x46, 0x5F, 0xC5, 0x94, 0x01, 0x02, 0x8F, 0x63, 0xFE, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x06};
    curr = read_scr(_pBuf7, sizeof(_pBuf7));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
}


static void get_pts_e0_td(void)
{
    uint64_t prev = 0;
    uint64_t curr = 0;
    
    uint8_t _pBuf1[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x8C, 0x80, 0x0A, 0x2B, 0x91, 0x97, 0x48, 0xA5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    curr = read_pts(_pBuf1, sizeof(_pBuf1));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf2[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x8C, 0x80, 0x0A, 0x2B, 0x91, 0x97, 0x48, 0xA5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    curr = read_pts(_pBuf2, sizeof(_pBuf2));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf3[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x0C, 0x8C, 0x80, 0x09, 0x2B, 0x91, 0x97, 0x8E, 0xF5, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf3, sizeof(_pBuf3));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf4[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x3C, 0x8C, 0x80, 0x0A, 0x2B, 0x91, 0x97, 0x9D, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    curr = read_pts(_pBuf4, sizeof(_pBuf4));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf5[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x0C, 0x8C, 0x80, 0x09, 0x2B, 0x91, 0x97, 0xE3, 0x55, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf5, sizeof(_pBuf5));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf6[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x0C, 0x8C, 0x80, 0x09, 0x2B, 0x91, 0x97, 0xFF, 0x75, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf6, sizeof(_pBuf6));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
    uint8_t _pBuf7[] = {0x00, 0x00, 0x01, 0xE0, 0x00, 0x0C, 0x8C, 0x80, 0x09, 0x2B, 0x91, 0x99, 0x0D, 0x85, 0xFF, 0xFF, 0xFF, 0xF8};
    curr = read_pts(_pBuf7, sizeof(_pBuf7));    
    Media_Debug("curr - prev=%lld", curr - prev);
    prev = curr;
}
#endif

static int WriteFilePs(char *_pcPsFileName, DataInfo *_piov, int _iNum)
{
    int i = 0;
    int iRet = 0;
    static int iFd = 0;
    if(0 == iFd)
    {
        if(NULL == _pcPsFileName)
        {
            iRet = -1;
            goto end;
        }
        iFd = open(_pcPsFileName, O_RDWR|O_APPEND|O_CREAT, 0644);
    }
    
    Media_Debug("====================================write file end_iNum=%d\n", _iNum);

    for(i = 0; i < _iNum; i++)
    {
        iRet = write(iFd, _piov[i].m_pvAddr, _piov[i].m_uiLen);
    }

end:
    return iRet;

}


static int WriteFileVideoRaw(char *_pcPsFileName, struct iovec *_piov, int _iNum)
{
    return 0;
    int i = 0;
    int iRet = 0;
    static int iFd = 0;
    if(0 == iFd)
    {
        if(NULL == _pcPsFileName)
        {
            iRet = -1;
            goto end;
        }
        iFd = open(_pcPsFileName, O_RDWR|O_APPEND|O_CREAT, 0644);
    }
    
    Media_Debug("====================================write file end_iNum=%d\n", _iNum);

    for(i = 0; i < _iNum; i++)
    {
        iRet = write(iFd, _piov[i].iov_base, _piov[i].iov_len);
    }

end:
    return iRet;

}

static int WriteFileAudioRaw(char *_pcPsFileName, struct iovec *_piov, int _iNum)
{
    return 0;
    int i = 0;
    int iRet = 0;
    static int iFd = 0;
    if(0 == iFd)
    {
        if(NULL == _pcPsFileName)
        {
            iRet = -1;
            goto end;
        }
        iFd = open(_pcPsFileName, O_RDWR|O_APPEND|O_CREAT, 0644);
    }
    
    Media_Debug("====================================write file end_iNum=%d\n", _iNum);

    for(i = 0; i < _iNum; i++)
    {
        iRet = write(iFd, _piov[i].iov_base, _piov[i].iov_len);
    }

end:
    return iRet;

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
    
end:
    return iRet;
}

static int AudioInfoProbe(unsigned char _u8ACoder, unsigned short _u16AFrameSize)
{
    AVCodecID eACodecID = AV_CODEC_ID_NONE;

    if(1 == _u8ACoder)
    {
        eACodecID = AV_CODEC_ID_PCM_ALAW;
        Media_Debug("AV_CODEC_ID_PCM_ALAW");
        //g711a
        if(324 == _u16AFrameSize)
        {
            //8k
        }
    }else if(2 == _u8ACoder)
    {
        eACodecID = AV_CODEC_ID_PCM_MULAW;
        Media_Debug("AV_CODEC_ID_PCM_MULAW");
        //g711u
        if(324 == _u16AFrameSize)
        {
            //8k
        }        
    }else if(3 == _u8ACoder)
    {
        eACodecID = AV_CODEC_ID_ADPCM_IMA_APC;
        Media_Debug("AV_CODEC_ID_ADPCM_IMA_APC");
        //adpcm-d
        if(168 == _u16AFrameSize)
        {
            //8k
        }        
    }else if(22 == _u8ACoder)
    {
        eACodecID = AV_CODEC_ID_AAC;
        Media_Debug("AV_CODEC_ID_AAC");
        //aac-lc
    }else
    {
        Media_Debug("no probe audio codec type");
    }

    return eACodecID;
}


typedef struct
{//ISO/IEC 14496-3 ADTS部分
    //adts_fixed_header
    int synword;                                        //0~11      12 bslbf
    unsigned char ID;                                   //12            1  bslbf
    unsigned char layer;                                //13~14     2  uimsbf
    unsigned char protection_absent;                    //15            1  bslbf
    unsigned char profile_ObjectType;                   //16~17     2  uimsbf
    unsigned char sampling_frequency_index;         //18~21     4  uimsbf
    unsigned char private_bit;                          //22            1  bslbf
    unsigned char channel_configuration;                //23~25     3  uimsbf
    unsigned char original_copy;                        //26            1  bslbf
    unsigned char home;                             //27            1  bslbf
    //adts_variable_header
    unsigned char copyright_identification_bit;         //28            1  bslbf
    unsigned char copyright_identification_start;           //29            1  bslbf
    unsigned char _[1];
    int aac_frame_length;                               //30~42     13 bslbf
    int adts_buffer_fullness;                           //33~53     11 bslbf
    unsigned char number_of_raw_data_blocks_in_frame;   //54~55     2 uimsfb
    unsigned char __[3];
}TADTSHeader;

#define ADTS_HEADER_LENGTH 7

static int decode_adts_header(TADTSHeader *header, char *aac_buf, int aac_len)
{
    if(NULL==header|| NULL==aac_buf || aac_len<ADTS_HEADER_LENGTH) 
    {
        Media_Trace("(NULL==header|| NULL==aac_buf || aac_len<ADTS_HEADER_LENGTH)");
        return -1;
    }

    if (((unsigned char)(aac_buf[0]) == 0xFF)&&(((unsigned char)(aac_buf[1]) & 0xF0) == 0xF0))      //syncword 12个1
    {
        header->synword = (aac_buf[0] << 4 )  | (aac_buf[1] >> 4);
        header->ID = ((unsigned int) aac_buf[1] & 0x08) >> 3;
        header->layer = ((unsigned int) aac_buf[1] & 0x06) >> 1;
        header->protection_absent = (unsigned int) aac_buf[1] & 0x01;
        header->profile_ObjectType = ((unsigned int) aac_buf[2] & 0xc0) >> 6;
        header->sampling_frequency_index = ((unsigned int) aac_buf[2] & 0x3c) >> 2;
        header->private_bit = ((unsigned int) aac_buf[2] & 0x02) >> 1;
        header->channel_configuration = ((((unsigned int) aac_buf[2] & 0x01) << 2) | (((unsigned int) aac_buf[3] & 0xc0) >> 6));
        header->original_copy = ((unsigned int) aac_buf[3] & 0x20) >> 5;
        header->home = ((unsigned int) aac_buf[3] & 0x10) >> 4;
        header->copyright_identification_bit = ((unsigned int) aac_buf[3] & 0x08) >> 3;
        header->copyright_identification_start = (unsigned int) aac_buf[3] & 0x04 >> 2;     
        header->aac_frame_length = (((((unsigned int) aac_buf[3]) & 0x03) << 11) | (((unsigned int) aac_buf[4] & 0xFF) << 3)| ((unsigned int) aac_buf[5] & 0xE0) >> 5) ;
        header->adts_buffer_fullness = (((unsigned int) aac_buf[5] & 0x1f) << 6 | ((unsigned int) aac_buf[6] & 0xfc) >> 2);
        header->number_of_raw_data_blocks_in_frame = ((unsigned int) aac_buf[6] & 0x03);

        return 0;
    }
    else 
    {
        Media_Trace("ADTS_HEADER : BUF ERROR\n");

        return -1;
    }
    
}

static int get_aac_frame_count(void * _pcframedata, int size)
{
    int iCountaac = 0;
    char *audioStart = NULL;
    int audioLeftSize = 0;
    
    if (NULL==_pcframedata || size<=0)
    {
        Media_Error("Input parameters is error!\n");
        return -1;
    }
    
    audioStart = (char*)_pcframedata;
    audioLeftSize = size - sizeof(S_FrameHeader);
    for(iCountaac = 0; audioLeftSize > ADTS_HEADER_LENGTH; iCountaac++)
    {
        TADTSHeader tHeader;
        decode_adts_header(&tHeader, audioStart, audioLeftSize);
        if(tHeader.aac_frame_length <= 0)
        {
            break;
        }
        audioStart += tHeader.aac_frame_length;
        audioLeftSize -= tHeader.aac_frame_length;
    }
    
    return iCountaac;
}

static int LiveStreamToPs(int argc, char ** argv)
{
    int iRet = 0;
    FILE *pSdvFh = NULL;
    S_FileHeader stFileHeader = {0};
    char *pcDataBuf = NULL;
    int iDataBufLen = 0;    
    char *pcFrameBuffer = NULL;
    int iFrameLen = 0;
    S_FrameHeader *pstFrameHeader = NULL;
    int iMediaDataLen = 0;
    
    PSHandle *pH = NULL;
    FrameOutInfo pstFrameOutInfo;

    //rtp
    int payloadtype = 0x3;
    int sequencenumber = 0x8;
    int timestamp = 0;
    int ssrc = 0xbeef;
    AVCodecID eVCodecID = AV_CODEC_ID_NONE;
    AVCodecID eACodecID = AV_CODEC_ID_NONE;

    if(argc < 3)
    {
        Media_Error("argc(%d) < 3", argc < 3);
        return -1;
    }

    pSdvFh = fopen(argv[1], "r");
    if(NULL == pSdvFh)
    {
        Media_Error("call fopen failed");
        return -1;
    }

    if(fread((void *)(&stFileHeader), 1, sizeof(stFileHeader), pSdvFh) != sizeof(S_FileHeader))
    {
        Media_Error("call fread failed");
    }
    
    pcDataBuf = (char *)malloc(sizeof(S_FrameHeader));
    if(NULL == pcDataBuf)
    {
        Media_Error("call malloc failed");
        return -1;
    }
    iDataBufLen = (int)sizeof(S_FrameHeader);
    pstFrameHeader = (S_FrameHeader *)pcDataBuf;
    pcFrameBuffer = pcDataBuf;

    pH = PS_Create();
    if(NULL == pH)
    {
        Media_Error("call PS_Create failed");
        iRet = -1;
        goto end;
    }
        
    while(1)
    {
        iFrameLen = 0;
        if(fread((void *)pstFrameHeader, 1, (size_t)sizeof(S_FrameHeader), pSdvFh) != (size_t)sizeof(S_FrameHeader))
        {
            if(feof(pSdvFh) != 0)
            {
                Media_Debug("feof end");
                break;
            }
            Media_Error("call fread failed");
        }
        iFrameLen += (int)sizeof(S_FrameHeader);
        if(0x02000000 != pstFrameHeader->u32FrameID)
        {
            Media_Error("0x02000000 != pstFrameHeader->u32FrameID(0x%x)", pstFrameHeader->u32FrameID);
            break;
        }
        
        iMediaDataLen = (int)pstFrameHeader->u32VStreamLen + (int)(pstFrameHeader->u16AFrames * pstFrameHeader->u16AFrameSize);
        if((iDataBufLen - (int)sizeof(S_FrameHeader)) < iMediaDataLen)
        {
            if(NULL != pcDataBuf)
            {
                S_FrameHeader stFrameHeader = {0};
                memcpy((void *)&stFrameHeader, pstFrameHeader, sizeof(S_FrameHeader));
                
                free(pcDataBuf);
                pcDataBuf = NULL;
                iDataBufLen = 0;
                
                iDataBufLen = (int)sizeof(S_FrameHeader) + iMediaDataLen;//4096
                iDataBufLen |= (0xfff);/*地址页对齐*/
                iDataBufLen++;
                
                pcDataBuf = (char *)malloc((size_t)iDataBufLen);
                if(NULL == pcDataBuf)
                {
                    iDataBufLen = 0;
                    Media_Error("call malloc failed");
                    return -1;
                }
                pstFrameHeader = (S_FrameHeader *)pcDataBuf;
                pcFrameBuffer = pcDataBuf;
                
                memcpy(pstFrameHeader, (void *)&stFrameHeader, sizeof(S_FrameHeader));
            }
        }
        
        //Media_Debug("pstFrameHeader->u32FrameID(0x%x)", pstFrameHeader->u32FrameID);
        if(fread((void *)(pcFrameBuffer + sizeof(S_FrameHeader)), 1, (size_t)iMediaDataLen, pSdvFh) != (size_t)iMediaDataLen)
        {
            if(feof(pSdvFh) != 0)
            {
                Media_Debug("feof end");
                break;
            }
            Media_Error("call fread failed");
        }
        iFrameLen += iMediaDataLen;
        
        int i = 0;
        char *pcAData = NULL;
        int iALen = 0;
        int iAOffset = 0;
        //Media_Debug("pstFrameHeader->u16AFrames=%d", pstFrameHeader->u16AFrames);
        //Media_Debug("pstFrameHeader->u8ACoder=%d", pstFrameHeader->u8ACoder);
        ShowFrameHeader(pstFrameHeader);
        Media_Debug("=================================");
        #if 0
        //h264 h265 video raw
        ResetFrameOutInfo(&pstFrameOutInfo);

        struct iovec iovecTmp;
        pstFrameOutInfo.m_pstDI = &iovecTmp;
        pstFrameOutInfo.m_iDICount = 1;

        iovecTmp.iov_base = pcFrameBuffer + sizeof(S_FrameHeader);
        iovecTmp.iov_len = pstFrameHeader->u32VStreamLen;
        if(WriteFileVideoRaw(argv[2], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
        {

        } 
        
        //audio raw
        ResetFrameOutInfo(&pstFrameOutInfo);

        pstFrameOutInfo.m_pstDI = &iovecTmp;
        pstFrameOutInfo.m_iDICount = 1;

        iovecTmp.iov_base = pcFrameBuffer + sizeof(S_FrameHeader) + pstFrameHeader->u32VStreamLen;
        iovecTmp.iov_len = pstFrameHeader->u16AFrames * pstFrameHeader->u16AFrameSize;
        if(WriteFileAudioRaw(argv[3], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
        {

        }  
        
        #endif


        #if 1
        //video ps
        if(AV_CODEC_ID_NONE == eVCodecID)
        {
            if(H264_Probe((unsigned char *)pcFrameBuffer + sizeof(S_FrameHeader), (int)pstFrameHeader->u32VStreamLen) == 1)
            {
                eVCodecID = AV_CODEC_ID_H264;
                Media_Debug("AV_CODEC_ID_H264");
            }else if(H265_Probe((unsigned char *)pcFrameBuffer + sizeof(S_FrameHeader), (int)pstFrameHeader->u32VStreamLen) == 1)
            {
                eVCodecID = AV_CODEC_ID_HEVC;
                Media_Debug("AV_CODEC_ID_HEVC");
            }else
            {
                Media_Debug("no probe video codec type, continue");
                continue;
            }

            VideoEs stVideoEs;
            stVideoEs.m_eVCodecID = eVCodecID;
            stVideoEs.m_uiFrameRate = 25;
            stVideoEs.m_uiBitRate = 8196;
            stVideoEs.m_uiWidth = 1920;
            stVideoEs.m_uiHeight = 1080;
            stVideoEs.m_iIsSendSEI = 0;
            
            if(PS_SetVideoEs(pH, &stVideoEs) < 0)
            {
                Media_Error("call PS_SetVideoEs failed");
                iRet = -1;
                goto end;
            }
        }

        if(AV_CODEC_ID_NONE == eACodecID)
        {
            eACodecID = AudioInfoProbe(pstFrameHeader->u8ACoder, pstFrameHeader->u16AFrameSize);
            if(AV_CODEC_ID_NONE != eACodecID)
            {
                AudioEs stAudioEs;
                stAudioEs.m_eACodecID = eACodecID;
                stAudioEs.m_uiSampleRate = 8000;
                stAudioEs.m_uiBitRate = 256;
                
                if(PS_SetAudioEs(pH, &stAudioEs) < 0)
                {
                    Media_Error("call PS_SetAudioEs failed");
                    iRet = -1;
                    goto end;
                }
            }else
            {
                Media_Debug("no probe audio codec type");
            }
        }
        
        ResetFrameOutInfo(&pstFrameOutInfo);
        
        PsFrameInfo stPsFrameInfo;
        stPsFrameInfo.m_TimestampMs = pstFrameHeader->u32TimeStamp;
        stPsFrameInfo.m_isSyncFrame = (pstFrameHeader->u8FrameType == 0 ? 1 : 0);
    
        if(Ps_PutVideoData(pH, pcFrameBuffer + sizeof(S_FrameHeader), (int)pstFrameHeader->u32VStreamLen, &stPsFrameInfo) < 0)
        {
            Media_Error("call Ps_PutVideoData failed");
        }
        
        if(PS_GetVideoPackage(pH, &pstFrameOutInfo, NULL) < 0)
        {
            Media_Error("call PS_GetVideoPackage failed");
        }
        if(WriteFilePs(argv[2], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
        {

        } 
        
        
        #endif

        #if 1
        //audio ps
        pcAData = pcFrameBuffer + sizeof(S_FrameHeader) + pstFrameHeader->u32VStreamLen;
        iALen = pstFrameHeader->u16AFrameSize * pstFrameHeader->u16AFrames;
        iAOffset = pstFrameHeader->u16AFrameSize;

        char pcAudio[8] = {0xd5, 0xd5, 0xd5, 0xd5, 0xd5, 0xd5, 0xd5, 0xd5};
        if(AV_CODEC_ID_PCM_ALAW == eACodecID || AV_CODEC_ID_PCM_MULAW == eACodecID)
        {            
            for(i = 0; i < pstFrameHeader->u16AFrames; i++)
            {
                pcAData = pcAData + i * iAOffset;
                ResetFrameOutInfo(&pstFrameOutInfo);
                if(PS_PutAudioData(pH, pcAData + 4, iAOffset - 4, &stPsFrameInfo) < 0)
                {
                    Media_Error("call PS_PutAudioData failed");
                }
                if(PS_GetAudioPackage(pH, &pstFrameOutInfo, NULL) < 0)
                {
                    Media_Error("call PS_GetAudioPackage failed");
            
                }
                
                if(WriteFilePs(argv[2], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
                {
                    Media_Error("call WriteFilePs failed");
                
                }            
            }
        }else if(AV_CODEC_ID_AAC == eACodecID)
        {
            Media_Debug("pcAData[0]=0x%x", pcAData[0]);
            PrintHexData((unsigned char *)pcAData, 16, 16);
            if((unsigned char)(pcAData[0]) == 0xff && ((unsigned char)(pcAData[1]) >> 4) == 0xf)
            {
                int AACFrameCount = 0;
                AACFrameCount = get_aac_frame_count(pcAData, iALen);
                TADTSHeader tHeader = {0};
                char *audioStart = NULL;
                int audioLeftSize= 0;
            
                if(AACFrameCount <= 0)
                {
                    Media_Warn("AACFrameCount=%d", AACFrameCount);
                    goto end;
                }
                
                audioStart = pcAData;
                audioLeftSize = iALen;
                
                Media_Debug("AACFrameCount=%d", AACFrameCount);
                for(i = 0; i < AACFrameCount; i++)
                {
                    decode_adts_header(&tHeader, (char*)audioStart, audioLeftSize);

                    ResetFrameOutInfo(&pstFrameOutInfo);
                    stPsFrameInfo.m_TimestampMs = pstFrameHeader->u32TimeStamp;
                    if(PS_PutAudioData(pH, audioStart, tHeader.aac_frame_length, &stPsFrameInfo) < 0)
                    {
                        Media_Error("call PS_PutAudioData failed");
                    }
                    if(PS_GetAudioPackage(pH, &pstFrameOutInfo, NULL) < 0)
                    {
                        Media_Error("call PS_GetAudioPackage failed");
                    
                    }
                    
                    if(WriteFilePs(argv[2], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
                    {
                        Media_Error("call WriteFilePs failed");
                    
                    }  

                    
                    audioStart += tHeader.aac_frame_length;
                    audioLeftSize -= tHeader.aac_frame_length;
                }
            }

        }else if(AV_CODEC_ID_ADPCM_IMA_APC == eACodecID)
        {
            Media_Debug("AV_CODEC_ID_ADPCM_IMA_APC");
            Media_Debug("pstFrameHeader->u16AFrames=%d", pstFrameHeader->u16AFrames);
            Media_Debug("pstFrameHeader->u16AFrameSize=%d", pstFrameHeader->u16AFrameSize);        
            for(i = 0; i < pstFrameHeader->u16AFrames; i++)
            {
                pcAData = pcAData + i * iAOffset;
                ResetFrameOutInfo(&pstFrameOutInfo);
                if(PS_PutAudioData(pH, pcAData, iAOffset, &stPsFrameInfo) < 0)
                {
                    Media_Error("call PS_PutAudioData failed");
                }
                if(PS_GetAudioPackage(pH, &pstFrameOutInfo, NULL) < 0)
                {
                    Media_Error("call PS_GetAudioPackage failed");
            
                }
                
                if(WriteFilePs(argv[2], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
                {
                    Media_Error("call WriteFilePs failed");
                
                }   
                break;
            }

        }
        else
        {
            Media_Error("not support audio");

        }
        
        #endif
        
        #if 0
        //rtp
        int iSockFd = 0;
        
        NET_SendRtpByUdp(iSockFd, payloadtype, &sequencenumber, timestamp, htonl(ssrc), pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount);
        Media_Debug("sequencenumber=%d", sequencenumber);
        #endif
        
        (void)usleep(1000);

    }

    if(NULL != pcDataBuf)
    {
        free(pcDataBuf);
        pcDataBuf = NULL;
    }

    if(NULL != pSdvFh)
    {
        (void)fclose(pSdvFh);
        pSdvFh = NULL;
    }

    if(NULL != pH)
    {
        if(PS_Destroy(pH) < 0)
        {
            iRet = -1;
            goto end;
        }
    }

end:
    return iRet;
}


static int SdvToPs(int argc, char ** argv)
{
    int iRet = 0;
    FILE *pSdvFh = NULL;
    S_FileHeader stFileHeader = {0};
    char *pcDataBuf = NULL;
    int iDataBufLen = 0;    
    char *pcFrameBuffer = NULL;
    int iFrameLen = 0;
    S_FrameHeader *pstFrameHeader = NULL;
    int iMediaDataLen = 0;
    
    void *pPsFile = NULL;
    FrameOutInfo pstFrameOutInfo;

    //rtp
    int payloadtype = 0x3;
    int sequencenumber = 0x8;
    int timestamp = 0;
    int ssrc = 0xbeef;
    AVCodecID eVCodecID = AV_CODEC_ID_NONE;
    AVCodecID eACodecID = AV_CODEC_ID_NONE;
    
    PsFrameInfo stPsFrameInfo;

    if(argc < 3)
    {
        Media_Error("argc(%d) < 3", argc < 3);
        return -1;
    }

    pSdvFh = fopen(argv[1], "r");
    if(NULL == pSdvFh)
    {
        Media_Error("call fopen failed");
        return -1;
    }

    if(fread((void *)(&stFileHeader), 1, sizeof(stFileHeader), pSdvFh) != sizeof(S_FileHeader))
    {
        Media_Error("call fread failed");
    }

    ShowSDVFileHeadInfo(&stFileHeader);

    pcDataBuf = (char *)malloc(sizeof(S_FrameHeader));
    if(NULL == pcDataBuf)
    {
        Media_Error("call malloc failed");
        return -1;
    }
    iDataBufLen = (int)sizeof(S_FrameHeader);
    pstFrameHeader = (S_FrameHeader *)pcDataBuf;
    pcFrameBuffer = pcDataBuf;

    pPsFile = PsFile_Create(argv[2]);
    if(NULL == pPsFile)
    {
        Media_Error("call PS_Create failed");
        iRet = -1;
        goto end;
    }
        
    while(1)
    {
        iFrameLen = 0;
        Media_Debug("ftell(pSdvFh)=%d", ftell(pSdvFh));
        if(fread((void *)pstFrameHeader, 1, (size_t)sizeof(S_FrameHeader), pSdvFh) != (size_t)sizeof(S_FrameHeader))
        {
            if(feof(pSdvFh) != 0)
            {
                Media_Debug("feof end");
                break;
            }
            Media_Error("call fread failed");
        }
        iFrameLen += (int)sizeof(S_FrameHeader);
        if(0x02000000 != pstFrameHeader->u32FrameID)
        {
            Media_Error("0x02000000 != pstFrameHeader->u32FrameID(0x%x)", pstFrameHeader->u32FrameID);
            break;
        }
        
        iMediaDataLen = (int)pstFrameHeader->u32VStreamLen + (int)(pstFrameHeader->u16AFrames * pstFrameHeader->u16AFrameSize);
        if((iDataBufLen - (int)sizeof(S_FrameHeader)) < iMediaDataLen)
        {
            if(NULL != pcDataBuf)
            {
                S_FrameHeader stFrameHeader = {0};
                memcpy((void *)&stFrameHeader, pstFrameHeader, sizeof(S_FrameHeader));
                
                free(pcDataBuf);
                pcDataBuf = NULL;
                iDataBufLen = 0;
                
                iDataBufLen = (int)sizeof(S_FrameHeader) + iMediaDataLen;//4096
                iDataBufLen |= (0xfff);/*地址页对齐*/
                iDataBufLen++;
                
                Media_Debug("iDataBufLen=%d", iDataBufLen);
                pcDataBuf = (char *)malloc((size_t)iDataBufLen);
                if(NULL == pcDataBuf)
                {
                    iDataBufLen = 0;
                    Media_Error("call malloc failed");
                    return -1;
                }
                pstFrameHeader = (S_FrameHeader *)pcDataBuf;
                pcFrameBuffer = pcDataBuf;
                
                memcpy(pstFrameHeader, (void *)&stFrameHeader, sizeof(S_FrameHeader));
            }
        }
        
        //Media_Debug("pstFrameHeader->u32FrameID(0x%x)", pstFrameHeader->u32FrameID);
        if(fread((void *)(pcFrameBuffer + sizeof(S_FrameHeader)), 1, (size_t)iMediaDataLen, pSdvFh) != (size_t)iMediaDataLen)
        {
            if(feof(pSdvFh) != 0)
            {
                Media_Debug("feof end");
                break;
            }
            Media_Error("call fread failed");
        }
        iFrameLen += iMediaDataLen;
        
        int i = 0;
        char *pcAData = NULL;
        int iALen = 0;
        int iAOffset = 0;
        
        ShowFrameHeader(pstFrameHeader);
        Media_Debug("=================================");


        #if 1
        //video ps
        if(AV_CODEC_ID_NONE == eVCodecID)
        {
            if(H264_Probe((unsigned char *)pcFrameBuffer + sizeof(S_FrameHeader), (int)pstFrameHeader->u32VStreamLen) == 1)
            {
                eVCodecID = AV_CODEC_ID_H264;
                Media_Debug("AV_CODEC_ID_H264");
            }else if(H265_Probe((unsigned char *)pcFrameBuffer + sizeof(S_FrameHeader), (int)pstFrameHeader->u32VStreamLen) == 1)
            {
                eVCodecID = AV_CODEC_ID_HEVC;
                Media_Debug("AV_CODEC_ID_HEVC");
            }else
            {
                Media_Debug("no probe video codec type, continue");
                continue;
            }
        }

        

        if(AV_CODEC_ID_NONE == eACodecID)
        {
            eACodecID = AudioInfoProbe(pstFrameHeader->u8ACoder, pstFrameHeader->u16AFrameSize);            
        }

        FileAttribute stFileAttribute;
        stFileAttribute.m_eACodecID = eACodecID;
        stFileAttribute.m_eVCodecID = eVCodecID;
        
        if(PsFile_SetAttribute(pPsFile, &stFileAttribute) < 0)
        {
            Media_Error("call PsFile_SetAttribute failed");
            //iRet = -1;
            //goto end;
        }
        
        stPsFrameInfo.m_TimestampMs = pstFrameHeader->u32TimeStamp;
        
        stPsFrameInfo.m_eAVCodecID = eVCodecID;
        stPsFrameInfo.m_isSyncFrame = (pstFrameHeader->u8FrameType == 0 ? 1 : 0);
        if(PsFile_Write(pPsFile, &stPsFrameInfo, pcFrameBuffer + sizeof(S_FrameHeader), pstFrameHeader->u32VStreamLen) < 0)
        {
            Media_Error("call PsFile_Write failed");
            iRet = -1;
            goto end;
        }
        
        
        #endif

        #if 1
        //audio ps
        pcAData = pcFrameBuffer + sizeof(S_FrameHeader) + pstFrameHeader->u32VStreamLen;
        iALen = pstFrameHeader->u16AFrameSize * pstFrameHeader->u16AFrames;
        iAOffset = pstFrameHeader->u16AFrameSize;
        
        stPsFrameInfo.m_eAVCodecID = eACodecID;


        if(AV_CODEC_ID_PCM_ALAW == eACodecID || AV_CODEC_ID_PCM_MULAW == eACodecID)
        {            
            for(i = 0; i < pstFrameHeader->u16AFrames; i++)
            {
                PrintHexData((unsigned char *)pcAData, iAOffset, 32);
                if(PsFile_Write(pPsFile, &stPsFrameInfo, pcAData + 4, iAOffset - 4) < 0)
                {
                    Media_Error("call PsFile_Write failed");
                    iRet = -1;
                    goto end;
                }
                pcAData = pcAData + iAOffset;                   
            }
        }else if(AV_CODEC_ID_AAC == eACodecID)
        {
            Media_Debug("pcAData[0]=0x%x", pcAData[0]);
            PrintHexData((unsigned char *)pcAData, 16, 16);
            if((unsigned char)(pcAData[0]) == 0xff && ((unsigned char)(pcAData[1]) >> 4) == 0xf)
            {
                int AACFrameCount = 0;
                AACFrameCount = get_aac_frame_count(pcAData, iALen);
                TADTSHeader tHeader = {0};
                char *audioStart = NULL;
                int audioLeftSize= 0;
            
                if(AACFrameCount <= 0)
                {
                    Media_Warn("AACFrameCount=%d", AACFrameCount);
                    goto end;
                }
                
                audioStart = pcAData;
                audioLeftSize = iALen;
                
                Media_Debug("AACFrameCount=%d", AACFrameCount);
                for(i = 0; i < AACFrameCount; i++)
                {
                    decode_adts_header(&tHeader, (char*)audioStart, audioLeftSize);
                    
                    if(PsFile_Write(pPsFile, &stPsFrameInfo, audioStart, tHeader.aac_frame_length) < 0)
                    {
                        Media_Error("call PsFile_Write failed");
                        iRet = -1;
                        goto end;
                    }                  
                                        
                    audioStart += tHeader.aac_frame_length;
                    audioLeftSize -= tHeader.aac_frame_length;
                }
            }

        }else if(AV_CODEC_ID_ADPCM_IMA_APC == eACodecID)
        {
            Media_Debug("AV_CODEC_ID_ADPCM_IMA_APC");
            Media_Debug("pstFrameHeader->u16AFrames=%d", pstFrameHeader->u16AFrames);
            Media_Debug("pstFrameHeader->u16AFrameSize=%d", pstFrameHeader->u16AFrameSize);        
            for(i = 0; i < pstFrameHeader->u16AFrames; i++)
            {
                
                Media_Debug("i(%d) pcAData(%p) iAOffset(%d)", i, pcAData, iAOffset);                
                if(PsFile_Write(pPsFile, &stPsFrameInfo, pcAData, iAOffset) < 0)
                {
                    Media_Error("call PsFile_Write failed");
                    iRet = -1;
                    goto end;
                }
                pcAData = pcAData + iAOffset;                 
            }

        }
        else
        {
            Media_Error("not support audio");

        }
        
        #endif
                
        (void)usleep(1000);

    }

    if(NULL != pcDataBuf)
    {
        free(pcDataBuf);
        pcDataBuf = NULL;
    }

    if(NULL != pSdvFh)
    {
        (void)fclose(pSdvFh);
        pSdvFh = NULL;
    }

    if(NULL != pPsFile)
    {
        if(PS_Destroy(pPsFile) < 0)
        {
            iRet = -1;
            goto end;
        }
    }

end:
    return iRet;
}

int PsToSdv(int argc,char * * argv)
{
    void *pPsH = NULL;
    int iMaxFrameLen = 0;
    int64_t i64StartTime = 0;
    int64_t i64EndTime = 0;
    
    if(argc < 3)
    {
        Media_Error("argc(%d) < 4 ", argc);
        return -1;
    }

    if(1 != PsFile_IsPSFormat(argv[1]))
    {
        Media_Debug("%s is not ps file", argv[1]);
        return -1;
    }
    
    /*./multimedia.bin hk-h264-g711a.mp4 hk-h264-g711a_video.raw hk-h264-g711a_audio.raw*/
    
    pPsH = PsFile_Open(argv[1]);//"h264-8192bps-g711a-8k_1.ps"/*"h264-8192bps-25fps-25-g711a-8k_111.ps" "hk-h264-g711a.mp4"*/

    if(NULL == pPsH)
    {
        Media_Debug("invalid pPsH(%p)\n", pPsH);
        return -1;
    }
    iMaxFrameLen = PsFile_GetMaxFrameLength(pPsH);
    i64StartTime = PsFile_GetStartTime(pPsH);
    i64EndTime = PsFile_GetEndTime(pPsH);
    
    Media_Debug("iMaxFrameLen=%d", iMaxFrameLen);
    Media_Debug("i64StartTime=%lld", i64StartTime);
    Media_Debug("i64EndTime=%lld", i64EndTime);
    
#define OUT_BUFFER_SIZE     (1024*1024)
    PsFrameInfo stPsFrameInfo;
    char *pData = NULL;
    unsigned int uiLen = 0;
    
    char *pSdvData = NULL;
    unsigned int uiSdvLen = 0;

    pData = (char *)malloc(OUT_BUFFER_SIZE);
    uiLen = OUT_BUFFER_SIZE;

    pSdvData = (char *)malloc(2 * OUT_BUFFER_SIZE);

    struct iovec tmpaaa;
    S_FileHeader stFileHeader;
    S_FrameHeader stFrameHeader;
    int iMode = 0;
    int iAudioSample = 0;

    stFrameHeader.u32FrameID = 0x02000000;
    FILE *pSdv = NULL;
    int iLen = 0;
    int iFrameNO = 1;
    
    pSdv = fopen(argv[2], "w+");
    if(NULL == pSdv)
    {
        Media_Debug("fopen failed! pSdv(%p)\n", pSdv);
        return -1;
    }
    
    
    if(sizeof(stFileHeader) != fwrite(&stFileHeader, 1, sizeof(stFileHeader), pSdv))
    {
        Media_Debug("fwrite failed! stFileHeader");
        return -1;
    }
    
    while(1)
    {
        uiLen = OUT_BUFFER_SIZE;

        #if 0
        //seek
        if(uiSdvLen > 0)
        {
            int64_t i64SeekTime = 0;

            i64SeekTime = stPsFrameInfo.m_TimestampMs + 40LL;
            
            Media_Debug("stPsFrameInfo.m_TimestampMs=%lld i64SeekTime=%lld", stPsFrameInfo.m_TimestampMs, i64SeekTime);
            
            if(PsFile_Seek(pPsH, i64SeekTime) < 0)   
            {
                Media_Debug("call PsFile_Seek failed\n");
                return -1;
            }
        }
        #endif
        
        if(PsFile_Read(pPsH, &stPsFrameInfo, (unsigned char **)(&pData), &uiLen) < 0)    
        {
            Media_Debug("call PsFile_Read failed\n");
            break;
        }

        Media_Debug("m_eAVCodecID(%d)\n", stPsFrameInfo.m_eAVCodecID);
        Media_Debug("m_TimestampMs(%llu)\n", stPsFrameInfo.m_TimestampMs);
        Media_Debug("m_isSyncFrame(%d)\n", stPsFrameInfo.m_isSyncFrame);
        Media_Debug("uiLen(%d)\n", uiLen);

        tmpaaa.iov_base = pData;
        tmpaaa.iov_len = uiLen;

        if(AV_CODEC_ID_H264 == stPsFrameInfo.m_eAVCodecID)
        {
            if(uiSdvLen > 0)
            {
                if(sizeof(stFrameHeader) != fwrite(&stFrameHeader, 1, sizeof(stFrameHeader), pSdv))
                {
                    Media_Debug("fwrite failed! stFrameHeader");
                    return -1;
                }
                if(uiSdvLen != fwrite(pSdvData, 1, uiSdvLen, pSdv))
                {
                    Media_Debug("fwrite failed! stFrameHeader");
                    return -1;
                }
                
                uiSdvLen = 0;
            }

            
            WriteFileVideoRaw(argv[2],&tmpaaa, 1);
            Media_Debug("AV_CODEC_ID_H264");           
            
            memset(&stFrameHeader, 0, sizeof(stFrameHeader));
            stFrameHeader.u32FrameID = 0x02000000;
            stFrameHeader.u32FrameSize = sizeof(stFrameHeader) + tmpaaa.iov_len;
            stFrameHeader.u8FrameType = stPsFrameInfo.m_isSyncFrame == 0 ? 1: 0;
            stFrameHeader.u32FrameNO = iFrameNO++;
            stFrameHeader.u32VStreamLen = tmpaaa.iov_len;
            stFrameHeader.u32TimeStamp = stPsFrameInfo.m_TimestampMs;
            stFrameHeader.u16AFrames = 0;
            stFrameHeader.u16AFrameSize = 0;
            stFrameHeader.u8ACoder = 0;
            stFrameHeader.reserved = ~(0);

            memcpy(pSdvData + uiSdvLen, tmpaaa.iov_base, tmpaaa.iov_len);
            uiSdvLen += tmpaaa.iov_len;
            iMode = 21;//h264
        
        }else if(AV_CODEC_ID_HEVC == stPsFrameInfo.m_eAVCodecID)
        {
            if(uiSdvLen > 0)
            {
                if(sizeof(stFrameHeader) != fwrite(&stFrameHeader, 1, sizeof(stFrameHeader), pSdv))
                {
                    Media_Debug("fwrite failed! stFrameHeader");
                    return -1;

                }
                if(uiSdvLen != fwrite(pSdvData, 1, uiSdvLen, pSdv))
                {
                    Media_Debug("fwrite failed! stFrameHeader");
                    return -1;
                }
                
                uiSdvLen = 0;
            }

            
            WriteFileVideoRaw(argv[2],&tmpaaa, 1);          
            
            memset(&stFrameHeader, 0, sizeof(stFrameHeader));
            stFrameHeader.u32FrameID = 0x02000000;
            stFrameHeader.u32FrameSize = sizeof(stFrameHeader) + tmpaaa.iov_len;
            stFrameHeader.u8FrameType = stPsFrameInfo.m_isSyncFrame == 0 ? 1: 0;
            stFrameHeader.u32FrameNO = iFrameNO++;
            stFrameHeader.u32VStreamLen = tmpaaa.iov_len;
            stFrameHeader.u32TimeStamp = stPsFrameInfo.m_TimestampMs;
            stFrameHeader.u16AFrames = 0;
            stFrameHeader.u16AFrameSize = 0;
            stFrameHeader.u8ACoder = 0;
            stFrameHeader.reserved = ~(0);

            memcpy(pSdvData + uiSdvLen, tmpaaa.iov_base, tmpaaa.iov_len);
            uiSdvLen += tmpaaa.iov_len;
            
            iMode = 23;//h265
            WriteFileVideoRaw(argv[2],&tmpaaa, 1);
            Media_Debug("AV_CODEC_ID_HEVC");
        }else if(AV_CODEC_ID_AAC == stPsFrameInfo.m_eAVCodecID)
        {            
            #if 1
            stFrameHeader.u32FrameSize += tmpaaa.iov_len;
            stFrameHeader.u16AFrames = 1;
            stFrameHeader.u16AFrameSize += tmpaaa.iov_len;
            stFrameHeader.u8ACoder = 22;            
            
            memcpy(pSdvData + uiSdvLen, tmpaaa.iov_base, tmpaaa.iov_len);
            uiSdvLen += tmpaaa.iov_len;

            Media_Debug("aac aac aac aac aac aac aac aac aac aac aac aac aac aac aac ");
            PrintHexData(tmpaaa.iov_base, tmpaaa.iov_len, 32);
            WriteFileAudioRaw(argv[3],&tmpaaa, 1);
            Media_Debug("AV_CODEC_ID_AAC");
            #endif
        }else if(AV_CODEC_ID_PCM_ALAW == stPsFrameInfo.m_eAVCodecID)
        {
            stFrameHeader.u32FrameSize += tmpaaa.iov_len;
            stFrameHeader.u16AFrames++;
            stFrameHeader.u16AFrameSize = tmpaaa.iov_len;
            stFrameHeader.u8ACoder = 1;            
                       
            memcpy(pSdvData + uiSdvLen, tmpaaa.iov_base, tmpaaa.iov_len);
            uiSdvLen += tmpaaa.iov_len;
            
            WriteFileAudioRaw(argv[3],&tmpaaa, 1);
            if(324 == tmpaaa.iov_len)
            {
                Media_Debug("8k");
                iAudioSample = 8000;
            }else if(484 == tmpaaa.iov_len)
            {
                Media_Debug("32k");
                iAudioSample = 32000;
            }else if(484 == tmpaaa.iov_len)
            {
                Media_Debug("48k");
                iAudioSample = 48000;
            }
            
            Media_Debug("AV_CODEC_ID_PCM_ALAW");
        }else if(AV_CODEC_ID_PCM_MULAW == stPsFrameInfo.m_eAVCodecID)
        {
            stFrameHeader.u32FrameSize += tmpaaa.iov_len;
            stFrameHeader.u16AFrames++;
            stFrameHeader.u16AFrameSize = tmpaaa.iov_len;
            stFrameHeader.u8ACoder = 2;            
                       
            memcpy(pSdvData + uiSdvLen, tmpaaa.iov_base, tmpaaa.iov_len);
            uiSdvLen += tmpaaa.iov_len;
            
            WriteFileAudioRaw(argv[3],&tmpaaa, 1);
            if(324 == tmpaaa.iov_len)
            {
                Media_Debug("8k");
                iAudioSample = 8000;
            }else if(484 == tmpaaa.iov_len)
            {
                Media_Debug("32k");
                iAudioSample = 32000;
            }else if(484 == tmpaaa.iov_len)
            {
                Media_Debug("48k");
                iAudioSample = 48000;
            }
            
            Media_Debug("AV_CODEC_ID_PCM_ALAW");
        }else if(AV_CODEC_ID_ADPCM_IMA_APC == stPsFrameInfo.m_eAVCodecID)
        {
            stFrameHeader.u32FrameSize += tmpaaa.iov_len;
            stFrameHeader.u16AFrames++;
            stFrameHeader.u16AFrameSize = tmpaaa.iov_len;
            stFrameHeader.u8ACoder = 3;
            
            memcpy(pSdvData + uiSdvLen, tmpaaa.iov_base, tmpaaa.iov_len);
            uiSdvLen += tmpaaa.iov_len;
            
            WriteFileAudioRaw(argv[3],&tmpaaa, 1);
            if(168 == tmpaaa.iov_len)
            {
                Media_Debug("8k");
                iAudioSample = 8000;
            }else if(248 == tmpaaa.iov_len)
            {
                Media_Debug("32k");
                iAudioSample = 32000;
            }else if(248 == tmpaaa.iov_len)
            {
                Media_Debug("48k");
                iAudioSample = 48000;
            }
            Media_Debug("AV_CODEC_ID_ADPCM_IMA_APC");
        }else
        {
            WriteFileAudioRaw(argv[3],&tmpaaa, 1);
            Media_Debug("other");
        }
        
        
        Media_Debug("===================================================");
        
    }

    SetFileHeader(&stFileHeader, iMode, iAudioSample);
    if(fseek(pSdv, 0, SEEK_SET) < 0)
    {
        Media_Debug("fseek failed!");
        return -1;
    }
    
    if(sizeof(stFileHeader) != fwrite(&stFileHeader, 1, sizeof(stFileHeader), pSdv))
    {
        Media_Debug("fwrite failed! stFileHeader");
        return -1;
    }
    
    if(NULL != pData)
    {
        free(pData);
        pData = NULL;
    }

    if(NULL != pSdv)
    {
        fclose(pSdv);
        pSdv = NULL;
    }

    if(NULL != pSdvData)
    {
        free(pSdvData);
        pSdvData = NULL;
    }
    
    PsFile_Close(pPsH);

    return 0;
}

int main(int argc,char * * argv)
{   
    #if 0
    char *ptr1 = "Email";
    
    PrintHexData(ptr1,  strlen(ptr1), 16);
    PrintHexData(ptr1,  strlen(ptr1) + 1, 16);
    
    Media_Debug("==================");
    char *ptr = "wenminchen@126.com";

    PrintHexData(ptr,  strlen(ptr), 16);
    PrintHexData(ptr,  strlen(ptr) + 1, 16);

    return 0;
    #endif
    
    #if 0
    uint8_t marker = 0;
    uint8_t payloadtype = 0;
    uint16_t sequencenumber = 0;
    uint32_t timestamp = 0;
    uint32_t ssrc = 0;
    struct iovec Iovec[20];
    int _iIovNum = 20;

    char bufferArray[20][2000];
    int a = 0;
    int valude = 0;
    
    for(a = 0; a < 50; a++)
    {
        bufferArray[0][a] = valude++;
    }
    Iovec[0].iov_base = bufferArray[0];
    Iovec[0].iov_len = 50;
    
    for(a = 0; a < 50; a++)
    {
        bufferArray[1][a] = valude++;
    }
    Iovec[1].iov_base = bufferArray[1];
    Iovec[1].iov_len = 50;

    for(a = 0; a < 50; a++)
    {
        bufferArray[2][a] = valude++;
    }
    Iovec[2].iov_base = bufferArray[2];
    Iovec[2].iov_len = 50;
    
    for(a = 0; a < 50; a++)
    {
        bufferArray[3][a] = valude++;
    }
    Iovec[3].iov_base = bufferArray[3];
    Iovec[3].iov_len = 50;

    for(a = 0; a < 1; a++)
    {
        bufferArray[4][a] = valude++;
    }
    Iovec[4].iov_base = bufferArray[4];
    Iovec[4].iov_len = 1;

    for(a = 0; a < 2; a++)
    {
        bufferArray[5][a] = valude++;
    }
    Iovec[5].iov_base = bufferArray[5];
    Iovec[5].iov_len = 2;

    for(a = 0; a < 21; a++)//21
    {
        bufferArray[6][a] = valude++;
    }
    Iovec[6].iov_base = bufferArray[6];
    Iovec[6].iov_len = 21;

    for(a = 0; a < 32; a++)
    {
        bufferArray[7][a] = valude++;
    }
    Iovec[7].iov_base = bufferArray[7];
    Iovec[7].iov_len = 32;

    _iIovNum = 8;

    payloadtype = 0x3;
    sequencenumber = 0x8;
    timestamp = 0;
    ssrc = 0xbeef;
    
    NET_SendRtpByUdp(marker, payloadtype, &sequencenumber, timestamp, htonl(ssrc), Iovec, _iIovNum);
    Media_Debug("sequencenumber=%d", sequencenumber);
    return 0;
    #endif
    
    #if 0
    get_scr_ba_td();
    Media_Debug("============================");
    get_pts_e0_td();
    return 0;
    #endif
    #if 0
    get_scr_ba_hk();
    Media_Debug("============================");
    get_pts_e0_hk();
    return 0;
    #endif
    //ps_ba();
    //ps_bb();
    //ps_bc();
    //pes();

    #if 0
    int i = 0;
    for(i = 23; i < sizeof(data); i++)
    {
        data[i] = i;
    }
    #endif

    #if 0
    if(LiveStreamToPs(argc, argv) < 0)
    {
        Media_Error("call LiveStreamToPs failed");
    }

    //PsPackVideoH264( data, sizeof(data), 0, NULL);
    return 0;
    #endif

    #if 1
    if(SdvToPs(argc, argv) < 0)
    {
        Media_Error("call SdvToPs failed");
    }
    return 0;
    #endif
    #if 0
    Demo_Demux(argc, argv);
    #endif

    #if 1

    //ps to sdv
    if(PsToSdv(argc, argv) < 0)
    {
        Media_Error("call PsToSdv failed");
        return -1;
    }
    return 0;
    #endif
    
    
#if 0
    uint8_t pesBuffer[64] = {0};
    int iPaylodLen = 0;
    int iPayloadUsed = 0;
    int iPesHeaderLen = 0;
    uint64_t ullPts = 0;
    uint64_t ullDts = 0;
    int iStuffingByte = 0;
    char StuffingByte[16];

    iPaylodLen = 65527;
    iPesHeaderLen = Mpeg_PutPesHeader(pesBuffer, VIDEO_ID, PES_DATA_ALIGNMENT_INDICATOR_START, iPaylodLen, PES_PTS_DTS_0B10, ullPts, ullDts, iStuffingByte, (unsigned char *)StuffingByte);
    Media_Debug("iPesHeaderLen=%d\n", iPesHeaderLen);
    Media_Debug("iPesHeaderLen - 6=%d\n", iPesHeaderLen - 6);

    if(iPaylodLen >= (PES_MAX_LEN - (iPesHeaderLen - PES_HEADER_BASE_LEN)))
    {
        iPayloadUsed = PES_MAX_LEN - (iPesHeaderLen - PES_HEADER_BASE_LEN);
        Media_Debug("iPayloadUsed=%d\n", iPayloadUsed);
    }else
    {
        iPayloadUsed = iPaylodLen;
        Media_Debug("iPayloadUsed=%d\n", iPayloadUsed);
    }
    
    
    return 0;
#endif

}
#endif

#endif
#endif

