
#include <stdint.h>

#include <stdio.h>

#include "media_video_h265.h"
#include "media_log.h"


int H265_AnalyzeNalu(unsigned char *_pcVedioBuf, unsigned int _uiVedioBufLen, unsigned int _uiOffSet, NaluInfo* _pstNalu)
{  
    unsigned int i = _uiOffSet;  
    while(i<_uiVedioBufLen)  
    {  
        if(_pcVedioBuf[i++] == 0x00 && _pcVedioBuf[i++] == 0x00 && _pcVedioBuf[i++] == 0x00 && _pcVedioBuf[i++] == 0x01)
        {
            unsigned int pos = i;  
            while (pos<_uiVedioBufLen)  
            {  
                if(_pcVedioBuf[pos+0] == 0x00 && _pcVedioBuf[pos+1] == 0x00 && _pcVedioBuf[pos+2] == 0x00 && _pcVedioBuf[pos+3] == 0x01)  
                {  
                    pos += 4;
                    break;  
                }  
                pos = pos + 1;
            }  
            if(pos == _uiVedioBufLen)  
            {  
                _pstNalu->m_iVedioSize = (int)(pos - i);
            }  
            else  
            {  
                _pstNalu->m_iVedioSize = (int)((pos - 4) - i);  
            } 
            _pstNalu->m_iType = _pcVedioBuf[i]&0x7e;
            _pstNalu->m_iType = (_pstNalu->m_iType) >> 1; //�м�6 bit
            _pstNalu->m_pucVedioData =(unsigned char*)&_pcVedioBuf[i];  //nula 2 byte

            return (int)((unsigned int)_pstNalu->m_iVedioSize + i - _uiOffSet);  
        }  
    }  
    return 0;  
}

int H265_Probe(unsigned char *_pData, int _iDataSize)
{
    uint32_t code = 0xffffffff;
    int vps = 0, sps = 0, pps = 0, irap = 0;
    int i;

    if(NULL == _pData)
    {
        return -1;
    }
    for (i = 0; i < _iDataSize - 1; i++) {
        code = (code << 8) + _pData[i];
        if ((code & 0xffffff00) == 0x100) {
            uint8_t nal2 = _pData[i + 1];
            int type = (code & 0x7E) >> 1;

            if (code & 0x81) // forbidden and reserved zero bits
                return 0;

            if (nal2 & 0xf8) // reserved zero
                return 0;

            switch (type) {
            case NAL_VPS:        vps++;  break;
            case NAL_SPS:        sps++;  break;
            case NAL_PPS:        pps++;  break;
            case NAL_BLA_N_LP:
            case NAL_BLA_W_LP:
            case NAL_BLA_W_RADL:
            case NAL_CRA_NUT:
            case NAL_IDR_N_LP:
            case NAL_IDR_W_RADL: irap++; break;
            default:
                break;
            }
        }
    }

    if (vps && sps && pps && irap)
        return 1; // 1 more than .mpg
    return 0;
}

