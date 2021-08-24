
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "media_fileops.h"
#include "media_log.h"

void* Media_Fopen(void* _pvFileParam, const char* _pcFileName, const char* _cMode)
{
    void *pvFh = NULL;
    
    if(NULL == _pcFileName || NULL == _cMode)
    {
        Media_Debug("invalid _pcFileName(%p) _cMode(%p)", _pcFileName, _cMode);
        goto end;
    }

    _pvFileParam = _pvFileParam;

    pvFh = (void*) fopen(_pcFileName, _cMode);

end:
    return pvFh;
}

int Media_Fread(void *_pvPtr, size_t _iSize, size_t _iNmemb, void *_pvFh)
{
    int iRet = 0;
    if(NULL == _pvPtr || NULL == _pvFh)
    {
        Media_Debug("invalid _pvPtr(%p) _pvFh(%p)", _pvPtr, _pvFh);
        iRet = -1;
        goto end;
    }
    
    iRet = (int)fread(_pvPtr, _iSize, _iNmemb, (FILE *)_pvFh);

end:
    return iRet;
}

int Media_Fwrite(const void *_pvPtr, size_t _iSize, size_t _iNmemb, void *_pvFh)
{
    int iRet = 0;
    if(NULL == _pvPtr || NULL == _pvFh)
    {
        Media_Debug("invalid _pvPtr(%p) _pvFh(%p)", _pvPtr, _pvFh);
        iRet = -1;
        goto end;
    }

    iRet = (int)fwrite(_pvPtr, _iSize, _iNmemb, (FILE *)_pvFh);
    
end:
    return iRet;
}

int Media_Fseek(void *_pvFh, off_t _offset, int _iWhence)
{
    int iRet = 0;
    if(NULL == _pvFh)
    {
        Media_Debug("invalid _pvFh(%p)", _pvFh);
        iRet = -1;
        goto end;
    }

    iRet = fseeko((FILE*)_pvFh, _offset, _iWhence);
    
end:
    return iRet;
}

off_t Media_Ftell(void *_pvFh)
{
    off_t ret = 0;
    
    if(NULL == _pvFh)
    {
        Media_Debug("invalid _pvFh(%p)", _pvFh);
        ret = -1;
        goto end;
    }

    ret = (off_t)ftello((FILE*)_pvFh);

end:
    return ret;
}

int Media_Feof(void *_pvFh)
{
    int iRet = 0;
    
    if(NULL == _pvFh)
    {
        Media_Debug("invalid _pvFh(%p)", _pvFh);
        iRet = -1;
        goto end;
    }

    iRet = feof((FILE*)_pvFh);

end:
    return iRet;
}

int Media_Fclose(void *_pvFh)
{
    int iRet = 0;
    
    if(NULL == _pvFh)
    {
        Media_Debug("invalid _pvFh(%p)", _pvFh);
        iRet = -1;
        goto end;
    }

    iRet = fclose((FILE*)_pvFh);

end:
    return iRet;
}

int Media_Fstat(char* _pcFileName, void*_pvBuf)
{
    int iRet = 0;
    
    if(NULL == _pcFileName || NULL == _pvBuf)
    {
        Media_Debug("invalid _pcFileName(%p) _pvBuf(%p)", _pcFileName, _pvBuf);
        iRet = -1;
        goto end;
    }

    iRet = stat(_pcFileName, (struct stat*)_pvBuf);
    
end:
    return iRet;
}

int Media_Funlink(char* _pcFileName)
{
    int iRet = 0;
    
    if(NULL == _pcFileName)
    {
        Media_Debug("invalid _pcFileName(%p)", _pcFileName);
        iRet = -1;
        goto end;
    }

    iRet = unlink(_pcFileName);
    
end:
    return iRet;
}

