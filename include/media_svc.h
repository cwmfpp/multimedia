
#ifndef _MEDIASVC_H
#define _MEDIASVC_H

#include "media_video_h264.h"

/* NAL unit types */
enum {
    NAL_SVAC_SLICE           = 1,  //��IDRͼƬ�ı���Ƭ
    NAL_SVAC_IDR_SLICE       = 2,  //IDRͼƬ�ı���Ƭ
    NAL_SVAC_SLICE_EXT       = 3,  //��IDRͼƬ��SVC��ǿ�����Ƭ
    NAL_SVAC_IDR_SLICE_EXT   = 4,  //IDRͼƬ��SVC��ǿ�����Ƭ
    NAL_SVAC_SUR             = 5,  //�����չ���ݵ�Ԫ
    NAL_SVAC_SEI             = 6,  //������ǿ��Ϣ
    NAL_SVAC_SPS             = 7,  //���в�����
    NAL_SVAC_PPS             = 8,  //ͼ�������
    NAL_SVAC_SPS_EXT         = 9,  //��ȫ������
    NAL_SVAC_AUD             = 10, //��֤����
    NAL_SVAC_END_STREAM      = 11, //��β��
    NAL_SVAC_PPS_EXT         = 15, //SVC��ǿ��ͼ�������
};

int SVAC_AnalyzeNalu(unsigned char *_pcVedioBuf, unsigned int _uiVedioBufLen, unsigned int _uiOffSet, NaluInfo* _pstNalu);
int SVAC_Probe(unsigned char *_pData, int _iDataSize);

#endif //_MEDIASVC_H

