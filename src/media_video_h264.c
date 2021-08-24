
#include <stdint.h>

#include <stdio.h>
#include "media_video_h264.h"
#include "media_log.h"

#define flag_success          0 
#define flag_failure          (-1)

int H264_AnalyzeNalu(unsigned char* _pcVedioBuf, unsigned int _uiVedioBufLen, unsigned int _uiOffSet, NaluInfo* _pstNalu)
{
    int iThisFunRes = flag_failure;
    unsigned char* pcVedioBuf = NULL;
    int iVedioBufLen = 0;
    NaluInfo* pstNalu = NULL;
    int i = 0;
    int iPos = 0;
    int iPrevType = 0;
    int iNextType = 0;
    
    if((NULL == _pcVedioBuf) || (NULL == _pstNalu))
    {
        Media_Error("%d: Input parameters is error!\n", __LINE__);
        iThisFunRes = flag_failure;
        goto end;
    }
    
    pcVedioBuf = (unsigned char*)_pcVedioBuf;
    iVedioBufLen = (int)_uiVedioBufLen;
    i = (int)_uiOffSet;
    pstNalu = _pstNalu;
 
    while(i<iVedioBufLen)  
    {  
        if(pcVedioBuf[i++] == 0x00 && pcVedioBuf[i++] == 0x00 && pcVedioBuf[i++] == 0x00 && pcVedioBuf[i++] == 0x01)
        {
            iPos = i;
            iPrevType = pcVedioBuf[iPos] & 0x1f;
            while (iPos<iVedioBufLen)  
            {  
                if(pcVedioBuf[iPos+0] == 0x00 && pcVedioBuf[iPos+1] == 0x00 && pcVedioBuf[iPos+2] == 0x00 && pcVedioBuf[iPos+3] == 0x01)  
                {  
                    iPos += 4;

                    iNextType = pcVedioBuf[iPos] & 0x1f;                        
                    if(iNextType == iPrevType)
                    {
                        continue;
                    }

                    break;  
                }  
                iPos = iPos + 1;
            }  
            
            if(iPos == iVedioBufLen)  
            {  
                pstNalu->m_iVedioSize= iPos-i;
            }  
            else  
            {  
                pstNalu->m_iVedioSize = (iPos-4)-i;  
            }
            pstNalu->m_iType = pcVedioBuf[i] & 0x1f;
            pstNalu->m_pucVedioData =(unsigned char*)&pcVedioBuf[i];    
            iThisFunRes = (pstNalu->m_iVedioSize+i-(int)_uiOffSet); 
            goto end;
        }  
    } 
    
    iThisFunRes = flag_success;
    
end:
    return iThisFunRes;  
}


int H264_Probe(unsigned char *_pData, int _iDataSize)
{
    uint32_t code = (unsigned int)-1;
    int sps = 0, pps = 0, idr = 0, res = 0, sli = 0;
    int i;

    if(NULL == _pData)
    {
        return -1;
    }

    for (i = 0; i < _iDataSize; i++) {
        code = (code << 8) + _pData[i];
        if ((code & 0xffffff00) == 0x100) {
            int ref_idc = (code >> 5) & 3;
            int type    = code & 0x1F;
            static const int8_t ref_zero[] = {
                 2,  0,  0,  0,  0, -1,  1, -1,
                -1,  1,  1,  1,  1, -1,  2,  2,
                 2,  2,  2,  0,  2,  2,  2,  2,
                 2,  2,  2,  2,  2,  2,  2,  2
            };

            if (code & 0x80) // forbidden_bit
                return 0;

            if (ref_zero[type] == 1 && ref_idc)
                return 0;
            if (ref_zero[type] == -1 && !ref_idc)
                return 0;
            if (ref_zero[type] == 2) {
                if (!(code == 0x100 && !_pData[i + 1] && !_pData[i + 2]))
                    res++;
            }

            switch (type) {
            case 1:
                sli++;
                break;
            case 5:
                idr++;
                break;
            case 7:
                if (_pData[i + 2] & 0x03)
                    return 0;
                sps++;
                break;
            case 8:
                pps++;
                break;
            default:
                break;
            }
        }
    }

    if (sps && pps && (idr || sli > 3) && res < (sps + pps + idr))
        return 1;  // 1 more than .mpg

    return 0;
}

