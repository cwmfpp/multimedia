
#ifndef _MEDIALOG_H_
#define _MEDIALOG_H_

#include <stdio.h>

//#define MEDIA_LOG_SWITCH
#if defined(MEDIA_LOG_SWITCH)

#define Media_Error(pszfmt ,arg...)   
#define Media_Warn(pszfmt ,arg...)    
#define Media_Trace(pszfmt ,arg...)   
#define Media_Debug(pszfmt ,arg...)   

#else
#define Media_Error(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define Media_Warn(fmt, x...)   printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define Media_Trace(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#define Media_Debug(fmt, x...)  printf("%s:%s:%d: " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##x);
#endif

#endif //_MEDIALOG_H


