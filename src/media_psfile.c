
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <errno.h>
#include <unistd.h>

#include <stdint.h>
#include <sys/uio.h>

#include "media_psmux.h"
#include "media_mpegdec.h"
#include "media_psfile.h"
#include "media_fileops.h"
//#include "media_video_h264.h"
//#include "media_video_h265.h"
#include "media_log.h"

typedef struct _PsFileWrite {
	PSHandle *m_psH;/*cwm:ps�����*/
	void *m_pvFH;/*cwm:ps�ļ����*/
	VideoEs m_stVideoEs;
	AudioEs m_stAudioEs;
} PsFileWrite;

static int ResetVideoEs(VideoEs *_pstVideoEs)
{
    int iRet = 0;
    
    if(NULL == _pstVideoEs)
    {
        Media_Error("invalid _pstVideoEs(%p)", _pstVideoEs);
        iRet = -1;
        goto end;
    }

    _pstVideoEs->m_eVCodecID = AV_CODEC_ID_NONE;
    _pstVideoEs->m_uiFrameRate = 25;
    _pstVideoEs->m_uiBitRate = 4096;
    _pstVideoEs->m_uiWidth = 1920;
    _pstVideoEs->m_uiHeight = 1080;
    _pstVideoEs->m_iIsSendSEI = 1;
    
end:
    return iRet;
}


static AVCodecID GetVideoEsCodecId(VideoEs *_pstVideoEs)
{
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    
    if(NULL == _pstVideoEs)
    {
        Media_Error("invalid _pstVideoEs(%p)", _pstVideoEs);
        goto end;
    }

    eAVCodecID = _pstVideoEs->m_eVCodecID;
    
end:
    
    return eAVCodecID;
}

static int SetVideoEsCodecId(VideoEs *_pstVideoEs, AVCodecID _eAVCodecID)
{
    int iRet = 0;
    
    if(NULL == _pstVideoEs)
    {
        Media_Error("invalid _pstVideoEs(%p)", _pstVideoEs);
        iRet = -1;
        goto end;
    }

    _pstVideoEs->m_eVCodecID = _eAVCodecID;
    
end:
    return iRet;
}

static int ResetAudioEs(AudioEs *_pstAudioEs)
{
    int iRet = 0;
    
    if(NULL == _pstAudioEs)
    {
        Media_Error("invalid _pstVideoEs(%p)", _pstAudioEs);
        iRet = -1;
        goto end;
    }

    _pstAudioEs->m_eACodecID = AV_CODEC_ID_NONE;
    _pstAudioEs->m_uiSampleRate = 8000;
    _pstAudioEs->m_uiBitRate = 256;
    
end:
    return iRet;
}

static AVCodecID GetAudioEsCodecId(AudioEs *_pstAudioEs)
{
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    
    if(NULL == _pstAudioEs)
    {
        Media_Error("invalid _pstAudioEs(%p)", _pstAudioEs);
        goto end;
    }

    eAVCodecID = _pstAudioEs->m_eACodecID;
    
end:
    return eAVCodecID;
}

static int SetAudioEsCodecId(AudioEs *_pstAudioEs, AVCodecID _eAVCodecID)
{
    int iRet = 0;
    
    if(NULL == _pstAudioEs)
    {
        Media_Error("invalid _pstAudioEs(%p)", _pstAudioEs);
        iRet = -1;
        goto end;
    }

    _pstAudioEs->m_eACodecID = _eAVCodecID;
    
end:
    return iRet;
}

static int InitPsFileWrite(PsFileWrite *_pPsFw, const char *_pFileName)
{
    int iRet = 0;
    PsFileWrite *pPsFw = NULL;

    if(NULL == _pPsFw || NULL == _pFileName)
    {
        Media_Error("invalid _pPsFw(%p) _pFileName(%p)", _pPsFw, _pFileName);
        iRet = -1;
        goto end;
    }

    pPsFw = _pPsFw;
    
    pPsFw->m_psH = PS_Create();
    if(NULL == pPsFw->m_psH)
    {
        Media_Error("call PS_Create failed!");
        iRet = -1;
        goto end;
    }

    if(ResetVideoEs(&pPsFw->m_stVideoEs) < 0)
    {
        Media_Error("call ResetVideoEs failed!");
        iRet = -1;
        goto end;
    }
    
    if(ResetAudioEs(&pPsFw->m_stAudioEs) < 0)
    {
        Media_Error("call ResetAudioEs failed!");
        iRet = -1;
        goto end;
    }
    
    pPsFw->m_pvFH = Media_Fopen(NULL, _pFileName, (const char *)"w+");
    if(NULL == pPsFw->m_pvFH)
    {
        Media_Error("call PS_Create failed!");
        iRet = -1;
        goto end;
    }
    
end:

    return iRet;
}

static int UninitPsFileWrite(PsFileWrite *_pPsFw)
{
    int iRet = 0;
    PsFileWrite *pPsFw = NULL;

    if(NULL == _pPsFw)
    {
        Media_Error("invalid _pPsFw(%p)", _pPsFw);
        iRet = -1;
        goto end;
    }

    pPsFw = _pPsFw;
    
    if(ResetVideoEs(&pPsFw->m_stVideoEs) < 0)
    {
        Media_Error("call ResetVideoEs failed!");
        iRet = -1;
        goto end;
    }
    
    if(ResetAudioEs(&pPsFw->m_stAudioEs) < 0)
    {
        Media_Error("call ResetAudioEs failed!");
        iRet = -1;
        goto end;
    }
    
    if(NULL != pPsFw->m_pvFH)
    {
        if(0 != Media_Fclose(pPsFw->m_pvFH))
        {
            Media_Error("call Media_Fclose failed!");
        }
        pPsFw->m_pvFH = NULL;
    }

    if(NULL != pPsFw->m_psH)
    {
        if(PS_Destroy(pPsFw->m_psH) < 0)
        {
            Media_Error("call PS_Destroy failed!");
            goto end;
        }            
        pPsFw->m_psH = NULL;
    }
    
end:

    return iRet;
}

