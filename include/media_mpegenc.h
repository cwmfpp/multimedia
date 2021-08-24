
#ifndef _MEDIAMPEGENC_H
#define _MEDIAMPEGENC_H
    
#include "media_general.h"
#include "media_mpegdec.h"


#define PS_HEADER_LEN_MAX           32
#define PS_SYSTEM_HEADER_LEN_MAX    64
#define PS_MAP_HEADER_LEN_MAX       256
#define PS_PES_HEADER_LEN_MAX       64
#define PS_PES_PACKET_NUM           32

#define PACK_START_CODE                 ((unsigned int)0x000001ba)
#define SYSTEM_HEADER_START_CODE        ((unsigned int)0x000001bb)
#define PROGRAM_STREAM_MAP_START_CODE   ((unsigned int)0x000001bc)
#define SEQUENCE_END_CODE               ((unsigned int)0x000001b7)
#define PACKET_START_CODE_MASK          ((unsigned int)0xffffff00)
#define PACKET_START_CODE_PREFIX        ((unsigned int)0x00000100)
#define ISO_11172_END_CODE              ((unsigned int)0x000001b9)

/* mpeg2 */
#define PROGRAM_STREAM_MAP 0x1bc
#define PRIVATE_STREAM_1   0x1bd
#define PADDING_STREAM     0x1be
#define PRIVATE_STREAM_2   0x1bf

#define AUDIO_ID 0xc0
#define VIDEO_ID 0xe0
#define H264_ID  0xe2
#define AC3_ID   0x80
#define DTS_ID   0x88
#define LPCM_ID  0xa0
#define SUB_ID   0x20

#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_HEVC      0x24
#define STREAM_TYPE_VIDEO_MJPEG     0x34
#define STREAM_TYPE_VIDEO_CAVS      0x42
#define STREAM_TYPE_VIDEO_SVC       0x80
#define STREAM_TYPE_AUDIO_PCM_ALAW  0x90
#define STREAM_TYPE_AUDIO_PCM_MULAW 0x91
#define STREAM_TYPE_AUDIO_G722_1    0x92
#define STREAM_TYPE_AUDIO_G723_1    0x93
#define STREAM_TYPE_AUDIO_G729      0x99
#define STREAM_TYPE_AUDIO_SAC       0x9b
#define STREAM_TYPE_AUDIO_ADPCM_IMA_APC     0x9c


#define STREAM_TYPE_AUDIO_AC3       0x81

static const int lpcm_freq_tab[4] = { 48000, 96000, 44100, 32000 };

#define PES_PTS_DTS_0B11   3
#define PES_PTS_DTS_0B10   2
#define PES_PTS_DTS_0B00   0

#define PES_DATA_ALIGNMENT_INDICATOR_START      1   /*NALU�ĵ�һ��pes*/
#define PES_DATA_ALIGNMENT_INDICATOR_CONTINUE   0   /*NALU�ķǵ�һ��pes*/


#define PES_STUFFING_BYTE_VPS_LEN       5
#define PES_STUFFING_BYTE_SPS_LEN       5
#define PES_STUFFING_BYTE_PPS_LEN       3
#define PES_STUFFING_BYTE_IDRS_LEN      4
#define PES_STUFFING_BYTE_IDRC_LEN      3
#define PES_STUFFING_BYTE_P_LEN         4
#define PES_STUFFING_BYTE_G711A_LEN     2
#define PES_STUFFING_BYTE_G711U_LEN     2

#define PES_HEADER_BASE_LEN     6

#define PES_MAX_LEN     0xffd2

#if 0
typedef struct AVCodecParameters {
    /**
     * General type of the encoded data.
     */
    enum AVMediaType m_eCodecType;
    /**
     * Specific type of the encoded data (the codec used).
     */
    enum AVCodecID   m_eCodecId;

    unsigned int m_uiESInfoLen;
    char m_cESInfoData[ELEMENTARY_STREAM_INFO_LEN];
    
    /**
     * Additional information about the codec (corresponds to the AVI FOURCC).
     */
    uint32_t         m_CodecTag;
}AVCodecParameters;
#endif

int Mpeg_PutPackHeader(uint8_t *_pBuf, uint64_t timestamp, unsigned int _uiStuffValue);

int Mpeg_PutSystemHeader(uint8_t *_pBuf);

int Mpeg_InitPsmHeader(PSMHeader *_pPSMHeader);

int Mpeg_SetProgramStreamInfo(PSMHeader *_pPSMHeader, char *_pPSInfo, unsigned int _uiLen);

int Mpeg_GetProgramStreamInfoData(PSMHeader *_pPSMHeader, char **_ppPSInfo, unsigned int *_puiLen);

int Mpeg_GetElementaryStreamData(PSMHeader *_pPSMHeader, int _iESIndex, char **_ppcESInfoData, int *_piESInfoLen);

AVCodecID Mpeg_GetVideoCodecId(PSMHeader *_pPSMHeader);

AVCodecID Mpeg_GetAudioCodecId(PSMHeader *_pPSMHeader);

int Mpeg_SetElementaryStream(PSMHeader *_pPSMHeader, int _iESIndex, AVMediaType _eAVMediaType, AVCodecID _eAVCodecID, char *_pcESInfoData, int _iESInfoLen);

int Mpeg_PutPsmHeader(PSMHeader *_pPSMHeader, uint8_t *_pBuf);

int Mpeg_PutPesHeader(uint8_t *pucBuf, unsigned char _ucStreamId, unsigned char _ucDataAlignIndicator, unsigned int _iPayloadLen, int _PTS_DTS_flags, uint64_t _ullPts, uint64_t _ullDts, int iStuffingByte, uint8_t *_pcStuffingByte);


#endif //_MEDIAMPEGENC_H
