
#ifndef _MEDIAVIDEOH264_H
#define _MEDIAVIDEOH264_H

/* NAL unit types */
enum {
    NAL_AVCC_SLICE           = 1,
    NAL_AVCC_DPA             = 2,
    NAL_AVCC_DPB             = 3,
    NAL_AVCC_DPC             = 4,
    NAL_AVCC_IDR_SLICE       = 5,
    NAL_AVCC_SEI             = 6,
    NAL_AVCC_SPS             = 7,
    NAL_AVCC_PPS             = 8,
    NAL_AVCC_AUD             = 9,
    NAL_AVCC_END_SEQUENCE    = 10,
    NAL_AVCC_END_STREAM      = 11,
    NAL_AVCC_FILLER_DATA     = 12,
    NAL_AVCC_SPS_EXT         = 13,
    NAL_AVCC_AUXILIARY_SLICE = 19,
    NAL_AVCC_FF_IGNORE       = 0xff0f001,
};

typedef struct
{// NALU
    int m_iType;  
    int m_iVedioSize;  
    unsigned char* m_pucVedioData;  
}NaluInfo;

int H264_AnalyzeNalu(unsigned char* _pcVedioBuf, unsigned int _uiVedioBufLen, unsigned int _uiOffSet, NaluInfo* _pstNalu);

int H264_Probe(unsigned char *_pData, int _iDataSize);

#endif //_MEDIAVIDEOH264_H

