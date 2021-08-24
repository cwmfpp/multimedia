#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "media_avcodec.h"
#include "media_video_h264.h"
#include "media_video_h265.h"
#include "media_psmux.h"
#include "media_log.h"

//文件头结构
typedef struct
{
    unsigned short  FrameRate;
    unsigned short  Width;
    unsigned short  Height;
    unsigned short  Mode;  // 21(H264)
    unsigned short  bAudio;
    unsigned short  Year;
    unsigned short  Month;
    unsigned short  Day;
    unsigned short  Hour;
    unsigned short  Minute;
    unsigned short  Second;
    unsigned short  CoverMask;
    char cCovWord[36];
    unsigned int  m_uiFrameNoDiff;
    unsigned int  TriggerHigh;
    unsigned int  TriggerLow;
    unsigned short reserved;
    unsigned char   AChannels;
 	unsigned char   BitsPerSample;
    unsigned int  AudioSample;
    unsigned int    TotalSize;
    unsigned int    FrameCount;
}FileInfo;


typedef struct
{
    unsigned int    u32FrameID ;    //标示ID,32位,固定为: 0x 00 00 00 02
    unsigned int    u32FrameSize;   //帧大小32位,整个帧大小,字节数
    unsigned char   u8FrameType;    //帧类型,8位,FRAME_I=0,  FRAME_P=1, FRAME_B=2 
    unsigned int    u32FrameNO;     //帧序号,32位,帧序号,循环递增
    unsigned int    u32TimeStamp;   //时间戳,32位
    unsigned int    u32VStreamLen;  //视频数据长度
    unsigned short  u16AFrames;     //音频帧数,16位,音频帧数
    unsigned short  u16AFrameSize;  //音频帧大小,16位,音频帧大小
    unsigned char   u8ACoder;       //音频压缩方式,8位,不压缩:0;G711_A:0x01; G711_U:0x02;ADPCM_A :0x03;G726:0x04
    unsigned short  reserved;       //16位,保留,32位对齐(这里面附带帧率数据)
}FrameInfo;

static void ShowFrameHeader(FrameInfo *_pstHeader)
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

static AVCodecID AudioInfoProbe(unsigned char _u8ACoder, unsigned short _u16AFrameSize)
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
{//ISO/IEC 14496-3 ADTS����
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

    if (((unsigned char)(aac_buf[0]) == 0xFF)&&(((unsigned char)(aac_buf[1]) & 0xF0) == 0xF0))      //syncword 12��1
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
        Media_Error("Input parameters is error _pcframedata(%p) size(%d)!\n", _pcframedata, size);
        return -1;
    }
    
    audioStart = (char*)_pcframedata;
    audioLeftSize = size - sizeof(FrameInfo);
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

