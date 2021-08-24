
#ifndef _MEDIAPSMUX_H
#define _MEDIAPSMUX_H

#include "media_general.h"


#ifndef API_PUBLIC
#define API_PUBLIC
#endif

#ifdef __cplusplus
extern "C"{
#endif

#define PRIVATE_FLAG    0xF4F3F2F1

typedef struct _PsPrivateAttribute
{
	unsigned short  m_usWidth;//��Ƶ�� 
	unsigned short  m_usHeight; //��Ƶ��
	unsigned int    m_uiSampleRate; //��Ƶ������
	unsigned char   m_ucIsEncrypt; //�Ƿ����,0�����ܣ�1����
	unsigned char   m_ucChannel;//��Ƶ������
	unsigned short  m_usBits;//��Ƶ����λ��
	int m_iIsMkhCCTV;//��������ļ���1��ֱ������ֱ�Ӳ�����0��������Ϊ׼
	int m_iIsSystemHeader;//��������ļ���0����ֱ�Ӳ�����1��������Ϊ׼
}PsPrivateAttribute;

typedef struct _PsInsertHead
{
	unsigned int    m_uiFlag; //TDST
	unsigned int    m_uiSize; //�����ṹ���С
	unsigned short  m_usWidth; 
	unsigned short  m_usHeight; 
	unsigned int    m_uiSampleRate; 
	unsigned char   m_ucIsEncrypt; 
	unsigned char   m_ucChannel;
	unsigned short  m_usBits;
    unsigned int    m_uiTimeStamp;
    unsigned short  m_usFrameRate;	//֡�� zyb����
    unsigned short  m_usReserved;
}PsInsertHead;

PSHandle* API_PUBLIC PS_Create(void);

int API_PUBLIC PS_SetStreamPrivateAttribute(PSHandle *_ph, PsPrivateAttribute *_pstPsPriAttr);

int API_PUBLIC PS_SetInsertHead(PSHandle *_ph, PsInsertHead *_pstPsInsertHead);

int API_PUBLIC PS_SetVideoEs(PSHandle *_ph, VideoEs *_pVideoEs);

int API_PUBLIC PS_SetAudioEs(PSHandle *_ph, AudioEs *_pAudioEs);

int API_PUBLIC PS_PutVideoData(PSHandle *_ph, char * _pcVData, int _iDataLen, PsFrameInfo *_pstPsFrameInfo);

int API_PUBLIC PS_GetVideoPackage(PSHandle *_ph, FrameOutInfo *_pstInfo, void *_pvPri);

int API_PUBLIC PS_PutAudioData(PSHandle *_ph, char * _pcAData, int _iADataLen, PsFrameInfo *_pstPsFrameInfo);

int API_PUBLIC PS_GetAudioPackage(PSHandle *_ph, FrameOutInfo *_pstInfo, void *_pvPri);

int API_PUBLIC PS_SetPrivateData(PSHandle *_ph, void *_pvPri);

void* API_PUBLIC PS_GetPrivateData(PSHandle *_ph);

int API_PUBLIC PS_Destroy(PSHandle *_ph);

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))

#ifdef __cplusplus
};
#endif

#endif //_MEDIAPSMUX_H

