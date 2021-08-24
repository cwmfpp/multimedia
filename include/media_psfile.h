
#ifndef _MEDIAPSFILE_H
#define _MEDIAPSFILE_H

#include "media_general.h"

#ifndef API_PUBLIC
#define API_PUBLIC
#endif

typedef struct FileAttribute{
    AVCodecID m_eACodecID;/*����Ƶ��������aac(AV_CODEC_ID_AAC) g711a(AV_CODEC_ID_PCM_ALAW) g711u(AV_CODEC_ID_PCM_MULAW) adpcm(AV_CODEC_ID_ADPCM_IMA_APC) SVC(AV_CODEC_ID_SAC)*/
    AVCodecID m_eVCodecID;/*����Ƶ��������h264(AV_CODEC_ID_H264) h265(AV_CODEC_ID_HEVC) mjpeg(AV_CODEC_ID_MJPEG) SVC(AV_CODEC_ID_SVC)*/
    PsPrivateAttribute m_stPsPriAttr;
}FileAttribute;

#ifdef __cplusplus
extern "C"{
#endif

void* API_PUBLIC PsFile_Create(const char *_pFileName);

int API_PUBLIC PsFile_SetAttribute(void *_pFile, FileAttribute *_pstFileAttribute);

int API_PUBLIC PsFile_SetInsertHead(void *_pFile, PsInsertHead *_pstPsInsertHead);

int API_PUBLIC PsFile_Write(void *_pFile, PsFrameInfo *_pstPsFrameInfo, char *_pcData, unsigned int _uiLen);

void API_PUBLIC PsFile_Destroy(void *_pFile);

int API_PUBLIC PsFile_IsPSFormat(char *_pFileName);

void* API_PUBLIC PsFile_Open(char *_pFileName);

int API_PUBLIC PsFile_GetMaxFrameLength(void *_pFile);

PSTimestamp API_PUBLIC PsFile_GetStartTime(void *_pFile);

PSTimestamp API_PUBLIC PsFile_GetEndTime(void *_pFile);

int API_PUBLIC PsFile_GetAttribute(void *_pFile, FileAttribute *_pstFileAttribute);

int API_PUBLIC PsFile_GetInsertHead(void *_pFile, PsInsertHead *_pstPsInsertHead);

int API_PUBLIC PsFile_Read(void *_pFile, PsFrameInfo *_pstPsFrameInfo, unsigned char **_ppData, unsigned int *_puiLen);

int API_PUBLIC PsFile_Seek(void *_pFile, PSTimestamp _TimestampMs);

void API_PUBLIC PsFile_Close(void *_pFile);

#ifdef __cplusplus
};
#endif

#endif //_MEDIAPSFILE_H

