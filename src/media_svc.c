
#include <stdint.h>
#include <stdio.h>

#include "media_log.h"
#include "media_svc.h"

#define flag_success          0 
#define flag_failure          (-1)
int SVAC_AnalyzeNalu(unsigned char *_pcVedioBuf, unsigned int _uiVedioBufLen, unsigned int _uiOffSet, NaluInfo* _pstNalu)
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
            iPrevType = (pcVedioBuf[iPos] >> 2) & 0x0f;
            while (iPos<iVedioBufLen)  
            {  
                if(pcVedioBuf[iPos+0] == 0x00 && pcVedioBuf[iPos+1] == 0x00 && pcVedioBuf[iPos+2] == 0x00 && pcVedioBuf[iPos+3] == 0x01)  
                {  
                    iPos += 4;

                    iNextType = (pcVedioBuf[iPos] >> 2) & 0x0f;                        
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
            pstNalu->m_iType = (pcVedioBuf[i] >> 2) & 0x0f;
            pstNalu->m_pucVedioData =(unsigned char*)&pcVedioBuf[i];    
            iThisFunRes = (pstNalu->m_iVedioSize+i-(int)_uiOffSet); 
            goto end;
        }  
    } 
    
    iThisFunRes = flag_success;
    
end:
    return iThisFunRes;  
}

int SVAC_Probe(unsigned char *_pData, int _iDataSize)
{
    uint32_t code = 0xffffffff;
    int sps = 0, pps = 0, irap = 0;
    int i;

    if(NULL == _pData)
    {
        return -1;
    }
    for (i = 0; i < _iDataSize - 1; i++) {
        code = (code << 8) + _pData[i];
        if ((code & 0xffffff00) == 0x100) {
            int type = (code >> 2) & 0x0F;

            switch (type) {
            //case NAL_SVAC_SLICE:              vps++;  break;
            case NAL_SVAC_SPS:              sps++;  break;
            case NAL_SVAC_PPS:              pps++;  break;
            case NAL_SVAC_IDR_SLICE_EXT:
            case NAL_SVAC_SUR:
            case NAL_SVAC_SEI:
            case NAL_SVAC_IDR_SLICE:        irap++; break;
            case NAL_SVAC_AUD:
            case NAL_SVAC_END_STREAM:
            case NAL_SVAC_PPS_EXT:  
            default:
                break;
            }
        }
    }

    if (sps && pps && irap)
        return 1; // 1 more than .mpg
    return 0;
}

