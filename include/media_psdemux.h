
#ifndef _MEDIA_PSDEMUX_H
#define _MEDIA_PSDEMUX_H

#include "media_general.h"


#ifdef __cplusplus
extern "C"{
#endif


void *PsDemux_Open(void);
int PsDemux_PutPsData(void *_ph, char * _pcData, int _iDataLen);
int PsDemux_GetRawData(void *_ph, PsFrameInfo *_pstPsFrameInfo, unsigned char **_ppData, unsigned int *_puiLen);
int PsDemux_Close(void *_ph);


#ifdef __cplusplus
};
#endif

#endif //_MEDIA_PSDEMUX_H


