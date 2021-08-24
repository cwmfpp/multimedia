
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>


#include "media_psdemux.h"
#include "vlc_bits.h"
#include "media_mpegenc.h"
#include "media_avcodec.h"
#include "media_log.h"


typedef struct MpegPsm {
    unsigned char m_pcPsmEsType[256];
    int m_iVideoType;
    int m_iAudioType;
} MpegPsm;

typedef struct PesHeader
{
    int m_iPesLen;
    int64_t m_i64Pts;
    int m_iPesHeaderLen;
}PesHeader;

typedef struct PsDataBuffer{
    char m_pcPsDataBuffer[64*1024];
    int m_iPsDataIndex;
}PsDataBuffer;

typedef struct PsDataParse {
    MpegPsm m_stMpegPsm;
    PesHeader m_stPesHeader;
    PsDataBuffer m_stPsDataBuffer;
    int m_iIsPsm;
    int m_iEsId;//strartCode
    int m_iEsLen;
    int m_iEsLeftLen;
    int m_iVideoLen;
    int m_iAudioLen;
    int m_iAudioCount;
    char *m_pcRawData;
    int m_iRawDataMaxSize;
    int m_iRawDataWriteIndex;
    int m_iFlag;
    int m_iReadIndex;
} PsDataParse;

#   define AV_RB8(x)                           \
        (((const uint8_t*)(x))[1])


#   define AV_RB16(x)                           \
        ((((const uint8_t*)(x))[0] << 8) |          \
          ((const uint8_t*)(x))[1])


#   define AV_RB32(x)                                \
        (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
                   (((const uint8_t*)(x))[1] << 16) |    \
                   (((const uint8_t*)(x))[2] <<  8) |    \
                    ((const uint8_t*)(x))[3])



