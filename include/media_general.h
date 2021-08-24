
#ifndef _MEDIAGENERAL_H
#define _MEDIAGENERAL_H

#include <sys/types.h>

#include "media_avcodec.h"


typedef void                    PSHandle;
typedef unsigned long long      PSTimestamp;
typedef unsigned long long      PSDuration;


typedef void                    TSHandle;
typedef unsigned long long      TSTimestamp;
typedef unsigned long long      TSDuration;
#if 0
typedef     unsigned char       uint8_t;        /* Unsigned 8 bit quantity  */
typedef     signed char         int8_t;         /* Signed 8 bit quantity    */
typedef     unsigned short      uint16_t;       /* Unsigned 16 bit quantity */
typedef     signed short        int16_t;        /* Signed 16 bit quantity   */
typedef     unsigned int        uint32_t;       /* Unsigned 32 bit quantity */
typedef     signed int          int32_t;        /* Signed 32 bit quantity   */
typedef     unsigned long long  uint64_t;       /* Unsigned 64 bit quantity */
typedef     signed long long    int64_t;        /* Signed 64 bit quantity   */
typedef     signed long         ssize_t;
#endif
typedef enum AVMediaType {
    AVMEDIA_TYPE_UNKNOWN = -1,  ///< Usually treated as AVMEDIA_TYPE_DATA
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_DATA,          ///< Opaque data information usually continuous
    AVMEDIA_TYPE_SUBTITLE,
    AVMEDIA_TYPE_ATTACHMENT,    ///< Opaque data information usually sparse
    AVMEDIA_TYPE_NB
}AVMediaType;

typedef struct _PsFrameInfo {
    AVCodecID m_eAVCodecID;/*����Ƶ��������h264(AV_CODEC_ID_H264) h265(AV_CODEC_ID_HEVC) MJPEG(AV_CODEC_ID_MJPEG) aac(AV_CODEC_ID_AAC) g711a(AV_CODEC_ID_PCM_ALAW) g711u(AV_CODEC_ID_PCM_MULAW) adpcm(AV_CODEC_ID_ADPCM_IMA_APC) SVC(AV_CODEC_ID_SVC)*/
    PSTimestamp m_TimestampMs;//ʱ�������
    int m_isSyncFrame;// 0����ͬ��֡��1��ͬ��֡
}PsFrameInfo;

typedef struct VideoEs{
    AVCodecID m_eVCodecID;/*����Ƶ��������h264(AV_CODEC_ID_H264) h265(AV_CODEC_ID_HEVC) MJPEG(AV_CODEC_ID_MJPEG) SVC(AV_CODEC_ID_SVC)*/
    unsigned int m_uiFrameRate;
    unsigned int m_uiBitRate;
    unsigned int m_uiWidth;
    unsigned int m_uiHeight;
    int m_iIsSendSEI;
}VideoEs;

typedef struct AudioEs{
    AVCodecID m_eACodecID;/*����Ƶ�������� aac(AV_CODEC_ID_AAC) g711a(AV_CODEC_ID_PCM_ALAW) g711u(AV_CODEC_ID_PCM_MULAW) adpcm(AV_CODEC_ID_ADPCM_IMA_APC) SAC (AV_CODEC_ID_SAC)*/
    unsigned int m_uiSampleRate;
    unsigned int m_uiBitRate;
}AudioEs;

typedef struct DataInfo {
    void *m_pvAddr;
    size_t m_uiLen;
} DataInfo;

typedef struct FrameOutInfo{
    DataInfo *m_pstDI;
    int m_iDICount;
    int m_isEnd;
}FrameOutInfo;

#endif //_MEDIAGENERAL_H

