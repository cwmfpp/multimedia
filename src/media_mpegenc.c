
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "media_mpegenc.h"
#include "vlc_bits.h"
#include "media_log.h"


static unsigned int mpegCRC_32 = 0x04C11DB7;  //MPEG  
//static unsigned int sshCRC_32 = 0xEDB88320;   //ssh
static unsigned int Table_mpegCRC[256];       //MPEG CRC ��
//static unsigned int Table_sshCRC[256];        //ssh CRC ��



// ���� 32 λ CRC �� 
static void BuildTable32( unsigned int aPoly,unsigned int uiTable[]) 
{ 
    unsigned int i, j; 
    unsigned int nData; 
    unsigned int nAccum; 

    for ( i = 0; i < 256; i++ ) 
    { 
        nData = ( unsigned int )( i << 24 ); 
        nAccum = 0; 
        for ( j = 0; j < 8; j++ ) 
        { 
            if ( ( nData ^ nAccum ) & 0x80000000 ) 
            {
                nAccum = ( nAccum << 1 ) ^ aPoly;
            }
            else 
            {
                nAccum <<= 1;

            }
            nData <<= 1; 
        } 
        uiTable[i] = nAccum; 
    } 
} 


#if 0

unsigned int ssh_crc32(const unsigned char *buf, unsigned int size)
{
 unsigned int i, crc;
    
 crc = 0xFFFFFFFF;
 BuildTable32(sshCRC_32,Table_sshCRC);
 
 for (i = 0; i < size; i++)
  crc = Table_sshCRC[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
 return crc^0xFFFFFFFF;
}
#endif


//MPEG CRC-32�㷨
static unsigned int  mpeg_crc32(const unsigned char *data, int len)
{    
    int i;    
    unsigned int crc = 0xFFFFFFFF;  
    BuildTable32(mpegCRC_32,Table_mpegCRC); 
    for(i = 0; i < len; i++)
    {
        crc = (crc << 8) ^ Table_mpegCRC[((crc >> 24) ^ *data++) & 0xFF];        
    }
    return crc;
}

int Mpeg_PutPackHeader(uint8_t *_pBuf, uint64_t timestamp, unsigned int _uiStuffValue)
{
    int iSize = 0;
    bs_t pb;
    int iBits = 0;
    
    bs_init(&pb, _pBuf, PS_HEADER_LEN_MAX);

    bs_write32(&pb, PACK_START_CODE);//pack_start_code    
    bs_write(&pb, 2, 0x1);//gb '01'
    bs_write(&pb,  3, (uint32_t)((timestamp >> 30) & 0x07));//system_clock_reference_base [32..30]
    bs_write(&pb,  1, 1);//marker_bit
    bs_write(&pb, 15, (uint32_t)((timestamp >> 15) & 0x7fff));//system_clock_reference_base [29..15]
    bs_write(&pb,  1, 1);//marker_bit
    bs_write(&pb, 15, (uint32_t)((timestamp >> 0)  & 0x7fff));//system_clock_reference_base [14..0]
    bs_write(&pb,  1, 1);//marker_bit
    bs_write(&pb, 9, 0);//gb system_clock_reference_extension
    bs_write(&pb, 1, 1);//marker_bit
    bs_write(&pb, 22, 41944);//program_mux_rate 41944 0x147b
    bs_write(&pb, 1, 1);//marker_bit    
    bs_write(&pb, 1, 1);//gb marker_bit
    bs_write(&pb, 5, 0x1f); /* reserved *///gb reserved
    bs_write(&pb, 3, 6); /* stuffing length *///gb pack_stuffing_length
    bs_write(&pb, 16, 0xffff);
    bs_write32(&pb, _uiStuffValue);

    iBits = bs_pos(&pb);
    
    //Media_Debug("iBits=%d\n", iBits);
    //PrintHexData(_pBuf, iBits / 8, 16);
    if(iBits & 7)
    {
        Media_Error("iBits=%d error\n", iBits);
    }
    iSize = iBits / 8;
    
    return iSize;
}


int Mpeg_PutSystemHeader(uint8_t *_pBuf)
{
    int iSize = 0;
    //int i = 0;
    bs_t pb, pb_HeaderLen;
    int iBits = 0;
    int iSysHeaderLen = 0;
    //gb
    //int P_STD_max_video = 0;
    int P_STD_max_mpeg_audio = 0;
    //int P_STD_max_mpeg_PS1 = 0;
    
    bs_init(&pb, _pBuf, PS_SYSTEM_HEADER_LEN_MAX);

    bs_write32(&pb, SYSTEM_HEADER_START_CODE);//system_header_start_code
    bs_save(&pb, &pb_HeaderLen);
    bs_write(&pb, 16, (uint32_t)iSysHeaderLen);//header_length
    bs_write(&pb, 1, 1);//marker_bit

    /* maximum bit rate of the multiplexed stream */
    bs_write(&pb, 22, 41944);//rate_bound hk=41944
    bs_write(&pb, 1, 1); /* marker *///marker_bit
    bs_write(&pb, 6, 1);//gb hk=1 audio_bound

  
    bs_write(&pb, 1, 0); /* variable bitrate */ //gb fixed_flag
    bs_write(&pb, 1, 0); /* nonconstrained bitstream */ //gb CSPS_flag
    
    /* see VCD standard p IV-7 */
    bs_write(&pb, 1, 1); /* audio locked */ //gb system_audio_lock_flag
    bs_write(&pb, 1, 1); /* video locked */ //gb system_video_lock_flag

    bs_write(&pb, 1, 1); /* marker */ //marker_bit

    bs_write(&pb, 5, 1); //gb hk=1 //video_bound
    
    bs_write(&pb, 1, 0);    /* packet_rate_restriction_flag */ //gb packet_rate_restriction_flag
    bs_write(&pb, 7, 0x7f); /* reserved byte */ //gb reserved_bits


    /* DVD-Video Stream_bound entries
     * id (0xB9) video, maximum P-STD for stream 0xE0. (P-STD_buffer_bound_scale = 1)
     * id (0xB8) audio, maximum P-STD for any MPEG audio (0xC0 to 0xC7) streams. If there are none set to 4096 (32x128). (P-STD_buffer_bound_scale = 0)
     * id (0xBD) private stream 1 (audio other than MPEG and subpictures). (P-STD_buffer_bound_scale = 1)
     * id (0xBF) private stream 2, NAV packs, set to 2x1024. */
    
#if 0
    for (i = 0; i < ctx->nb_streams; i++) {
        StreamInfo *stream = ctx->streams[i]->priv_data;

        id = stream->id;
        if (id == 0xbd && stream->max_buffer_size > P_STD_max_mpeg_PS1) {
            P_STD_max_mpeg_PS1 = stream->max_buffer_size;
        } else if (id >= 0xc0 && id <= 0xc7 &&
                   stream->max_buffer_size > P_STD_max_mpeg_audio) {
            P_STD_max_mpeg_audio = stream->max_buffer_size;
        } else if (id == 0xe0 &&
                   stream->max_buffer_size > P_STD_max_video) {
            P_STD_max_video = stream->max_buffer_size;
        }
    }
#endif

    /* video */
    bs_write(&pb, 8, 0xe0); /* stream ID 0xb9*/ //stream_id hk=0xe0
    bs_write(&pb, 2, 3);// '11'
    bs_write(&pb, 1, 1);// P-STD_buffer_bound_scale
    bs_write(&pb, 13, 128/*P_STD_max_video / 1024*/);// P-STD_buffer_size_bound 128

    /* audio */
    P_STD_max_mpeg_audio = 4096;
    P_STD_max_mpeg_audio = P_STD_max_mpeg_audio;
    
    bs_write(&pb, 8, 0xc0); /* stream ID 0xb8*///stream_id hk=0xc0
    bs_write(&pb, 2, 3);// '11'
    bs_write(&pb, 1, 0);// P-STD_buffer_bound_scale
    bs_write(&pb, 13, 8/*P_STD_max_mpeg_audio / 128*/);// P-STD_buffer_size_bound 8

    /* private stream 1 */
    bs_write(&pb, 8, 0xbd); /* stream ID *///stream_id
    bs_write(&pb, 2, 3);// '11'
    bs_write(&pb, 1, 1);// P-STD_buffer_bound_scale
    bs_write(&pb, 13, 128/*P_STD_max_mpeg_PS1 / 128*/);// P-STD_buffer_size_bound

    /* private stream 2 */
    bs_write(&pb, 8, 0xbf); /* stream ID *///stream_id
    bs_write(&pb, 2, 3);// '11'
    bs_write(&pb, 1, 1);// P-STD_buffer_bound_scale
    bs_write(&pb, 13, 128);// P-STD_buffer_size_bound

    iSysHeaderLen = (bs_pos(&pb) - bs_pos(&pb_HeaderLen)) - 16;
    iSysHeaderLen = iSysHeaderLen / 8;
    bs_write(&pb_HeaderLen, 16, (uint32_t)iSysHeaderLen);
    iBits = bs_pos(&pb);
    //Media_Debug("iBits=%d\n", iBits);
    if(iBits & 7)
    {
        Media_Error("iBits=%d error\n", iBits);
    }
    iSize = iBits / 8;
    
    return iSize;
}


static uint8_t GetStreamType(int stream_codec_id)
{
    uint8_t res = 0;

    switch(stream_codec_id)
    {
        case AV_CODEC_ID_MPEG4:
            res = STREAM_TYPE_VIDEO_MPEG4;
            break;
        case AV_CODEC_ID_H264:
            res = STREAM_TYPE_VIDEO_H264;
            break;
        case AV_CODEC_ID_HEVC:
            res = STREAM_TYPE_VIDEO_HEVC;
            break;
        case AV_CODEC_ID_MJPEG:
            res = STREAM_TYPE_VIDEO_MJPEG;
            break;
        case AV_CODEC_ID_SVC:
            res = STREAM_TYPE_VIDEO_SVC;
            break;
        case AV_CODEC_ID_PCM_ALAW:
            res = STREAM_TYPE_AUDIO_PCM_ALAW;
            break;
        case AV_CODEC_ID_PCM_MULAW:
            res = STREAM_TYPE_AUDIO_PCM_MULAW;
            break;
        case AV_CODEC_ID_G722_1:
            res = STREAM_TYPE_AUDIO_G722_1;
            break;
        case AV_CODEC_ID_G723_1:
            res = STREAM_TYPE_AUDIO_G723_1;
            break;
        case AV_CODEC_ID_G729:
            res = STREAM_TYPE_AUDIO_G729;
            break;
        case AV_CODEC_ID_SAC:
            res = STREAM_TYPE_AUDIO_SAC;
            break;
        case AV_CODEC_ID_AAC:
            res = STREAM_TYPE_AUDIO_AAC;
            break;
        case AV_CODEC_ID_ADPCM_IMA_APC:
            res = STREAM_TYPE_AUDIO_ADPCM_IMA_APC;
            break;
        default:
            break;
    }
    
    return res;
}

int Mpeg_InitPsmHeader(PSMHeader *_pPSMHeader)
{
    AVCodecParameters *codec = NULL;

    _pPSMHeader->m_uiPSInfoLen = 0;
    
    codec = _pPSMHeader->m_stCodec;
    codec[0].m_eCodecType = AVMEDIA_TYPE_UNKNOWN;
    codec[0].m_eCodecId = (AVCodecID)0;
    codec[0].m_uiESInfoLen = 0;
    
    codec[1].m_eCodecType = AVMEDIA_TYPE_UNKNOWN;
    codec[1].m_eCodecId = (AVCodecID)0;
    codec[1].m_uiESInfoLen = 0;

    return 0;
}

int Mpeg_SetProgramStreamInfo(PSMHeader *_pPSMHeader, char *_pPSInfo, unsigned int _uiLen)
{
    if(NULL == _pPSMHeader || NULL == _pPSInfo || _uiLen == 0 || _uiLen > PROGRAM_STREAM_INFO_LEN)
    {  
        Media_Error("_pPSMHeader(%p) _pPSInfo(%p) _iLen(%d)<=0 or > PROGRAM_STREAM_INFO_LEN(%d)", _pPSMHeader, _pPSInfo, _uiLen, PROGRAM_STREAM_INFO_LEN);   
        return -1;
    }

    _pPSMHeader->m_uiPSInfoLen = _uiLen;
    
    memcpy((char *)_pPSMHeader->m_cPSInfoData, _pPSInfo, _uiLen);
    
    return 0;
}

int Mpeg_GetProgramStreamInfoData(PSMHeader *_pPSMHeader, char **_ppPSInfo, unsigned int *_puiLen)
{
    if(NULL == _pPSMHeader || NULL == _ppPSInfo || NULL == _puiLen)
    {  
        Media_Error("_pPSMHeader(%p) _ppPSInfo(%p) _puiLen(%p)", _pPSMHeader, _ppPSInfo, _puiLen);   
        return -1;
    }
    
    *_ppPSInfo = _pPSMHeader->m_cPSInfoData;
    *_puiLen = _pPSMHeader->m_uiPSInfoLen;
    
    return 0;
}

int Mpeg_SetElementaryStream(PSMHeader *_pPSMHeader, int _iESIndex, AVMediaType _eAVMediaType, AVCodecID _eAVCodecID, char *_pcESInfoData, int _iESInfoLen)
{
    if(NULL == _pPSMHeader || NULL == _pcESInfoData)
    {
        Media_Error("_pPSMHeader(%p) _pcESInfoData(%p)", _pPSMHeader, _pcESInfoData); 
        return -1;
    }

    if(_iESIndex >= ELEMENTARY_STREAM_NUM)
    {
        Media_Error("_iESIndex(%d) >= ELEMENTARY_STREAM_NUM(%d)", _iESIndex, ELEMENTARY_STREAM_NUM); 
        return -1;
    }

    AVCodecParameters *pstCodec = NULL;

    pstCodec = _pPSMHeader->m_stCodec;


    if(_iESInfoLen > ELEMENTARY_STREAM_INFO_LEN)
    {
        Media_Error("_iESInfoLen(%d) >= ELEMENTARY_STREAM_INFO_LEN(%d)", _iESInfoLen, ELEMENTARY_STREAM_INFO_LEN); 
        return -1;
    }

    pstCodec[_iESIndex].m_eCodecType = _eAVMediaType;
    pstCodec[_iESIndex].m_eCodecId = _eAVCodecID;
    pstCodec[_iESIndex].m_uiESInfoLen = (unsigned int)_iESInfoLen;
    if(_iESInfoLen > 0)
    {
        memcpy(pstCodec[_iESIndex].m_cESInfoData, _pcESInfoData, (unsigned int)_iESInfoLen);
    }
    
    return 0;
}

int Mpeg_GetElementaryStreamData(PSMHeader *_pPSMHeader, int _iESIndex, char **_ppcESInfoData, int *_piESInfoLen)
{
    if(NULL == _pPSMHeader || NULL == _ppcESInfoData || NULL == _piESInfoLen)
    {
        return -1;
    }

    if(_iESIndex >= ELEMENTARY_STREAM_NUM)
    {
        return -1;
    }

    AVCodecParameters *pstCodec = NULL;

    pstCodec = _pPSMHeader->m_stCodec;

    *_ppcESInfoData = pstCodec[_iESIndex].m_cESInfoData;
    *_piESInfoLen = (int)(pstCodec[_iESIndex].m_uiESInfoLen);
    
    return 0;
}

AVCodecID Mpeg_GetVideoCodecId(PSMHeader *_pPSMHeader)
{
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    
    if(NULL == _pPSMHeader)
    {
        return AV_CODEC_ID_NONE;
    }

    AVCodecParameters *pstCodec = NULL;

    pstCodec = _pPSMHeader->m_stCodec;

    eAVCodecID = pstCodec[0].m_eCodecId;
    
    return eAVCodecID;
}

AVCodecID Mpeg_GetAudioCodecId(PSMHeader *_pPSMHeader)
{
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    
    if(NULL == _pPSMHeader)
    {
        return AV_CODEC_ID_NONE;
    }

    AVCodecParameters *pstCodec = NULL;

    pstCodec = _pPSMHeader->m_stCodec;

    eAVCodecID = pstCodec[1].m_eCodecId;
    
    return eAVCodecID;
}

int Mpeg_PutPsmHeader(PSMHeader *_pPSMHeader, uint8_t *_pBuf)
{
    unsigned int i;
    int iSize;
    bs_t pb, pb_PSMap, pb_ESMap;
    int iBits = 0;

    AVCodecParameters *codec = NULL;
    unsigned int uiPSMapLen = 0;
    unsigned int uiPSInfoLen = 0;
    char *cPSInfoData = NULL;
    unsigned int uiESMapLen = 0;

    #if 0
    #if 0
    char cPSInfoDataArrayCheck[] = {0x77, 0x65, 0x6e, 0x6d, 0x69, 0x6e, 0x63, 0x68, 
                                0x65, 0x6e, 0x40, 0x31, 0x32, 0x36, 0x2e, 0x63, 
                                0x6f, 0x6d,};
    #else
    unsigned char cPSInfoDataArrayCheck[] = {0x40, 0x0E, 0x48, 0x4B, 0x01, 0x00, 0x11, 0xB3, 
                                    0x43, 0x9A, 0x3A, 0x40, 0x00, 0xFF, 0xFF, 0xFF, 
                                    0x41, 0x12, 0x48, 0x4B, 0x00, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00};
    #endif
    uiPSInfoLen = sizeof(cPSInfoDataArrayCheck);//_pPSMHeader->m_iPSInfoLen;
    cPSInfoData = (char *)cPSInfoDataArrayCheck;//_pPSMHeader->m_cPSInfoData;
    #endif
    

    codec = _pPSMHeader->m_stCodec;

    uiPSInfoLen = _pPSMHeader->m_uiPSInfoLen;
    cPSInfoData = _pPSMHeader->m_cPSInfoData;

    bs_init(&pb, _pBuf, PS_MAP_HEADER_LEN_MAX);

    bs_write32(&pb, PROGRAM_STREAM_MAP_START_CODE);
    bs_save(&pb, &pb_PSMap);//save uiPSMapLen
    bs_write(&pb, 16, uiPSMapLen);         /*uiPSMapLen*/ //10 + uiPSInfoLen + uiESMapLen
    bs_write(&pb, 1, 1);          /*current_next_indicator */
    bs_write(&pb, 2, 3);          /*reserved*/
    bs_write(&pb, 5, 0xc);          /*program_stream_map_version*///old = 0
    bs_write(&pb, 7, 0x7F);       /*reserved */
    bs_write(&pb, 1, 1);          /*marker_bit */
    bs_write(&pb, 16,uiPSInfoLen);          /*uiPSInfoLen*/
    for(i = 0; i < uiPSInfoLen; i++)
    {
        bs_write(&pb, 8, (unsigned char)cPSInfoData[i]);       /*descriptor*/
    }
    
    bs_save(&pb, &pb_ESMap);//save uiESMapLen
    bs_write(&pb, 16, uiESMapLen);         /*uiESMapLen*/
    for (i = 0 ; i < ELEMENTARY_STREAM_NUM; i++)
    {
        if (codec[i].m_eCodecType == AVMEDIA_TYPE_VIDEO || codec[i].m_eCodecType == AVMEDIA_TYPE_AUDIO)
        {
            unsigned int j = 0;
            bs_write(&pb, 8, GetStreamType(codec[i].m_eCodecId));       /*stream_type*/
            if (codec[i].m_eCodecType == AVMEDIA_TYPE_AUDIO)
            {
                bs_write(&pb, 8, AUDIO_ID);       /*elementary_stream_id*/
            }
            if (codec[i].m_eCodecType == AVMEDIA_TYPE_VIDEO)
            {
                bs_write(&pb, 8, VIDEO_ID);       /*elementary_stream_id*/
            }

            bs_write(&pb, 16, codec[i].m_uiESInfoLen);         /*elementary_stream_info_length*/
            for(j = 0; j < codec[i].m_uiESInfoLen; j++)
            {
                bs_write(&pb, 8, (unsigned char)codec[i].m_cESInfoData[j]);       /*descriptor*/
            }
        }
    }
    
    uiESMapLen = (unsigned int)(bs_pos(&pb) - bs_pos(&pb_ESMap)) - 16;    
    //Media_Debug("uiESMapLen=%d\n", uiESMapLen);   
    bs_write(&pb_ESMap, 16, uiESMapLen / 8);
    
    uint32_t crc = 0x12345678;//0;//crc32(0, buf, 13 + streminfolen);
    
    iBits = bs_pos(&pb);
    //Media_Debug("iBits=%d\n", iBits);
    crc = mpeg_crc32((const unsigned char *)_pBuf, iBits / 8);
    /*audio*/
    /*crc (2e b9 0f 3d)*/
    bs_write32(&pb, crc);       /*CRC_32*/
    //flush_put_bits(&pb);

    uiPSMapLen = (unsigned int)(bs_pos(&pb) - bs_pos(&pb_PSMap)) - 16;
    //Media_Debug("uiPSMapLen=%d\n", uiPSMapLen);    
    bs_write(&pb_PSMap, 16, uiPSMapLen / 8);
    
    iBits = bs_pos(&pb);
    //Media_Debug("iBits=%d\n", iBits);
    if(iBits & 7)
    {
        Media_Error("iBits=%d error\n", iBits);
    }
    iSize = iBits / 8;
    
    return iSize;
}


int Mpeg_PutPesHeader(uint8_t *pucBuf, unsigned char _ucStreamId, unsigned char _ucDataAlignIndicator, unsigned int _iPayloadLen, int _PTS_DTS_flags, uint64_t _ullPts, uint64_t _ullDts, int iStuffingByte, uint8_t *_pcStuffingByte)
{
    int i, iSize;
    bs_t pb, pb_PESLen, pb_PESHeaderLen;
    int iBits = 0;
    
    unsigned int PES_packet_length = 0;//pb_PESLen  
    unsigned int PTS_DTS_flags = 0;//2bit
    unsigned int ESCR_flag = 0;//1bit
    unsigned int ES_rate_flag = 0;//1bit
    unsigned int DSM_trick_mode_flag = 0;//1bit
    unsigned int trick_mode_control = 0;//3bit
    unsigned int additional_copy_info_flag = 0;//1bit
    unsigned int PES_CRC_flag = 0;//1bit
    unsigned int PES_extension_flag = 0;//1bit
    unsigned int PES_header_data_length = 0;
    unsigned int PES_private_data_flag = 0;//1bit
    unsigned int pack_header_field_flag = 0;//1bit
    unsigned int program_packet_sequence_counter_flag = 0;//1bit
    unsigned int P_STD_buffer_flag = 0;//1bit
    unsigned int PES_extension_flag_2 = 0;//1bit
    int PES_extension_field_length = 0;
    int N1 = 0;

    //for eliminate pclint,pain
    if(_iPayloadLen == (unsigned int)(~0))
    {    
        ESCR_flag = _ucDataAlignIndicator;
        ES_rate_flag = _ucDataAlignIndicator;
        DSM_trick_mode_flag = _ucDataAlignIndicator;
        trick_mode_control = _ucDataAlignIndicator;
        additional_copy_info_flag = _ucDataAlignIndicator;
        PES_CRC_flag = _ucDataAlignIndicator;
        PES_extension_flag = _ucDataAlignIndicator;
        PES_private_data_flag = _ucDataAlignIndicator;
        pack_header_field_flag = _ucDataAlignIndicator;
        program_packet_sequence_counter_flag = _ucDataAlignIndicator;
        P_STD_buffer_flag = _ucDataAlignIndicator;
        PES_extension_flag_2 = _ucDataAlignIndicator;
        PES_extension_field_length = _ucDataAlignIndicator;
    }
    
    PTS_DTS_flags = (unsigned int)_PTS_DTS_flags;
    bs_init(&pb, pucBuf, PS_PES_HEADER_LEN_MAX);    

    bs_write(&pb, 24, 0x000001);//packet_start_code_prefix //24bit 00000001 , 
    bs_write(&pb, 8, _ucStreamId);//stream_id //8bit 0xe0 0xc0
    
    bs_save(&pb, &pb_PESLen);
    
    bs_write(&pb, 16, PES_packet_length);//PES_packet_length //16bit ֮��ĳ���(ͷ+ ����)
    
    bs_write(&pb, 2, 0x2);//'10' //2bit reserved mpeg2 PES
    bs_write(&pb, 2, 0);//PES_scrambling_control //2bit 0
    bs_write(&pb, 1, 1);//PES_priority //1bit 1 
    bs_write(&pb, 1, _ucDataAlignIndicator);//data_alignment_indicator //1bit 1 ��ʾһ��nalu��Ԫ�ĵ�һ��pes��ʱ��1��������0
    bs_write(&pb, 1, 0);//copyright //1bit 0
    bs_write(&pb, 1, 0);//original_or_copy //1bit 0
    bs_write(&pb, 2, PTS_DTS_flags);//PTS_DTS_flags //2bit 2
    bs_write(&pb, 1, ESCR_flag);//ESCR_flag //1bit 0
    bs_write(&pb, 1, ES_rate_flag);//ES_rate_flag //1bit 0
    bs_write(&pb, 1, DSM_trick_mode_flag);//DSM_trick_mode_flag //1bit 0
    bs_write(&pb, 1, additional_copy_info_flag);//additional_copy_info_flag //1bit 0
    bs_write(&pb, 1, PES_CRC_flag);//PES_CRC_flag //1bit 0
    bs_write(&pb, 1, PES_extension_flag);//PES_extension_flag //1bit 0

    bs_save(&pb, &pb_PESHeaderLen);        
    bs_write(&pb, 8, PES_header_data_length);//PES_header_data_length //8bit ֮��ͷ�ĳ���
    if(PTS_DTS_flags == 0x2) {//0b10 2bit
        bs_write(&pb, 4, 0x2);//'0010' //4bit
        bs_write(&pb, 3, (unsigned int)((_ullPts >> 30) & 0x7));//PTS[32.30] //3bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
        bs_write(&pb, 15, (unsigned int)((_ullPts >> 15) & 0x7fff));//PTS[29.15] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
        bs_write(&pb, 15, (unsigned int)((_ullPts >> 0) & 0x7fff));//PTS[14..0] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
    }
    if(PTS_DTS_flags == 0x3) {//0b11
        bs_write(&pb, 4, 0x3);//'0011' //4bit
        bs_write(&pb, 3, (unsigned int)((_ullPts >> 30) & 0x7));//PTS[32.30] //3bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
        bs_write(&pb, 15, (unsigned int)((_ullPts >> 15) & 0x1ffff));//PTS[29.15] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
        bs_write(&pb, 15, (unsigned int)((_ullPts >> 0) & 0x1ffff));//PTS[14..0] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
        
        bs_write(&pb, 4, 0x3);//'0011' //4bit
        bs_write(&pb, 3, (unsigned int)((_ullDts >> 30) & 0x7));//DTS[32.30] //3bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
        bs_write(&pb, 15, (unsigned int)((_ullDts >> 15) & 0x1ffff));//DTS[29.15] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
        bs_write(&pb, 15, (unsigned int)((_ullDts >> 0) & 0x1ffff));//DTS[14..0] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit 1
    }
    if(ESCR_flag == 0x1) {//0b1
        bs_write(&pb, 2, 0);//reserved //2bit
        bs_write(&pb, 3, 0);//ESCR_base[32..30] //3bit
        bs_write(&pb, 1, 1);//marker_bit //1bit
        bs_write(&pb, 15, 0);//ESCR_base[29..15] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit
        bs_write(&pb, 15, 0);//ESCR_base[14.. 0] //15bit
        bs_write(&pb, 1, 1);//marker_bit //1bit
        bs_write(&pb, 9, 0);//ESCR_extension //9bit
        bs_write(&pb, 1, 0);//marker_bit //1bit
    }
    if(ES_rate_flag == 0x1) {//0b1
        bs_write(&pb, 1, 1);//marker_bit //1bit
        bs_write(&pb, 22, 0);//ES_rate //22bit
        bs_write(&pb, 1, 1);//marker_bit //1bit
    }
    if(DSM_trick_mode_flag == 0x1) {//0b1
            bs_write(&pb, 3, trick_mode_control);//trick_mode_control //3bit
        if(trick_mode_control == 0x0) {//0b000
            bs_write(&pb, 2, 0);//field_id //2bit
            bs_write(&pb, 1, 0);//intra_slice_refresh //1bit
            bs_write(&pb, 2, 0);//frequency_truncation //2bit
        }
        else if(trick_mode_control == 0x1) {//0b001
            bs_write(&pb, 5, 0);//field_rep_cntrl //5biy
        }
        else if(trick_mode_control == 0x2) {//0b010
            bs_write(&pb, 2, 0);//field_id //2bit
            bs_write(&pb, 3, 0);//reserved //3bit
        }
        else if(trick_mode_control == 0x3) {//0b011
            bs_write(&pb, 2, 0);//field_Id //2bit
            bs_write(&pb, 1, 0);//intra_slice_refresh //1bit
            bs_write(&pb, 2, 0);//frequency_truncation //2bit
        }
        else if(trick_mode_control == 0x4) {//0b100
            bs_write(&pb, 5, 0);//field_rep_cntrl //5bit
        }
        else
            bs_write(&pb, 5, 0);//reserved //5bit
    }
    if(additional_copy_info_flag == 0x1) {//0b1
        bs_write(&pb, 1, 1);//marker_bit //1bit
        bs_write(&pb, 7, 0);//additional_copy_info //7bit
    }
    if(PES_CRC_flag == 0x1) {//0b1
        bs_write(&pb, 16, 0);//previous_PES_packet_CRC //16bit
    }
    if(PES_extension_flag == 0x1) {//0b1
        bs_write(&pb, 1, 0);//PES_private_data_flag //1bit
        bs_write(&pb, 1, 0);//pack_header_field_flag //1bit
        bs_write(&pb, 1, 0);//program_packet_sequence_counter_flag //1bit
        bs_write(&pb, 1, 0);//P_STD_buffer_flag //1bit
        bs_write(&pb, 3, 0);//reserved //3bit
        bs_write(&pb, 1, PES_extension_flag_2);//PES_extension_flag_2 //1bit
        
        if(PES_private_data_flag == 0x1) {//0b1
            bs_write(&pb, 128, 0);//PES_private_data //128bit
        }
        if(pack_header_field_flag == 0x1) {//0b1
            bs_write(&pb, 8, 0);//pack_field_length //8bit
            //pack_header()
        }
        if(program_packet_sequence_counter_flag == 0x1) {//0b1
            bs_write(&pb, 1, 1);//marker_bit //1bit
            bs_write(&pb, 7, 0);//program_packet_sequence_counter //7bit
            bs_write(&pb, 1, 1);//marker_bit //1bit
            bs_write(&pb, 1, 0);//MPEG1_MPEG2_identifier //1bit
            bs_write(&pb, 6, 0);//original_stuff_length //6bit
        }
        if(P_STD_buffer_flag == 0x1) {//0b1
            bs_write(&pb, 2, 0);//'01' //2bit
            bs_write(&pb, 1, 0);//P_STD_buffer_scale //1bit
            bs_write(&pb, 13, 0);//P_STD_buffer_size //13bit
        }
        if(PES_extension_flag_2 == 0x1) {//0b1
            bs_write(&pb, 1, 1);//marker_bit //1bit
            bs_write(&pb, 7, (unsigned int)PES_extension_field_length);//0 PES_extension_field_length //7bit
            for(i=0;i<PES_extension_field_length;i++) {
                bs_write(&pb, 8, 0);//reserved //8bit
            }
        }
    }
    
    N1 = iStuffingByte;
    for(i=0;i<N1;i++) {
        bs_write(&pb, 8, _pcStuffingByte[i]);//stuffing_byte //8bit
    }
    
    //PES_packet_data_byte //������������

    PES_header_data_length = (unsigned int)(bs_pos(&pb) - bs_pos(&pb_PESHeaderLen)) - 8;
    if(PES_header_data_length & 7)
    {
        Media_Error("PES_header_data_length=%d error\n", PES_header_data_length);
    }
    PES_header_data_length = PES_header_data_length / 8;
    bs_write(&pb_PESHeaderLen, 8, PES_header_data_length);//PES_header_data_length //8bit ֮��ͷ�ĳ���
    //Media_Debug("PES_header_data_length=%d\n", PES_header_data_length);

    
    PES_packet_length = (unsigned int)(bs_pos(&pb) - bs_pos(&pb_PESLen)) - 16;
    if(PES_packet_length & 7)
    {
        Media_Error("PES_packet_length=%d error\n", PES_packet_length);
    }
    PES_packet_length = PES_packet_length / 8;
    bs_write(&pb_PESLen, 16, ((PES_packet_length + _iPayloadLen) >= PES_MAX_LEN ? PES_MAX_LEN : (PES_packet_length + _iPayloadLen)));//PES_packet_length //16bit ֮��ĳ���(ͷ+ ����)  
    //Media_Debug("PES_packet_length=%d\n", PES_packet_length);

    iBits = bs_pos(&pb);
    //Media_Debug("iBits=%d\n", iBits);
    if(iBits & 7)
    {
        Media_Error("iBits=%d error\n", iBits);
    }
    iSize = iBits / 8;
    
    //Media_Debug("iSize - 6=%d\n", iSize - 6);
    
    return iSize;
}



