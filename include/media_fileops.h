
#ifndef _MEDIAFILEOPS_H
#define _MEDIAFILEOPS_H

void* Media_Fopen(void* _pvFileParam, const char* _pcFileName, const char* _cMode);

int Media_Fread(void *_pvPtr, size_t _iSize, size_t _iNmemb, void *_pvFh);

int Media_Fwrite(const void *_pvPtr, size_t _iSize, size_t _iNmemb, void *_pvFh);

int Media_Fseek(void *_pvFh, off_t _offset, int _iWhence);

off_t Media_Ftell(void *_pvFh);

int Media_Feof(void *_pvFh);

int Media_Fclose(void *_pvFh);

int Media_Fstat(char* _pcFileName, void*_pvBuf);

int Media_Funlink(char* _pcFileName);

#endif //_MEDIAFILEOPS_H