int API_PUBLIC PsFile_IsPSFormat(char *_pFileName)
{
    int iRet = 0;

    if(NULL == _pFileName)
    {
        Media_Error("invalid _pFileName(%p)", _pFileName);
        goto end;
    }

    iRet = Mpeg_DecProbe(_pFileName);
    if(iRet < 0)
    {
        Media_Error("call Mpeg_DecProbe failed");
        iRet = 0;
    }
    
end:
    return iRet;
}


void* API_PUBLIC PsFile_Create(const char *_pFileName)
{
    void *pPsFW = NULL;
    PsFileWrite *pstPsFileWrite = NULL;
    int iRet = 0;
    
    if(NULL == _pFileName)
    {
        Media_Error("invalid _pFileName(%p)", _pFileName);
        goto end;
    }

    pstPsFileWrite = (PsFileWrite *)malloc(sizeof(PsFileWrite));
    if(NULL == pstPsFileWrite)
    {
        Media_Error("malloc failed!");
        goto end;
    }

    if(InitPsFileWrite(pstPsFileWrite, _pFileName) < 0)
    {
        Media_Error("call InitPsFileWrite failed!");
        iRet = -1;
        goto end;
    }
    
    pPsFW = pstPsFileWrite;
    
end:
    if(iRet < 0)
    {
        if(UninitPsFileWrite(pstPsFileWrite) < 0)
        {
            Media_Error("call UninitPsFileWrite failed!");
            goto end;
        }
        
        if(NULL != pstPsFileWrite)
        {
            free(pstPsFileWrite);
            pstPsFileWrite = NULL;
        }
    }
    
    return pPsFW;
}