static void PrintHexData(unsigned char *_pData, int _pDataLen, int _displayCount)
{
    return;
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

static int FindNextStartCode(unsigned char *_pucData, int *_iDataSize, int32_t *_piHeaderState)
{
    unsigned int state = 0;
    unsigned int v = 0;
    int val = 0;
    int n = 0;
    unsigned char *pucData = NULL;

    if(NULL == _pucData || NULL == _iDataSize || NULL == _piHeaderState)
    {
        Media_Error("invalid _pucData(%p) _iDataSize(%p) _piHeaderState(%p)", _pucData, _iDataSize, _piHeaderState);
        val = -1;
        goto found;
    }
    
    pucData = _pucData;
    state = *_piHeaderState;
    n     = *_iDataSize;
    while (n > 0)
    {        
        v = *(pucData++);
        n--;
        if (state == 0x000001)
        {
            state = ((state << 8) | v) & 0xffffff;
            val   = state;
            if(n >= 2 && 0 == pucData[0] && 0 == pucData[1])
            {
                Media_Error("invalid val(0x%x) pucData[0]=0x%x pucData[1]=0x%x , continue find start", val, pucData[0], pucData[1]);
                continue;
            }
            goto found;
        }
        state = ((state << 8) | v) & 0xffffff;
    }
    val = -1;

found:
    *_piHeaderState = state;
    *_iDataSize     = n;
    return val;
}

static long MpegpsPsmParse(MpegPsm *_pstMpegPsm, char *_pcData, int _iDataLen)
{
    int iPsmLength = 0;
    int iPsInfoLength = 0;
    int iEsMapLength = 0;

    bs_t pb;

    bs_init(&pb, _pcData, _iDataLen);

    if(NULL == _pstMpegPsm || NULL == _pcData || _iDataLen < 10)
    {
        Media_Error("invalid _pstMpegPsm(%p) _pcData(%p) _iDataLen(%d)", _pstMpegPsm, _pcData, _iDataLen);
        goto end;
    }

    iPsmLength = bs_read(&pb, 16);
    Media_Debug("iPsmLength=%d", iPsmLength);
    bs_read(&pb, 8);
    bs_read(&pb, 8);
    iPsInfoLength = bs_read(&pb, 16);

    /* skip program_stream_info */
    bs_skip(&pb, iPsInfoLength * 8);
    /*iEsMapLength = */bs_read(&pb, 16);
    /* Ignore iEsMapLength, trust iPsmLength */
    iEsMapLength = iPsmLength - iPsInfoLength - 10;

    /* at least one es available? */
    while (iEsMapLength >= 4) {
        unsigned char type      = bs_read(&pb, 8);/*cwm:0x1b*/
        unsigned char es_id     = bs_read(&pb, 8);/*cwm:0xe0*/
        uint16_t es_info_length = bs_read(&pb, 16);

        Media_Debug("type=0x%x", type);
        Media_Debug("es_id=0x%x", es_id);
        if(0xe0 == es_id)
        {
            _pstMpegPsm->m_iVideoType = type;
        }
        if(0xc0 == es_id)
        {
            _pstMpegPsm->m_iAudioType = type;
        }
        /* remember mapping from stream id to stream type */
        _pstMpegPsm->m_pcPsmEsType[es_id] = type;
        /* skip program_stream_info */
        bs_skip(&pb, es_info_length * 8);
        iEsMapLength -= 4 + es_info_length;
    }
    bs_read(&pb, 32); /* crc32 */

end:
    return 2 + iPsmLength;
}


static inline int64_t ParsePesPts(const uint8_t *buf) {
    return (int64_t)(*buf & 0x0e) << 29 |
            (AV_RB16(buf+1) >> 1) << 15 |
             AV_RB16(buf+3) >> 1;
}


static int64_t GetPesPts(bs_t *_pBs_t, int c)
{
    uint8_t buf[5] = {0};

    if(NULL == _pBs_t)
    {
        Media_Error("invalid _pBs_t(%p)", _pBs_t);
        return -1;
    }
    
    buf[0] = c < 0 ? bs_read(_pBs_t, 8) : c;
    buf[1] = bs_read(_pBs_t, 8);
    buf[2] = bs_read(_pBs_t, 8);
    buf[3] = bs_read(_pBs_t, 8);
    buf[4] = bs_read(_pBs_t, 8);
    
    return ParsePesPts(buf);
}

static int ParsePesHeader(char *_pcData, int _iDataLen, PesHeader *_pstPesHeader)
{
    int iRet = 0;
    int iPesLen = 0;
    int iPesLenRaw = 0;
    int iStartcode = 0;
    int c = 0;
    int iFlags = 0;
    int iHeaderLen = 0;
    int iPesExt = 0;
    int iExt2Len = 0;
    int iIdExt = 0;
    int iSkip = 0;
    int64_t i64Pts = 0;
    int64_t i64Dts = 0;    
    bs_t pb;

    if(NULL == _pcData || NULL == _pstPesHeader || _iDataLen < 10)
    {
        Media_Error("invalid _pcData(%p) _pstPesHeader(%p) _iDataLen(%d)", _pcData, _pstPesHeader, _iDataLen);
        iRet = -1;
        goto end;
    }    

    bs_init(&pb, _pcData, _iDataLen);    
    
    iPesLen = bs_read(&pb, 16);//PES_packet_length
    if(iPesLen < 0)
    {
        Media_Error("iPesLen(%d) < 0", iPesLen);
        iRet = -1;
        goto end;
    }
    
    iPesLenRaw = iPesLen;
    
    /* stuffing */
    for (;;) 
    {
        if(iPesLen < 0)
        {
            Media_Error("iPesLen(%d) < 0", iPesLen);
            iRet = -1;
            goto end;
        }
        c = bs_read(&pb, 8);/*cwm:2bit reserved +2bit PES_scrambling_control + 1bit PES_priority + 1bit data_alignment_indicator + 1bit copyright + 1bit original_or_copy*/
        iPesLen--;/*cwm:ȥ���Ѷ�һ���ֽڳ���*/
        /* XXX: for MPEG-1, should test only bit 7 */
        if (c != 0xff)
        {
            //Media_Debug("c(0x%x) != 0xff", c);
            break;
        }
    }
    
    if ((c & 0xc0) == 0x40)
    {
        /* buffer scale & iSize */
        /*cwm:��mpeg 2*/
        bs_read(&pb, 8);
        c    = bs_read(&pb, 8);
        iPesLen -= 2;
    }
    
    if ((c & 0xe0) == 0x20)
    {
        /*cwm:��mpeg 2*/
        i64Dts  =
        i64Pts  = GetPesPts(&pb, c);
        iPesLen -= 4;
        if (c & 0x10)
        {
            i64Dts  = GetPesPts(&pb, -1);
            iPesLen -= 5;
        }
    } else if ((c & 0xc0) == 0x80)
    {
        /*cwm:mpeg 2*/
        /* mpeg 2 PES */
        iFlags      = bs_read(&pb, 8);/*cwm:2bit PTS_DTS_flags + 1bit ESCR_flag + 1bit ES_rate_flag + 1bit DSM_trick_mode_flag + 1bit additional_copy_info_flag + 1bit PES_CRC_flag + 1bit PES_extension_flag*/
        iHeaderLen = bs_read(&pb, 8);/*cwm:8bit PES_header_data_length*/
        iPesLen       -= 2;
        if (iHeaderLen > iPesLen)
        {
            Media_Error("iHeaderLen(%d) > iPesLen(%d)", iHeaderLen, iPesLen);
            iRet = -1;
            goto end;
        }
        iPesLen -= iHeaderLen;
        /*cwm:��ȡʱ���*/
        if (iFlags & 0x80)
        {
            i64Dts = i64Pts = GetPesPts(&pb, -1);
            iHeaderLen -= 5;
            if (iFlags & 0x40)
            {
                i64Dts = GetPesPts(&pb, -1);
                iHeaderLen -= 5;
            }
        }
        if (iFlags & 0x3f && iHeaderLen == 0)
        {
            iFlags &= 0xC0;
            Media_Error("Further iFlags set but no bytes left\n");
        }
        if (iFlags & 0x01)
        {   /* PES extension */
            iPesExt = bs_read(&pb, 8);
            iHeaderLen--;
            /* Skip PES private data, program packet sequence counter
                    * and P-STD buffer */
            iSkip  = (iPesExt >> 4) & 0xb;
            iSkip += iSkip & 0x9;
            if (iPesExt & 0x40 || iSkip > iHeaderLen)
            {
                Media_Error("iPesExt %X is invalid\n", iPesExt);
                iPesExt = iSkip = 0;
            }
            bs_skip(&pb, iSkip * 8);
            iHeaderLen -= iSkip;
    
            if (iPesExt & 0x01)
            { /* PES extension 2 */
                iExt2Len = bs_read(&pb, 8);
                iHeaderLen--;
                if ((iExt2Len & 0x7f) > 0)
                {
                    iIdExt = bs_read(&pb, 8);
                    if ((iIdExt & 0x80) == 0)
                    {
                        iStartcode = ((iStartcode & 0xff) << 8) | iIdExt;
                    }
                    iHeaderLen--;
                }
            }
        }
        if (iHeaderLen < 0)
        {
            Media_Error("iHeaderLen(%d) < 0", iHeaderLen);
            iRet = -1;
            goto end;
        }
        
        bs_skip(&pb, iHeaderLen * 8);
    } else if (c != 0xf)
    {
        Media_Error("c(0x%x) != 0xf", c);
        iRet = -1;
        goto end;
    }
    
    //Media_Error("iPesLen(%d)", iPesLen);
    //Media_Error("i64Pts(%lld)", i64Pts);
    //Media_Error("i64Dts(%lld)", i64Dts);

    _pstPesHeader->m_iPesLen = iPesLen;
    if(i64Pts > 0 && i64Dts > 0)
    {
        _pstPesHeader->m_i64Pts = i64Pts;
    }
    _pstPesHeader->m_iPesHeaderLen = iPesLenRaw - iPesLen + 2;
    
end:
    return iRet;
}

static int PsDataParseReset(PsDataParse *_pstPsDataParse)
{
    int iRet = 0;
    
    if(NULL == _pstPsDataParse)
    {
        Media_Error("invalid _pstPsDataParse(%p)", _pstPsDataParse);
        iRet = -1;
        goto end;
    }
    
    _pstPsDataParse->m_iIsPsm = 0;    
    _pstPsDataParse->m_iEsId = 0;
    _pstPsDataParse->m_iEsLen = 0;
    _pstPsDataParse->m_iEsLeftLen = 0;
    _pstPsDataParse->m_iVideoLen = 0;
    _pstPsDataParse->m_iAudioLen = 0;
    _pstPsDataParse->m_iAudioCount = 0;
    _pstPsDataParse->m_iRawDataWriteIndex = 0;//;
    
end:
    return iRet;
}

static int PsDataParseMallocData(PsDataParse *_pstPsDataParse, int _iRawDataSize)
{
    int iRet = 0;
    if(NULL == _pstPsDataParse || _iRawDataSize <= 0)
    {
        iRet = -1;
        Media_Debug("invalid _pstPsDataParse(%p) _iRawDataSize(%d)", _pstPsDataParse, _iRawDataSize);
        goto end;
    }
    
    if(NULL != _pstPsDataParse->m_pcRawData)
    {
        free(_pstPsDataParse->m_pcRawData);
        _pstPsDataParse->m_pcRawData = NULL;
    }
    _pstPsDataParse->m_pcRawData = (char *)malloc(_iRawDataSize);
    if(NULL == _pstPsDataParse->m_pcRawData)
    {
        iRet = -1;
        Media_Debug("call malloc failed, _iRawDataSize(%d)", _iRawDataSize);
        goto end;
    }
    _pstPsDataParse->m_iRawDataMaxSize = _iRawDataSize;
    
end:
    return iRet;

}

static int PsDataParsePushData(PsDataParse *_pstPsDataParse, char *_pcData, int _iDataLen)
{
    int iRet = 0;
//    Media_Debug("_pstPsDataParse->m_iRawDataMaxSize=%d", _pstPsDataParse->m_iRawDataMaxSize);

    if(NULL == _pstPsDataParse || NULL == _pcData || _iDataLen < 0)
    {
        iRet = -1;
        Media_Debug("_pstPsDataParse(%p) _pcData(%p) _iDataLen=(%d)", _pstPsDataParse, _pcData, _iDataLen);
        goto end;
    }
    
    if((_pstPsDataParse->m_iRawDataMaxSize - _pstPsDataParse->m_iRawDataWriteIndex) < _iDataLen)
    {
        Media_Error("(_pstPsDataParse->m_iRawDataMaxSize(%d) - _pstPsDataParse->m_iRawDataWriteIndex(%d)) < _iDataLen(%d)", _pstPsDataParse->m_iRawDataMaxSize, _pstPsDataParse->m_iRawDataWriteIndex, _iDataLen);
        if(_pstPsDataParse->m_iRawDataMaxSize > _iDataLen)
        {
            PsDataParseReset(_pstPsDataParse);
        }else
        {
            #define MIN_RAW_DATA_FRAME    (1 * 1024 * 1024)
            #define MAX_RAW_DATA_FRAME    (2 * 1024 * 1024)
            int iRawDataFrameSize = 0;

            if((_pstPsDataParse->m_iRawDataMaxSize + _iDataLen) < MIN_RAW_DATA_FRAME)
            {
                iRawDataFrameSize = MIN_RAW_DATA_FRAME;
            }else if((_pstPsDataParse->m_iRawDataMaxSize + _iDataLen) <= MAX_RAW_DATA_FRAME)
            {
                iRawDataFrameSize = MAX_RAW_DATA_FRAME;
            }
            
            Media_Debug("iRawDataFrameSize=%d", iRawDataFrameSize);
            if(PsDataParseMallocData(_pstPsDataParse, iRawDataFrameSize) < 0)
            {
                Media_Error("call PsDataParseMallocData failed, iRawDataFrameSize(%d)", iRawDataFrameSize);
                iRet = -1;
                goto end;
            }
        }
    }
    if(NULL == _pstPsDataParse->m_pcRawData)
    {
        Media_Error("invalid _pstPsDataParse->m_pcRawData(%p)", _pstPsDataParse->m_pcRawData);
        iRet = -1;
        goto end;
    }
    memcpy(_pstPsDataParse->m_pcRawData + _pstPsDataParse->m_iRawDataWriteIndex, _pcData, _iDataLen);
    if(0x1e0 == _pstPsDataParse->m_iEsId)
    {
        _pstPsDataParse->m_iVideoLen += _iDataLen;
        Media_Debug("video _pstPsDataParse->m_iEsId=0x%x", _pstPsDataParse->m_iEsId);
    }else if(0x1c0 == _pstPsDataParse->m_iEsId)
    {
        _pstPsDataParse->m_iAudioLen += _iDataLen;
        Media_Debug("audio _pstPsDataParse->m_iEsId=0x%x", _pstPsDataParse->m_iEsId);
    }else
    {
        Media_Debug("esiderror _pstPsDataParse->m_iEsId=0x%x", _pstPsDataParse->m_iEsId);
    }
    _pstPsDataParse->m_iRawDataWriteIndex += _iDataLen;
    
end:
    return iRet;
}

static int HandlePsStream(PsDataParse *_pstPsDataParse, char *_pcData, int _iDataLen, void *_pvPri)
{
    char *pcData = NULL;
    int iDataLen = 0;/*���ݳ��ȣ��м䲻�޸�*/
    int iDataIndex = 0;/*������*/
    int iDataLeftLen = 0;/*ʣ�����ݳ���*/
    int iTmp = 0;
    int iFindOffset = 0;
    int iHeaderState = 0xff;
    int iStartCode = 0;
    
    pcData = _pcData;
    iDataLen = _iDataLen;
    iDataLeftLen = _iDataLen;
    
    if(NULL == _pstPsDataParse || NULL == _pcData || _iDataLen < 0)
    {
        iDataIndex = -1;
        Media_Debug("_pstPsDataParse(%p) _pcData(%p) _iDataLen=(%d)", _pstPsDataParse, _pcData, _iDataLen);
        goto end;
    }
    
    if(_pstPsDataParse->m_iEsLeftLen > 0)
    {
        Media_Debug("m_iEsLeftLen=%d", _pstPsDataParse->m_iEsLeftLen);
        Media_Debug("iDataLeftLen=%d", iDataLeftLen);
        int iCurrEslen = 0;
        if(_pstPsDataParse->m_iEsLeftLen > iDataLeftLen)
        {
            iCurrEslen = iDataLeftLen;
        }else
        {
            iCurrEslen = _pstPsDataParse->m_iEsLeftLen;
        }

        if(PsDataParsePushData(_pstPsDataParse, _pcData + iDataIndex, iCurrEslen) < 0)
        {
            Media_Debug("call PsDataParsePushData failed!");
            goto end;
        }
        
        iDataIndex += iCurrEslen;
        _pstPsDataParse->m_iEsLeftLen -= iCurrEslen;
        iDataLeftLen -= iCurrEslen;        
        //PrintHexData((unsigned char *)(_pcData + iDataIndex), iDataLeftLen, 16);
    }

    if(iDataIndex >= iDataLen)
    {
        Media_Error("iDataIndex(%d) >= iDataLen(%d)", iDataIndex, iDataLen);
        goto end;
    }

    while(iDataIndex < iDataLen)
    {
        iTmp = iDataLeftLen;
        if(iDataLeftLen < 8)
        {
            Media_Debug("iDataLeftLen(%d) < 4, break", iDataLeftLen);
            break;
        }
        iStartCode = FindNextStartCode((unsigned char *)pcData + iDataIndex, &iDataLeftLen, &iHeaderState);
        if(iDataLeftLen < 4)
        {
            Media_Debug("iDataLeftLen(%d) < 4, break", iDataLeftLen);
            break;
        }
        iFindOffset = iTmp - iDataLeftLen;

        iDataIndex += iFindOffset;
        
        //Media_Debug("iFindOffset=%d", iFindOffset);
        //Media_Debug("iDataIndex=%d iDataLen=%d", iDataIndex, iDataLen);
        
        //Media_Debug("iStartCode=0x%x", iStartCode);

        if(0x1ba == iStartCode)
        {
            Media_Debug("iStartCode=0x%x", iStartCode);
            Media_Debug("m_iEsLeftLen=%d", _pstPsDataParse->m_iEsLeftLen);
            Media_Debug("m_iVideoLen=%d", _pstPsDataParse->m_iVideoLen);
            Media_Debug("m_iAudioLen=%d", _pstPsDataParse->m_iAudioLen);
            Media_Debug("m_iAudioCount=%d", _pstPsDataParse->m_iAudioCount);
            Media_Debug("m_iRawDataWriteIndex=%d", _pstPsDataParse->m_iRawDataWriteIndex);
            if(_pstPsDataParse->m_iRawDataWriteIndex > 0)
            {
                _pstPsDataParse->m_iFlag = 1;
            }
            //PsDataParseReset(_pstPsDataParse);
            //PrintHexData((unsigned char *)pcData + iDataIndex, 16, 16);
            break;//cwm
        }else if(0x1bb == iStartCode)
        {
            //Media_Debug("iStartCode=0x%x", iStartCode);
            PrintHexData((unsigned char *)pcData + iDataIndex, 16, 16);
            continue;

        }else if(0x1bc == iStartCode)
        {
            //Media_Debug("iStartCode=0x%x", iStartCode);
            //PrintHexData((unsigned char *)pcData + iDataIndex, 16, 16);
            
            MpegpsPsmParse(&_pstPsDataParse->m_stMpegPsm, pcData + iDataIndex, iDataLeftLen);
            _pstPsDataParse->m_iIsPsm = 1;
            
        }else if(0x1e0 == iStartCode || 0x1c0 == iStartCode)
        {
            //Media_Debug("iStartCode=0x%x", iStartCode);
            //Media_Debug("iDataLeftLen=%d", iDataLeftLen);
            PesHeader *pstPesHeader = NULL;
            pstPesHeader = &_pstPsDataParse->m_stPesHeader;
            //PrintHexData((unsigned char *)pcData + iDataIndex, iDataLeftLen > 16 ? 16 : iDataLeftLen, 16);
            if(ParsePesHeader(pcData + iDataIndex, iDataLeftLen, pstPesHeader) < 0)
            {
                //Media_Debug("iDataIndex=%d iDataLen=%d, iFindOffset=%d break", iDataIndex, iDataLen, iFindOffset);
                if(iDataIndex > iFindOffset)
                {
                    iDataIndex -= iFindOffset;
                    iDataLeftLen += iFindOffset;
                }
                break;
            }

            if(iDataLeftLen <= pstPesHeader->m_iPesHeaderLen)
            {
                //Media_Debug("iDataLeftLen(%d) <= stPesHeader.m_iPesHeaderLen(%d), break", iDataLeftLen, pstPesHeader->m_iPesHeaderLen);
                //Media_Debug("iDataIndex=%d iDataLen=%d, iFindOffset=%d break", iDataIndex, iDataLen, iFindOffset);
                
                //PrintHexData((unsigned char *)pcData + iDataIndex, iDataLeftLen > 16 ? 16 : iDataLeftLen, 16);
                if(iDataIndex > iFindOffset)
                {
                    iDataIndex -= iFindOffset;
                    iDataLeftLen += iFindOffset;
                }
                break;
            }
            iDataIndex += pstPesHeader->m_iPesHeaderLen;
            iDataLeftLen -= pstPesHeader->m_iPesHeaderLen;
            
            _pstPsDataParse->m_iEsId = iStartCode;
            _pstPsDataParse->m_iEsLeftLen = pstPesHeader->m_iPesLen;

            if(0x1c0 == iStartCode)
            {
                _pstPsDataParse->m_iAudioCount++;
                _pstPsDataParse->m_iAudioLen = 0;
                #if 0
                if(0x90 == _pstPsDataParse->m_stMpegPsm.m_iAudioType ||
                    0x91 == _pstPsDataParse->m_stMpegPsm.m_iAudioType)
                {
                    if(320 == pstPesHeader->m_iPesLen)
                    {
                        unsigned char ucG711Header[4] = {0x00,0x01,0xa0,0x00};
                        PsDataParsePushData(_pstPsDataParse, (char *)ucG711Header, sizeof(ucG711Header));
                    }else
                    {
                        unsigned char ucG711Header[4] = {0x00,0x01,0xf0,0x00};
                        PsDataParsePushData(_pstPsDataParse, (char *)ucG711Header, sizeof(ucG711Header));
                    }

                }      
                #endif
            }

            if(_pstPsDataParse->m_iEsLeftLen > 0)
            {
                int iCurrEslen = 0;
                if(_pstPsDataParse->m_iEsLeftLen > iDataLeftLen)
                {
                    iCurrEslen = iDataLeftLen;
                }else
                {
                    iCurrEslen = _pstPsDataParse->m_iEsLeftLen;
                }
                //PrintHexData((unsigned char *)pcData + iDataIndex, 16, 16);
            
                PsDataParsePushData(_pstPsDataParse, _pcData + iDataIndex, iCurrEslen);
                iDataIndex += iCurrEslen;
                _pstPsDataParse->m_iEsLeftLen -= iCurrEslen;
                iDataLeftLen -= iCurrEslen;
                #if 0
                Media_Debug("iCurrEslen=%d", iCurrEslen);
                Media_Debug("iDataLeftLen=%d", iDataLeftLen);
                Media_Debug("_pstPsDataParse->m_iEsLeftLen=%d", _pstPsDataParse->m_iEsLeftLen);
                #endif
                
            }
            
        }else
        {
            Media_Debug("other iStartCode=0x%x", iStartCode);
        }

    }
    
end:

    return iDataIndex;
}

static int PsDemuxPutPsData(PsDataParse *_pstPsDataParse, char *_pcData, int _iLen, void *_pvPri)
{
    int iRet = 0;
    int iPsDataIndexUsed = 0;
    PsDataBuffer *pstPsDataBuffer = NULL;

    if(NULL == _pstPsDataParse || NULL == _pcData || _iLen <= 0)
    {
        Media_Debug("invalid _pstPsDataParse(%p) _pcData(%p) _iLen(%d)", _pstPsDataParse, _pcData, _iLen);
        iRet = -1;
        goto end;
    }

    if(1 == _pstPsDataParse->m_iFlag)
    {
        Media_Debug("xxxxxxxxxxxxxxxxxxxxxxxx");
        iRet = -1;
        goto end;
    }
    
    pstPsDataBuffer = &_pstPsDataParse->m_stPsDataBuffer;

    if((sizeof(pstPsDataBuffer->m_pcPsDataBuffer) - pstPsDataBuffer->m_iPsDataIndex) < _iLen)
    {
        Media_Debug("(sizeof(pstPsDataBuffer->m_pcPsDataBuffer)(%ld) - pstPsDataBuffer->m_iPsDataIndex(%d)) < _iLen(%d)", sizeof(pstPsDataBuffer->m_pcPsDataBuffer), pstPsDataBuffer->m_iPsDataIndex, _iLen);
        iRet = -1;
        goto end;
    }
    memcpy(pstPsDataBuffer->m_pcPsDataBuffer + pstPsDataBuffer->m_iPsDataIndex, _pcData, _iLen);
    pstPsDataBuffer->m_iPsDataIndex += _iLen;
    
    iPsDataIndexUsed = HandlePsStream(_pstPsDataParse, pstPsDataBuffer->m_pcPsDataBuffer, pstPsDataBuffer->m_iPsDataIndex, _pvPri);
    if(iPsDataIndexUsed < 0)
    {
        Media_Debug("call HandlePsStream failed!");
        iRet = -1;
        goto end;
    }
    pstPsDataBuffer->m_iPsDataIndex = pstPsDataBuffer->m_iPsDataIndex - iPsDataIndexUsed;
    Media_Debug("iPsDataIndex=%d", pstPsDataBuffer->m_iPsDataIndex);
    if(pstPsDataBuffer->m_iPsDataIndex > 0)
    {
        memmove(pstPsDataBuffer->m_pcPsDataBuffer, pstPsDataBuffer->m_pcPsDataBuffer + iPsDataIndexUsed, pstPsDataBuffer->m_iPsDataIndex);
        Media_Debug("memmove iPsDataIndex=%d", pstPsDataBuffer->m_iPsDataIndex);
    }
    
end:
    return iRet;
}



static AVCodecID GetCodecIdByEsTypeTmp(int _esType)
{
    AVCodecID res = (AVCodecID)0;

    switch(_esType)
    {
        case STREAM_TYPE_AUDIO_MPEG1:
            res = AV_CODEC_ID_MP3;
            break;
        case STREAM_TYPE_AUDIO_MPEG2:
            res = AV_CODEC_ID_MP3;
            break;
        case STREAM_TYPE_VIDEO_MPEG4:
            res = AV_CODEC_ID_MPEG4;
            break;
        case STREAM_TYPE_VIDEO_H264:
            res = AV_CODEC_ID_H264;
            break;
        case STREAM_TYPE_VIDEO_HEVC:
            res = AV_CODEC_ID_HEVC;
            break;
        case STREAM_TYPE_VIDEO_MJPEG:
            res = AV_CODEC_ID_MJPEG;
            break;
        case STREAM_TYPE_VIDEO_SVC:
            res = AV_CODEC_ID_SVC;
            break;
        case STREAM_TYPE_AUDIO_PCM_ALAW:
            res = AV_CODEC_ID_PCM_ALAW;
            break;
        case STREAM_TYPE_AUDIO_PCM_MULAW:
            res = AV_CODEC_ID_PCM_MULAW;
            break;
        case STREAM_TYPE_AUDIO_G722_1:
            res = AV_CODEC_ID_G722_1;
            break;
        case STREAM_TYPE_AUDIO_G723_1:
            res = AV_CODEC_ID_G723_1;
            break;
        case STREAM_TYPE_AUDIO_G729:
            res = AV_CODEC_ID_G729;
            break;
        case STREAM_TYPE_AUDIO_SAC:
            res = AV_CODEC_ID_SAC;
            break;
        case STREAM_TYPE_AUDIO_AAC:
            res = AV_CODEC_ID_AAC;
            break;
        case STREAM_TYPE_AUDIO_ADPCM_IMA_APC:
            res = AV_CODEC_ID_ADPCM_IMA_APC;
            break;
        default:
            break;
    }
    
    return res;
}

void *PsDemux_Open(void)
{
    PsDataParse *pstPsDataParse = NULL;

    pstPsDataParse = (PsDataParse *)malloc(sizeof(PsDataParse));

    if(NULL == pstPsDataParse)
    {
        Media_Debug("call malloc failed!");
        goto end;
    }

    memset(pstPsDataParse, 0, sizeof(PsDataParse));

    pstPsDataParse->m_iIsPsm = 0;    
    pstPsDataParse->m_iEsId = 0;
    pstPsDataParse->m_iEsLen = 0;
    pstPsDataParse->m_iEsLeftLen = 0;
    pstPsDataParse->m_iVideoLen = 0;
    pstPsDataParse->m_iAudioLen = 0;
    pstPsDataParse->m_iAudioCount = 0;
    pstPsDataParse->m_iRawDataMaxSize = 0;
    pstPsDataParse->m_pcRawData = NULL;
    pstPsDataParse->m_iRawDataWriteIndex = 0;//;
    pstPsDataParse->m_iRawDataMaxSize = 0;   

end:
    return pstPsDataParse;    
}

int PsDemux_PutPsData(void *_ph, char * _pcData, int _iDataLen)
{
    int iRet = 0;

    if(NULL == _ph ||
        NULL == _pcData ||
        _iDataLen <= 0)
    {
        Media_Debug("invalid _ph(%p) _pcData(%p) _iDataLen(%d)", _ph, _pcData, _iDataLen);
        iRet = -1;
        goto end;
    }
    
        
    if(PsDemuxPutPsData((PsDataParse *)_ph, _pcData, _iDataLen, NULL) < 0)
    {
        Media_Debug("call PsDemuxPutPsData failed!");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;
}

int PsDemux_GetRawData(void *_ph, PsFrameInfo *_pstPsFrameInfo, unsigned char **_ppData, unsigned int *_puiLen)
{
    int iRet = 0;
    PsDataParse *pstPsDataParse = NULL;
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    

    if(NULL == _ph ||
        NULL == _pstPsFrameInfo ||
        _ppData <= 0)
    {
        Media_Debug("invalid _ph(%p) _pstPsFrameInfo(%p) _ppData(%p) _puiLen(%p)", _ph, _pstPsFrameInfo, _ppData, _puiLen);
        iRet = -1;
        goto end;
    }

    pstPsDataParse = (PsDataParse *)_ph;

    if(0 == pstPsDataParse->m_iFlag)
    {
        goto end;
    }
    
    Media_Debug("###################################################");
    Media_Debug("m_iEsLeftLen=%d", pstPsDataParse->m_iEsLeftLen);
    Media_Debug("m_iVideoLen=%d", pstPsDataParse->m_iVideoLen);
    Media_Debug("m_iAudioLen=%d", pstPsDataParse->m_iAudioLen);
    Media_Debug("m_iAudioCount=%d", pstPsDataParse->m_iAudioCount);
    Media_Debug("m_iRawDataWriteIndex=%d", pstPsDataParse->m_iRawDataWriteIndex);

    if(NULL == *_ppData)
    {
        if(pstPsDataParse->m_iVideoLen > 0)
        {
            *_ppData = (unsigned char *)(pstPsDataParse->m_pcRawData);
            *_puiLen = pstPsDataParse->m_iVideoLen;
            pstPsDataParse->m_iReadIndex = pstPsDataParse->m_iVideoLen;
            pstPsDataParse->m_iVideoLen = 0;  
            eAVCodecID = GetCodecIdByEsTypeTmp(pstPsDataParse->m_stMpegPsm.m_iVideoType);//todo
        }else if(pstPsDataParse->m_iAudioCount > 0)
        {
            *_ppData = (unsigned char *)(pstPsDataParse->m_pcRawData) + pstPsDataParse->m_iReadIndex;
            *_puiLen = pstPsDataParse->m_iAudioLen;
            Media_Debug("*_puiLen=%d", *_puiLen);
            pstPsDataParse->m_iReadIndex = pstPsDataParse->m_iReadIndex + pstPsDataParse->m_iAudioLen;
            pstPsDataParse->m_iAudioCount--;
            eAVCodecID = GetCodecIdByEsTypeTmp(pstPsDataParse->m_stMpegPsm.m_iAudioType);//todo
        }
    }else
    {
        if(pstPsDataParse->m_iVideoLen > 0)
        {
            memcpy(*_ppData, pstPsDataParse->m_pcRawData, pstPsDataParse->m_iVideoLen);
            *_puiLen = pstPsDataParse->m_iVideoLen;
            pstPsDataParse->m_iReadIndex = pstPsDataParse->m_iVideoLen;
            pstPsDataParse->m_iVideoLen = 0;  
            eAVCodecID = GetCodecIdByEsTypeTmp(pstPsDataParse->m_stMpegPsm.m_iVideoType);//todo
        }else if(pstPsDataParse->m_iAudioCount > 0)
        {
            memcpy(*_ppData, pstPsDataParse->m_pcRawData + pstPsDataParse->m_iReadIndex, pstPsDataParse->m_iAudioLen);
            *_puiLen = pstPsDataParse->m_iAudioLen;
            pstPsDataParse->m_iReadIndex = pstPsDataParse->m_iReadIndex + pstPsDataParse->m_iAudioLen;
            pstPsDataParse->m_iAudioCount--;
            eAVCodecID = GetCodecIdByEsTypeTmp(pstPsDataParse->m_stMpegPsm.m_iAudioType);//todo
        }
    }

    if(0 == pstPsDataParse->m_iVideoLen && 0 == pstPsDataParse->m_iAudioCount)
    {
        pstPsDataParse->m_iFlag = 0;
        pstPsDataParse->m_iReadIndex = 0;
        pstPsDataParse->m_iAudioLen = 0;
        pstPsDataParse->m_iRawDataWriteIndex = 0;
    }

    _pstPsFrameInfo->m_eAVCodecID = eAVCodecID;
    _pstPsFrameInfo->m_isSyncFrame = pstPsDataParse->m_iIsPsm;
    _pstPsFrameInfo->m_TimestampMs = pstPsDataParse->m_stPesHeader.m_i64Pts / 90;
    
    pstPsDataParse->m_iIsPsm = 0;    

end:
    return iRet;
}


static int PsDemuxClose(PsDataParse *_pstPsDataParse)
{
    int iRet = 0;
    
    if(NULL == _pstPsDataParse)
    {
        Media_Error("invalid _pstPsDataParse(%p)", _pstPsDataParse);
        iRet = -1;
        goto end;
    }
    
    _pstPsDataParse->m_iIsPsm = 0;    
    _pstPsDataParse->m_iEsId = 0;
    _pstPsDataParse->m_iEsLen = 0;
    _pstPsDataParse->m_iEsLeftLen = 0;
    _pstPsDataParse->m_iVideoLen = 0;
    _pstPsDataParse->m_iAudioLen = 0;
    _pstPsDataParse->m_iAudioCount = 0;
    _pstPsDataParse->m_iRawDataMaxSize = 0;
    if(NULL != _pstPsDataParse->m_pcRawData)
    {
        free(_pstPsDataParse->m_pcRawData);
        _pstPsDataParse->m_pcRawData = NULL;
    }
    _pstPsDataParse->m_iRawDataWriteIndex = 0;
    if(NULL != _pstPsDataParse)
    {
        free(_pstPsDataParse);
        _pstPsDataParse = NULL;
    }
end:
    return iRet;
}

int PsDemux_Close(void *_ph)
{
    int iRet = 0;

    if(NULL == _ph)
    {
        Media_Debug("invalid _ph(%p)", _ph);
        iRet = -1;
        goto end;
    }

    if(PsDemuxClose((PsDataParse *)_ph) < 0)
    {
        Media_Debug("call PsDemuxClose failed!");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;
}