static int LiveStreamToPs(int argc, char ** argv)
{
    int iRet = 0;
    FILE *pSdvFh = NULL;
    FileInfo stFileHeader = {0};
    char *pcDataBuf = NULL;
    int iDataBufLen = 0;    
    char *pcFrameBuffer = NULL;
    int iFrameLen = 0;
    FrameInfo *pstFrameHeader = NULL;
    int iMediaDataLen = 0;
    
    PSHandle *pH = NULL;
    FrameOutInfo pstFrameOutInfo;

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


    if(fread((void *)(&stFileHeader), 1, sizeof(stFileHeader), pSdvFh) != sizeof(FileInfo))
    {
        Media_Error("call fread failed");
    }
    
    pcDataBuf = (char *)malloc(sizeof(FrameInfo));
    if(NULL == pcDataBuf)
    {
        Media_Error("call malloc failed");
        return -1;
    }
    iDataBufLen = (int)sizeof(FrameInfo);
    pstFrameHeader = (FrameInfo *)pcDataBuf;
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
        if(fread((void *)pstFrameHeader, 1, (size_t)sizeof(FrameInfo), pSdvFh) != (size_t)sizeof(FrameInfo))
        {
            if(feof(pSdvFh) != 0)
            {
                Media_Debug("feof end");
                break;
            }
            Media_Error("call fread failed");
        }
        iFrameLen += (int)sizeof(FrameInfo);
        if(0x02000000 != pstFrameHeader->u32FrameID)
        {
            Media_Error("0x02000000 != pstFrameHeader->u32FrameID(0x%x)", pstFrameHeader->u32FrameID);
            break;
        }
        
        iMediaDataLen = (int)pstFrameHeader->u32VStreamLen + (int)(pstFrameHeader->u16AFrames * pstFrameHeader->u16AFrameSize);
        if((iDataBufLen - (int)sizeof(FrameInfo)) < iMediaDataLen)
        {
            if(NULL != pcDataBuf)
            {
                FrameInfo stFrameHeader = {0};
                memcpy((void *)&stFrameHeader, pstFrameHeader, sizeof(FrameInfo));
                
                free(pcDataBuf);
                pcDataBuf = NULL;
                iDataBufLen = 0;
                
                iDataBufLen = (int)sizeof(FrameInfo) + iMediaDataLen;//4096
                iDataBufLen |= (0xfff);/*��ַҳ����*/
                iDataBufLen++;
                
                pcDataBuf = (char *)malloc((size_t)iDataBufLen);
                if(NULL == pcDataBuf)
                {
                    iDataBufLen = 0;
                    Media_Error("call malloc failed");
                    return -1;
                }
                pstFrameHeader = (FrameInfo *)pcDataBuf;
                pcFrameBuffer = pcDataBuf;
                
                memcpy(pstFrameHeader, (void *)&stFrameHeader, sizeof(FrameInfo));
            }
        }
        
        //Media_Debug("pstFrameHeader->u32FrameID(0x%x)", pstFrameHeader->u32FrameID);
        if(fread((void *)(pcFrameBuffer + sizeof(FrameInfo)), 1, (size_t)iMediaDataLen, pSdvFh) != (size_t)iMediaDataLen)
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

        iovecTmp.iov_base = pcFrameBuffer + sizeof(FrameInfo);
        iovecTmp.iov_len = pstFrameHeader->u32VStreamLen;
        if(WriteFileVideoRaw(argv[2], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
        {

        } 
        
        //audio raw
        ResetFrameOutInfo(&pstFrameOutInfo);

        pstFrameOutInfo.m_pstDI = &iovecTmp;
        pstFrameOutInfo.m_iDICount = 1;

        iovecTmp.iov_base = pcFrameBuffer + sizeof(FrameInfo) + pstFrameHeader->u32VStreamLen;
        iovecTmp.iov_len = pstFrameHeader->u16AFrames * pstFrameHeader->u16AFrameSize;
        if(WriteFileAudioRaw(argv[3], pstFrameOutInfo.m_pstDI, pstFrameOutInfo.m_iDICount) < 0)
        {

        }  
        
        #endif


        #if 1
        //video ps
        if(AV_CODEC_ID_NONE == eVCodecID)
        {
            if(H264_Probe((unsigned char *)pcFrameBuffer + sizeof(FrameInfo), (int)pstFrameHeader->u32VStreamLen) == 1)
            {
                eVCodecID = AV_CODEC_ID_H264;
                Media_Debug("AV_CODEC_ID_H264");
            }else if(H265_Probe((unsigned char *)pcFrameBuffer + sizeof(FrameInfo), (int)pstFrameHeader->u32VStreamLen) == 1)
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
                
        PsFrameInfo stPsFrameInfo;
        stPsFrameInfo.m_TimestampMs = pstFrameHeader->u32TimeStamp;
        stPsFrameInfo.m_isSyncFrame = (pstFrameHeader->u8FrameType == 0 ? 1 : 0);
    
        if(PS_PutVideoData(pH, pcFrameBuffer + sizeof(FrameInfo), (int)pstFrameHeader->u32VStreamLen, &stPsFrameInfo) < 0)
        {
            Media_Error("call PS_PutVideoData failed");
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
        pcAData = pcFrameBuffer + sizeof(FrameInfo) + pstFrameHeader->u32VStreamLen;
        iALen = pstFrameHeader->u16AFrameSize * pstFrameHeader->u16AFrames;
        iAOffset = pstFrameHeader->u16AFrameSize;

		iALen = 0;
		if (iALen <= 0) {
            Media_Error("invalid iALen(%d) continue", iALen);
			continue;
		}
        if(AV_CODEC_ID_PCM_ALAW == eACodecID || AV_CODEC_ID_PCM_MULAW == eACodecID)
        {            
            for(i = 0; i < pstFrameHeader->u16AFrames; i++)
            {
                pcAData = pcAData + i * iAOffset;
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


static void func(void)
{
#include <limits.h>
#include <stdlib.h>
	char *ptr = NULL;
	char buffer[PATH_MAX + 1] = {0};
	ptr = realpath("/mnt/d/work/project/multimedia/out/../../", buffer);
	Media_Error("ptr=%p", ptr);
	Media_Error("buffer=%s", buffer);
	Media_Error("==========================");

	
	ptr = realpath("/mnt/d/work/project/multimedia/out", buffer);
	Media_Error("ptr=%p", ptr);
	Media_Error("buffer=%s", buffer);
	Media_Error("==========================");
	
	ptr = realpath("/a/b/c/d/../", buffer);
	Media_Error("ptr=%p", ptr);
	Media_Error("buffer=%s", buffer);
	Media_Error("==========================");

}
int main(int argc, char **argv)
{
	func();
	LiveStreamToPs(argc, argv);
    return 0;
}