int API_PUBLIC PsFile_SetAttribute(void *_pFile, FileAttribute *_pstFileAttribute)
{
    int iRet = 0;
    PsFileWrite *pPsFw = NULL;
    AVCodecID eVCodecID = AV_CODEC_ID_NONE;
    AVCodecID eACodecID = AV_CODEC_ID_NONE;

    if(NULL == _pFile || NULL == _pstFileAttribute)
    {
        Media_Error("invalid _pFile(%p) _pstFileAttribute(%p)", _pFile, _pstFileAttribute);
        iRet = -1;
        goto end;
    }

    pPsFw = (PsFileWrite *)_pFile;

    eVCodecID = GetVideoEsCodecId(&pPsFw->m_stVideoEs);
    eACodecID = GetAudioEsCodecId(&pPsFw->m_stAudioEs);
    if(_pstFileAttribute->m_eVCodecID != eVCodecID)
    {
        Media_Debug("video old eVCodecID(%d), new pstPsFrameInfo->m_eVCodecID=%d", eVCodecID, _pstFileAttribute->m_eVCodecID);
        if(SetVideoEsCodecId(&pPsFw->m_stVideoEs, _pstFileAttribute->m_eVCodecID) < 0)
        {
            Media_Error("call SetVideoEsCodecId failed!");
            iRet = -1;
            goto end;
        }
        
        if(PS_SetVideoEs(pPsFw->m_psH, &pPsFw->m_stVideoEs) < 0)
        {
            Media_Error("call PS_SetVideoEs failed");
            iRet = -1;
            goto end;
        }
    }
    
    if(_pstFileAttribute->m_eACodecID != eACodecID)
    {
        Media_Debug("audio old eACodecID(%d), new pstPsFrameInfo->m_eAVCodecID=%d", eACodecID, _pstFileAttribute->m_eACodecID);
        if(SetAudioEsCodecId(&pPsFw->m_stAudioEs, _pstFileAttribute->m_eACodecID) < 0)
        {
            Media_Error("call SetAudioEsCodecId failed!");
            iRet = -1;
            goto end;
        }
        Media_Debug("audio pstPsFrameInfo->m_eAVCodecID=%d", _pstFileAttribute->m_eACodecID);
        if(PS_SetAudioEs(pPsFw->m_psH, &pPsFw->m_stAudioEs) < 0)
        {
            Media_Error("call PS_SetAudioEs failed");
            iRet = -1;
            goto end;
        }
    }
    
    if(PS_SetStreamPrivateAttribute(pPsFw->m_psH, &_pstFileAttribute->m_stPsPriAttr) < 0)
    {
        Media_Error("call PS_SetStreamPrivateAttribute failed");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;

}

int API_PUBLIC PsFile_SetInsertHead(void *_pFile, PsInsertHead *_pstPsInsertHead)
{
    int iRet = 0;
    PsFileWrite *pPsFw = NULL;

    if(NULL == _pFile || NULL == _pstPsInsertHead)
    {
        Media_Error("invalid _pFile(%p) _pstPsInsertHead(%p)", _pFile, _pstPsInsertHead);
        iRet = -1;
        goto end;
    }

    pPsFw = (PsFileWrite *)_pFile;

    if(PS_SetInsertHead(pPsFw->m_psH, _pstPsInsertHead) < 0)
    {
        Media_Error("call PS_SetInsertHead failed");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;

}

//static unsigned short errno = 0;
#include <errno.h>

static int SafeWrite(void *_pvData, int _iSize, void * _pvFh)
{
    int n = 0;

    do {
        n = Media_Fwrite(_pvData, 1, (unsigned int)_iSize, _pvFh);
        Media_Error("write size n(%d)", n);
        if(n < 0)
        {
            Media_Error("call Media_Fwrite failed! continue n(%d)", n);
            if(ENOSPC == errno)
            {
                Media_Error("ENOSPC (No space left on device)");
                break;
            }
        }
    } while (n < 0 && (errno == EINTR ||  errno == EAGAIN));

    return n;
}

static int WriteData(void *_pvData, int _iSize,void *_pvFh)
{
    int iRet = 0;
    int iRetry = 0;
    int iLen = 0;
    int iReadSize = 0;/*Ҫд�������ܳ���*/
    char* pFileBuf = NULL;
    
    if(NULL == _pvData || NULL == _pvFh)
    {
        Media_Error("invalid _pvData(%p) _pvFh(%p)", _pvData, _pvFh);
        iRet = -1;
        goto end;
    }
    
    iRetry = 0;
    iLen = 0;
    iReadSize = _iSize;
    pFileBuf = (char*)_pvData;
    
    while(1)
    {
        iRet = SafeWrite(pFileBuf + iLen, iReadSize - iLen, _pvFh);
        if(iRet < 0)
        {
            Media_Error("call SafeWrite failed!");
            if(ENOSPC == errno)
            {
                Media_Error("ENOSPC (No space left on device)");
                iRet = -3;//no space
                goto end;
            }
        }
        if (iRet < (iReadSize - iLen))
        {
            Media_Warn("iRetry=%d iRet=%d iLen=%d iReadSize=%d", iRetry, iRet, iLen, iReadSize);
            iRetry++;
            if(iRetry < 10)
            {
                //USLEEP(USLEEP_100MS);
                usleep(100 * 1000);
                if(iRet > 0)
                {
                    iLen += iRet;
                }
                continue;
            }
            Media_Error("write failed! iReadSize=%d", iReadSize);
            iRet = -1;
        }

        
        Media_Debug("write end iReadSize=%d", iReadSize);
        break;
    }

end:

    return iRet;
}
static int WriteFileRawToPs(FrameOutInfo *_pstFrameOutInfo, void *_pvFH)
{
    int iRet = 0;
    int i = 0;
    int iCount = 0;    
    DataInfo *pstDataInfo = NULL;

    if(NULL == _pstFrameOutInfo || NULL == _pvFH)
    {
        Media_Error("invalid _pstFrameOutInfo(%p) _pvFH(%p)", _pstFrameOutInfo, _pvFH);
        iRet = -1;
        goto end;
    }

    iCount = _pstFrameOutInfo->m_iDICount;
    pstDataInfo = _pstFrameOutInfo->m_pstDI;
    
    for(i = 0; i < iCount; i++)
    {
        #if 1
        iRet = WriteData(pstDataInfo[i].m_pvAddr, (int)(pstDataInfo[i].m_uiLen), _pvFH);
        if(iRet < 0)
        {
            Media_Error("call WriteData failed!");
            break;
        }

        #else
        
        if((int)(pstDataInfo[i].m_uiLen) != Media_Fwrite(pstDataInfo[i].m_pvAddr, 1, (unsigned int)(pstDataInfo[i].m_uiLen), _pvFH))
        {
            Media_Error("call Media_Fwrite failed! pstDataInfo[%d].m_uiLen=%d", i, pstDataInfo[i].m_uiLen);
            iRet = -1;
            goto end;
        }

        #endif
    }

end:
    
    return iRet;
    
}


int API_PUBLIC PsFile_Write(void *_pFile, PsFrameInfo *_pstPsFrameInfo, char *_pcData, unsigned int _uiLen)
{
    int iRet = 0;
    int iFindCodecId = 0;
    PsFileWrite *pPsFw = NULL;
    PsFrameInfo *pstPsFrameInfo = NULL;
    FrameOutInfo pstFrameOutInfo;
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    
    if(NULL == _pFile || NULL == _pstPsFrameInfo || NULL == _pcData || 0 == _uiLen)
    {
        Media_Error("invalid _pFile(%p) _pstPsFrameInfo(%p) _pcData(%p) _uiLen(%d)", _pFile, _pstPsFrameInfo, _pcData, _uiLen);
        iRet = -1;
        goto end;
    }

    pPsFw = (PsFileWrite *)_pFile;
    pstPsFrameInfo = _pstPsFrameInfo;

    if(AV_CODEC_ID_H264 == pstPsFrameInfo->m_eAVCodecID || AV_CODEC_ID_HEVC == pstPsFrameInfo->m_eAVCodecID || AV_CODEC_ID_MJPEG == pstPsFrameInfo->m_eAVCodecID || AV_CODEC_ID_SVC == pstPsFrameInfo->m_eAVCodecID)
    {
        //video
        Media_Debug("video pstPsFrameInfo->m_eAVCodecID=%d", pstPsFrameInfo->m_eAVCodecID);
        iFindCodecId = 1;
        eAVCodecID = GetVideoEsCodecId(&pPsFw->m_stVideoEs);
        if(eAVCodecID != _pstPsFrameInfo->m_eAVCodecID)
        {
            Media_Debug("video old eAVCodecID(%d), new pstPsFrameInfo->m_eAVCodecID=%d", eAVCodecID, pstPsFrameInfo->m_eAVCodecID);
            if(SetVideoEsCodecId(&pPsFw->m_stVideoEs, pstPsFrameInfo->m_eAVCodecID) < 0)
            {
                Media_Error("call SetVideoEsCodecId failed!");
                iRet = -1;
                goto end;
            }
            
            if(PS_SetVideoEs(pPsFw->m_psH, &pPsFw->m_stVideoEs) < 0)
            {
                Media_Error("call PS_SetVideoEs failed");
                iRet = -1;
                goto end;
            }
        }
        
        if(PS_PutVideoData(pPsFw->m_psH, _pcData, (int)_uiLen, pstPsFrameInfo) < 0)
        {
            Media_Error("call PS_PutVideoData failed");
            iRet = -1;
            goto end;
        }

        do {        
            if(PS_GetVideoPackage(pPsFw->m_psH, &pstFrameOutInfo, NULL) < 0)
            {
                Media_Error("call PS_GetVideoPackage failed");
                iRet = -1;
                goto end;
            }

            iRet = WriteFileRawToPs(&pstFrameOutInfo, pPsFw->m_pvFH);
            if(iRet < 0)
            {
                Media_Error("call WriteFileRawToPs failed");
                goto end;
            }
            
        } while(1 != pstFrameOutInfo.m_isEnd);
    }
    
    if(AV_CODEC_ID_AAC == pstPsFrameInfo->m_eAVCodecID
        || AV_CODEC_ID_PCM_ALAW == pstPsFrameInfo->m_eAVCodecID
        || AV_CODEC_ID_PCM_MULAW == pstPsFrameInfo->m_eAVCodecID
        || AV_CODEC_ID_ADPCM_IMA_APC == pstPsFrameInfo->m_eAVCodecID
        || AV_CODEC_ID_SAC == pstPsFrameInfo->m_eAVCodecID)
    {
        //audio
        Media_Debug("audio pstPsFrameInfo->m_eAVCodecID=%d", pstPsFrameInfo->m_eAVCodecID);
        iFindCodecId = 1;
        eAVCodecID = GetAudioEsCodecId(&pPsFw->m_stAudioEs);
        if(eAVCodecID != _pstPsFrameInfo->m_eAVCodecID)
        {
            Media_Debug("audio old eAVCodecID(%d), new pstPsFrameInfo->m_eAVCodecID=%d", eAVCodecID, pstPsFrameInfo->m_eAVCodecID);
            if(SetAudioEsCodecId(&pPsFw->m_stAudioEs, pstPsFrameInfo->m_eAVCodecID) < 0)
            {
                Media_Error("call SetAudioEsCodecId failed!");
                iRet = -1;
                goto end;
            }
            Media_Debug("audio pstPsFrameInfo->m_eAVCodecID=%d", pstPsFrameInfo->m_eAVCodecID);
            if(PS_SetAudioEs(pPsFw->m_psH, &pPsFw->m_stAudioEs) < 0)
            {
                Media_Error("call PS_SetAudioEs failed");
                iRet = -1;
                goto end;
            }
        }
        
        if(PS_PutAudioData(pPsFw->m_psH, _pcData, (int)_uiLen, pstPsFrameInfo) < 0)
        {
            Media_Error("call PS_PutAudioData failed");
            iRet = -1;
            goto end;
        }

        do {        
            if(PS_GetAudioPackage(pPsFw->m_psH, &pstFrameOutInfo, NULL) < 0)
            {
                Media_Error("call PS_GetVideoPackage failed");
                iRet = -1;
                goto end;
            }

            iRet = WriteFileRawToPs(&pstFrameOutInfo, pPsFw->m_pvFH);
            if(iRet < 0)
            {
                Media_Error("call WriteFileRawToPs failed");
                goto end;
            }
            
        } while(1 != pstFrameOutInfo.m_isEnd);
        
    }

    if(0 == iFindCodecId)
    {
        Media_Error("not support pstPsFrameInfo->m_eAVCodecID(%d)", pstPsFrameInfo->m_eAVCodecID);
        iRet = -1;
        goto end;
    }

    
end:
    return iRet;
}


void API_PUBLIC PsFile_Destroy(void *_pFile)
{
    PsFileWrite *pPsFw = NULL;
    
    if(NULL == _pFile )
    {
        Media_Error("invalid _pFile(%p)  ", _pFile);
        goto end;
    }

    pPsFw = (PsFileWrite *)_pFile;
    
    if(UninitPsFileWrite(pPsFw) < 0)
    {
        Media_Error("call UninitPsFileWrite failed");
        goto end;
    }
    
    free(pPsFw);
    pPsFw = NULL;
    
end:
    return;
}

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
#endif

typedef struct _PsFileRead {
    MpegDec m_stMpegDec;
    AVMediaType m_eAVMediaTypePrev;
    int m_iStreamCount;
} PsFileRead;


void* API_PUBLIC PsFile_Open(char *_pFileName)
{
    PsFileRead *pstPsFileRead = NULL;

    if(NULL == _pFileName )
    {
        Media_Error("invalid _pFileName(%p)", _pFileName); 
        goto end;
    }

    pstPsFileRead = (PsFileRead *)malloc(sizeof(PsFileRead));

    if(NULL == pstPsFileRead )
    {
        Media_Error("malloc pstPsFileRead(%p) failed!", pstPsFileRead); 
        goto end;
    }
    (void)memset(pstPsFileRead, 0, sizeof(PsFileRead));

    if(Mpeg_DecInit(&pstPsFileRead->m_stMpegDec, _pFileName) < 0)
    {
        Media_Error("call Mpeg_DecInit failed!"); 
        free(pstPsFileRead);
        pstPsFileRead = NULL;
        goto end;
    }

    pstPsFileRead->m_eAVMediaTypePrev = AVMEDIA_TYPE_UNKNOWN;
    pstPsFileRead->m_iStreamCount = sizeof(pstPsFileRead->m_stMpegDec.m_stAVFormatContext.streams) / sizeof(pstPsFileRead->m_stMpegDec.m_stAVFormatContext.streams[0]);
    
end:
    return pstPsFileRead;
}

int API_PUBLIC PsFile_GetMaxFrameLength(void *_pFile)
{
    int iRet = 0;
    
    PsFileRead *pstPsFileRead = NULL;

    if(NULL == _pFile )
    {
        iRet = -1;
        Media_Error("invalid _pFile(%p) ", _pFile);
        goto end;
    }

    pstPsFileRead = (PsFileRead*)_pFile;

    iRet = Mpeg_DecMaxFrameLength(&pstPsFileRead->m_stMpegDec);
    if(iRet < 0)
    {
        Media_Error("call Mpeg_DecMaxFrameLength failed!");
        iRet = -1;
        goto end;
    }
    
end:

    return iRet;
}


PSTimestamp API_PUBLIC PsFile_GetStartTime(void *_pFile)
{
    PsFileRead *pstPsFileRead = NULL;
    PSTimestamp psTimestamp = 0;

    if(NULL == _pFile )
    {
        Media_Error("invalid _pFile(%p) ", _pFile); 
        goto end;
    }

    pstPsFileRead = (PsFileRead*)_pFile;

    psTimestamp = (PSTimestamp)((int)Mpeg_DecGetStartTime(&pstPsFileRead->m_stMpegDec));
    psTimestamp = psTimestamp / 90;
    
end:
    return psTimestamp;
}


PSTimestamp API_PUBLIC PsFile_GetEndTime(void *_pFile)
{
    PsFileRead *pstPsFileRead = NULL;
    PSTimestamp psTimestamp = 0;

    if(NULL == _pFile )
    {
        Media_Error("invalid _pFile(%p) ", _pFile); 
        goto end;
    }

    pstPsFileRead = (PsFileRead*)_pFile;

    psTimestamp = (PSTimestamp)((int)Mpeg_DecGetEndTime(&pstPsFileRead->m_stMpegDec));
    psTimestamp = psTimestamp / 90;
    
end:
    return psTimestamp;
}


static void ShowPsPrivateAttribute(PsPrivateAttribute *_pstPsPriAttr)
{
    Media_Debug("_pstPsPriAttr->m_usWidth=%d", _pstPsPriAttr->m_usWidth);
    Media_Debug("_pstPsPriAttr->m_usHeight=%d", _pstPsPriAttr->m_usHeight);
    Media_Debug("_pstPsPriAttr->m_uiSampleRate=%d", _pstPsPriAttr->m_uiSampleRate);
    Media_Debug("_pstPsPriAttr->m_usBits=%d", _pstPsPriAttr->m_usBits);
    _pstPsPriAttr = _pstPsPriAttr;
}

static int SetPsPrivateAttribute(PsPrivateAttribute *_pstPsPriAttr, PsInsertHead *_pstPsInsertHead)
{
    int iRet = 0;
    
    if(NULL == _pstPsInsertHead || NULL == _pstPsPriAttr)
    {
        Media_Error("invalid _pstPsInsertHead(%p) _pstPsPriAttr(%p)", _pstPsInsertHead, _pstPsPriAttr);
        iRet = -1;
        goto end;
    }
    
    _pstPsPriAttr->m_usWidth = _pstPsInsertHead->m_usWidth;
    _pstPsPriAttr->m_usHeight = _pstPsInsertHead->m_usHeight;
    _pstPsPriAttr->m_uiSampleRate = _pstPsInsertHead->m_uiSampleRate;
    _pstPsPriAttr->m_ucIsEncrypt = _pstPsInsertHead->m_ucIsEncrypt;
    _pstPsPriAttr->m_ucChannel = _pstPsInsertHead->m_ucChannel;
    _pstPsPriAttr->m_usBits = _pstPsInsertHead->m_usBits;

    ShowPsPrivateAttribute(_pstPsPriAttr);
    
end:
    
    return iRet;
}

int API_PUBLIC PsFile_GetAttribute(void *_pFile, FileAttribute *_pstFileAttribute)
{
    int iRet = 0;
    PsFileRead *pstPsFileRead = NULL;
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    char *ptrPirvate = NULL;
    int iPrivateLen = 0;
    char *pData = NULL;
    int iLen = 0;
    unsigned int uiFlag = PRIVATE_FLAG;
    PsInsertHead stPsInsertHead;

    if(NULL == _pFile || NULL == _pstFileAttribute)
    {
        Media_Error("invalid _pFile(%p) _pstFileAttribute(%p)", _pFile, _pstFileAttribute);
        iRet = -1;
        goto end;
    }

    pstPsFileRead = (PsFileRead*)_pFile;    

    eAVCodecID = Mpeg_DecGetVideoCodecId(&pstPsFileRead->m_stMpegDec);

    if(AV_CODEC_ID_NONE == eAVCodecID)
    {
        Media_Error("call Mpeg_DecGetVideoCodecId failed");
        iRet = -1;
        goto end;
    }
    _pstFileAttribute->m_eVCodecID = eAVCodecID;
    Media_Debug("_pstFileAttribute->m_eVCodecID=%d", _pstFileAttribute->m_eVCodecID);


    eAVCodecID = Mpeg_DecGetAudioCodecId(&pstPsFileRead->m_stMpegDec);

    if(AV_CODEC_ID_NONE == eAVCodecID)
    {
        Media_Error("call Mpeg_DecGetAudioCodecId failed");
        iRet = -1;
        goto end;
    }
    _pstFileAttribute->m_eACodecID = eAVCodecID;
    Media_Debug("_pstFileAttribute->m_eACodecID=%d", _pstFileAttribute->m_eACodecID);
    
    Media_Debug("Mpeg_DecGetProgramInfo");
    if(Mpeg_DecGetProgramInfo(&pstPsFileRead->m_stMpegDec, &pData, &iLen) < 0)
    {
        Media_Error("call Mpeg_DecGetProgramInfo failed");
        iRet = -1;
        goto end;
    }
        
    ptrPirvate = (char *)memmem((const void *)pData, (unsigned int)iLen, (const void*)(&uiFlag), sizeof(unsigned int));
    if(NULL == ptrPirvate)
    {
        Media_Error("invalid ptr(%p), not find private data, is not ps file, solve it yourself", ptrPirvate);
        iRet = -1;
        goto end;
    }
    

    iPrivateLen = iLen - (ptrPirvate - pData);
    Media_Debug("iPrivateLen=%d", iPrivateLen);

    if((unsigned int)iPrivateLen != sizeof(PsInsertHead))
    {
        Media_Error("actual iPrivateLen(%d) != now code sizeof(PsInsertHead)(%ld), use min", iPrivateLen, sizeof(PsInsertHead));
        if(iPrivateLen > sizeof(PsInsertHead))
        {
            iPrivateLen = sizeof(PsInsertHead);
            Media_Error("use iPrivateLen(%d)=sizeof(PsInsertHead)(%ld)", iPrivateLen, sizeof(PsInsertHead));
        }
    }
    
    memcpy(&stPsInsertHead, ptrPirvate, (unsigned int)iPrivateLen);

    if(SetPsPrivateAttribute(&_pstFileAttribute->m_stPsPriAttr, &stPsInsertHead) < 0)
    {
        Media_Error("call SetPsPrivateAttribute failed");
        iRet = -1;
        goto end;
    }
    
    #if 0
    Media_Debug("Mpeg_DecGetAudioESInfo");
    if(Mpeg_DecGetAudioESInfo(&pstPsFileRead->m_stMpegDec, &pData, &iLen) < 0)
    {
        Media_Error("call Mpeg_DecGetAudioESInfo failed");
        goto end;
    }

    
    Media_Debug("Mpeg_DecGetVideoESInfo");
    if(Mpeg_DecGetVideoESInfo(&pstPsFileRead->m_stMpegDec, &pData, &iLen) < 0)
    {
        Media_Error("call Mpeg_DecGetAudioESInfo failed");
        goto end;
    }

    #endif
    
end:
    return iRet;
}

int API_PUBLIC PsFile_GetInsertHead(void *_pFile, PsInsertHead *_pstPsInsertHead)
{
    int iRet = 0;
    PsFileRead *pstPsFileRead = NULL;
    char *ptrPirvate = NULL;
    int iPrivateLen = 0;
    char *pData = NULL;
    int iLen = 0;
    unsigned int uiFlag = PRIVATE_FLAG;
    PsInsertHead *pstPsInsertHeadSrc = NULL;
    
    if(NULL == _pFile || NULL == _pstPsInsertHead)
    {
        Media_Error("invalid _pFile(%p) _pstPsInsertHead(%p)", _pFile, _pstPsInsertHead);
        iRet = -1;
        goto end;
    }
    if(_pstPsInsertHead->m_uiSize <= 0)
    {
        Media_Error("invalid _pstPsInsertHead->m_uiSize(%d)", _pstPsInsertHead->m_uiSize);
        iRet = -1;
        goto end;
    }
    pstPsFileRead = (PsFileRead*)_pFile;    

    if(Mpeg_DecGetProgramInfo(&pstPsFileRead->m_stMpegDec, &pData, &iLen) < 0)
    {
        Media_Error("call Mpeg_DecGetProgramInfo failed");
        iRet = -1;
        goto end;
    }
        
    ptrPirvate = (char *)memmem(pData, (unsigned int)iLen, (const char*)(&uiFlag), sizeof(unsigned int)/*strlen(PRIVATE_FLAG)*/);
    if(NULL == ptrPirvate)
    {
        Media_Error("invalid ptr(%p), not find private data, is not ps file, solve it yourself", ptrPirvate);
        iRet = -1;
        goto end;
    }
    #if 1
    pstPsInsertHeadSrc = (PsInsertHead *)ptrPirvate;
    if(_pstPsInsertHead->m_uiSize <= pstPsInsertHeadSrc->m_uiSize)
    {
        iPrivateLen= _pstPsInsertHead->m_uiSize;
    }else
    {
        iPrivateLen= pstPsInsertHeadSrc->m_uiSize;
    }
    #else   
    iPrivateLen = iLen - (ptrPirvate - pData);
    #endif
    
    Media_Debug("iPrivateLen=%d", iPrivateLen);
    #if 0
    if((unsigned int)iPrivateLen != sizeof(PsInsertHead))
    {
        Media_Error("actual iPrivateLen(%d) != now code sizeof(PsInsertHead)(%d), solve it yourself", iPrivateLen, sizeof(PsInsertHead));
        iRet = -1;
        goto end;
    }
    #endif
    
    memcpy(_pstPsInsertHead, ptrPirvate, (unsigned int)iPrivateLen);
        
end:
    return iRet;
}


int API_PUBLIC PsFile_Read(void *_pFile, PsFrameInfo *_pstPsFrameInfo, unsigned char **_ppData, unsigned int *_puiLen)
{
    int iRet = 0;

    if(NULL == _pFile || NULL == _pstPsFrameInfo || NULL == _ppData || NULL == _puiLen)
    {
        Media_Error("invalid _pFile(%p) _pstPsFrameInfo(%p) _ppData(%p) _puiLen(%p)", _pFile, _pstPsFrameInfo, _ppData, _puiLen);
        iRet = -1;
        goto end;
    }

    if(NULL == *_ppData)
    {
        Media_Error("invalid *_ppData(%p)", *_ppData);
        iRet = -1;
        goto end;

    }
    
    if(NULL != *_ppData && 0 == *_puiLen)
    {
        Media_Error("invalid *_ppData(%p) *_puiLen(%d)", *_ppData, *_puiLen);
        iRet = -1;
        goto end;
    }
    
    PsFileRead *pstPsFileRead = NULL;
    MpegDec *pstMpegDec = NULL;
    AVFormatContext *pstAVFormatContext = NULL;
    //AVIOContext *pstAVIOContext = NULL;
    AVPacket *pstAVPacket = NULL;    
    int iStreamIndex = 0;
    int iStreamCount = 0;

    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    PSTimestamp timestamp = 0;
    unsigned char *pData = NULL;
    unsigned int uiDataLen = 0;
    unsigned int uiDataLenRaw = 0;
    int iIsSync = 0;
    int iFailedCount = 0;
	
    pstPsFileRead = (PsFileRead*)_pFile;
    pstMpegDec = &pstPsFileRead->m_stMpegDec;
    pstAVFormatContext = &pstMpegDec->m_stAVFormatContext;
    //pstAVIOContext = &pstMpegDec->m_stAVIOContext;
    pstAVPacket = &pstMpegDec->m_stAVPacket;
    
    
    iStreamIndex = pstAVPacket->stream_index;
    iStreamCount = pstPsFileRead->m_iStreamCount;

    if(iStreamIndex < 0 || iStreamIndex >= iStreamCount)
    {
        Media_Error("invalid iStreamIndex(%d) iStreamCount(%d)", iStreamIndex, iStreamCount);
        iRet = -1;
        goto end;
    }

    pData = *_ppData;
    uiDataLen = 0;
    uiDataLenRaw = *_puiLen;

    
    //AVMediaType eAVMediaTypePrev = AVMEDIA_TYPE_UNKNOWN;
    AVMediaType eAVMediaTypeCurr = AVMEDIA_TYPE_UNKNOWN;

    do {
        if(Mpeg_DecReadES(pstMpegDec, &iIsSync) < 0)
        {
            Media_Error("call Mpeg_DecReadES failed!");
            iFailedCount++;
            iFailedCount = iFailedCount;
            Media_Error("iFailedCount=%d pstAVPacket->size=%d uiDataLen=%d", iFailedCount, pstAVPacket->size, uiDataLen);
            if(0 == pstAVPacket->size)
            {
                if(0 == uiDataLen)
                {
                    Media_Error("read end file iFailedCount=%d pstAVPacket->size=%d uiDataLen=%d", iFailedCount, pstAVPacket->size, uiDataLen);
                    iRet = -1;
                    goto end;
                }else
                {
                    Media_Error("last frame iFailedCount=%d pstAVPacket->size=%d uiDataLen=%d", iFailedCount, pstAVPacket->size, uiDataLen);
                    break;
                }
            }
        }
        
        iStreamIndex = pstAVPacket->stream_index;            
        if(iStreamIndex < 0 || iStreamIndex >= iStreamCount)
        {
            Media_Error("invalid iStreamIndex(%d) iStreamCount(%d)", iStreamIndex, iStreamCount);
            iRet = -1;
            goto end;
        }            
        eAVMediaTypeCurr = pstAVFormatContext->streams[iStreamIndex].codecpar->codec_type;
        if(AV_CODEC_ID_NONE == eAVCodecID)
        {
            eAVCodecID = pstAVFormatContext->streams[iStreamIndex].codecpar->codec_id;
        }
        
        if(AV_NOPTS_VALUE != pstAVPacket->pts)
        {
            if(0 == timestamp)
            {
                timestamp = (PSTimestamp)pstAVPacket->pts;
            }
        }
        
        if(AVMEDIA_TYPE_AUDIO == eAVMediaTypeCurr)
        {
            //memcpy audio
            Media_Debug("memcpy audio data");
            Media_Debug("data(%p)", pstAVPacket->data);
            Media_Debug("size(%d)", pstAVPacket->size);
            if(AV_CODEC_ID_PCM_ALAW == eAVCodecID || AV_CODEC_ID_PCM_MULAW == eAVCodecID)
            {
                if(80 == pstAVPacket->size)
                {
                    unsigned char ucG711Header[4] = {0x00,0x01,0x28,0x00};
                    if((uiDataLen + sizeof(ucG711Header)) > uiDataLenRaw)
                    {
                        Media_Error("uiDataLen(%d) + sizeof(ucG711Header)(%ld) > uiDataLenRaw(%d)", uiDataLen, sizeof(ucG711Header), uiDataLenRaw);
                        iRet = -1;
                        goto end;
                    }
                    memcpy(pData + uiDataLen, ucG711Header, sizeof(ucG711Header));
                    uiDataLen += sizeof(ucG711Header);
                }else if(320 == pstAVPacket->size)
                {
                    unsigned char ucG711Header[4] = {0x00,0x01,0xa0,0x00};
                    if((uiDataLen + sizeof(ucG711Header)) > uiDataLenRaw)
                    {
                        Media_Error("uiDataLen(%d) + sizeof(ucG711Header)(%ld) > uiDataLenRaw(%d)", uiDataLen, sizeof(ucG711Header), uiDataLenRaw);
                        iRet = -1;
                        goto end;
                    }
                    memcpy(pData + uiDataLen, ucG711Header, sizeof(ucG711Header));
                    uiDataLen += sizeof(ucG711Header);
                }else if(480 == pstAVPacket->size)
                {
                    //0001F000
                    //0001F000
                    //0001F000
                    //0001F000
                    unsigned char ucG711Header[4] = {0x00,0x01,0xf0,0x00};
                    if((uiDataLen + sizeof(ucG711Header)) > uiDataLenRaw)
                    {
                        Media_Error("uiDataLen(%d) + sizeof(ucG711Header)(%ld) > uiDataLenRaw(%d)", uiDataLen, sizeof(ucG711Header), uiDataLenRaw);
                        iRet = -1;
                        goto end;
                    }
                    memcpy(pData + uiDataLen, ucG711Header, sizeof(ucG711Header));
                    uiDataLen += sizeof(ucG711Header);
                }else
                {
                    #if 0
                    unsigned char ucG711Header[4] = {0x00,0x01,0x28,0x00};
                    if((uiDataLen + sizeof(ucG711Header)) > uiDataLenRaw)
                    {
                        Media_Error("uiDataLen(%d) + sizeof(ucG711Header)(%d) > uiDataLenRaw(%d)", uiDataLen, sizeof(ucG711Header), uiDataLenRaw);
                        iRet = -1;
                        goto end;
                    }
                    memcpy(pData + uiDataLen, ucG711Header, sizeof(ucG711Header));
                    uiDataLen += sizeof(ucG711Header);
                    #endif
                }

            }
            
            if((uiDataLen + (unsigned int)(pstAVPacket->size)) > uiDataLenRaw)
            {
                Media_Error("uiDataLen(%d) + pstAVPacket->size(%d) > uiDataLenRaw(%d)", uiDataLen, pstAVPacket->size, uiDataLenRaw);
                iRet = -1;
                goto end;
            }
            memcpy(pData + uiDataLen, pstAVPacket->data, (unsigned int)(pstAVPacket->size));
            uiDataLen += (unsigned int)(pstAVPacket->size);
            break;
        }else if(AVMEDIA_TYPE_VIDEO == eAVMediaTypeCurr)
        {
            //memcpy video
            if(1 == pstAVFormatContext->m_iVideoEnd)
            {
                //video end
                Media_Debug("video end");
                break;
            }
            if((uiDataLen + (unsigned int)(pstAVPacket->size)) > uiDataLenRaw)
            {
                Media_Error("uiDataLen(%d) + pstAVPacket->size(%d) > uiDataLenRaw(%d)", uiDataLen, pstAVPacket->size, uiDataLenRaw);
                iRet = -1;
                goto end;
            }
            memcpy(pData + uiDataLen, pstAVPacket->data, (unsigned int)(pstAVPacket->size));
            uiDataLen += (unsigned int)(pstAVPacket->size);
            Media_Debug("memcpy video data");
            Media_Debug("data(%p)", pstAVPacket->data);
            Media_Debug("size(%d)", pstAVPacket->size);
            continue;
        }else 
        {
            //break invalid data
            Media_Debug("invalid data");
            break;
        }
    } while(uiDataLenRaw > 0);


    *_puiLen = uiDataLen;
    _pstPsFrameInfo->m_eAVCodecID = eAVCodecID;
    _pstPsFrameInfo->m_TimestampMs = timestamp / 90/*90000*/;
    _pstPsFrameInfo->m_isSyncFrame = iIsSync;
    Media_Debug("_pstPsFrameInfo->m_isSyncFrame=%d", _pstPsFrameInfo->m_isSyncFrame);
    Media_Debug("_pstPsFrameInfo->m_eAVCodecID=%d", _pstPsFrameInfo->m_eAVCodecID);
    
end:
    return iRet;
}



int API_PUBLIC PsFile_Seek(void *_pFile, PSTimestamp _TimestampMs)
{
    int iRet = 0;
    PsFileRead *pstPsFileRead = NULL;

    if(NULL == _pFile)
    {
        Media_Error("invalid _pFile(%p)", _pFile);
        goto end;
    }

    pstPsFileRead = (PsFileRead *)_pFile;

    if(Mpeg_DecSeek(&pstPsFileRead->m_stMpegDec, (int64_t)(_TimestampMs * 90LL)) < 0)
    {
        Media_Error("call Mpeg_DecSeek failed! _TimestampMs(%llu)", _TimestampMs);
        iRet = -1;
        goto end;
    }

    _TimestampMs = _TimestampMs;

end:
    return iRet;
}

void API_PUBLIC PsFile_Close(void *_pFile)
{
    PsFileRead *pstPsFileRead = NULL;
    
    if(NULL == _pFile)
    {
        Media_Error("invalid _pFile(%p)", _pFile);
        goto end;
    }

    pstPsFileRead = (PsFileRead *)_pFile;
    
    if(Mpeg_DecUninit(&pstPsFileRead->m_stMpegDec) < 0)
    {
        Media_Error("call Mpeg_DecUninit failed");
    }
    
    free(pstPsFileRead);
    pstPsFileRead = NULL;

end:
    return;
}


