#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "vlc_bits.h"
#include "media_mpegdec.h"
#include "media_mpegenc.h"
#include "media_log.h"
#include "media_fileops.h"

static int avio_open(AVIOContext *s, char *path, const char *mode)
{
    int iRet = 0;
    void *pvFh = NULL;

    if(NULL == s || NULL == path || NULL == mode)
    {
        Media_Error("invalid s(%p) path(%p) mode(%p)", s, path, mode);
        iRet = -1;
        goto end;    
    }
    
    pvFh = Media_Fopen(NULL, path, mode);
    if(NULL == pvFh)
    {
        Media_Error("call Media_Fopen failed, path(%s)", path);
        iRet = -1;
        goto end;
    }
    s->opaque = (void *)pvFh;
    
end:
    return iRet;
}

static int avio_read(AVIOContext *s, unsigned char *buf, int size)
{
    int iRet = 0;
    void *pvFh = NULL;

    if(NULL == s || NULL == buf)
    {
        Media_Error("invalid s(%p) buf(%p)", s, buf);
        iRet = -1;
        goto end;    
    }

    pvFh = s->opaque;
    if(NULL == pvFh)
    {
        Media_Error("invalid pvFh(%p)", pvFh);
        iRet = -1;
        goto end;
    }


    if(size != Media_Fread(buf, 1, (size_t)size, pvFh))
    {
        if(Media_Feof(pvFh))
        {
            Media_Error("file end");
        }
        Media_Error("call Media_Fread failed size(%d)", size);
        iRet = -1;
        goto end;
    }
    
    
end:
    return iRet;
}

/*static int avio_write(AVIOContext *s, unsigned char *buf, int size)
{
    int iRet = 0;
    void *pvFh = NULL;

    if(NULL == s || NULL == buf)
    {
        Media_Error("invalid s(%p) buf(%p)", s, buf);
        iRet = -1;
        goto end;    
    }
    
    pvFh = s->opaque;
    if(NULL == pvFh)
    {
        Media_Error("invalid pvFh(%p)", pvFh);
        iRet = -1;
        goto end;
    }
    
    if(size != Media_Fread(buf, 1, (size_t)size, pvFh))
    {
        if(Media_Feof(pvFh))
        {
            Media_Error("file end");
        }
        Media_Error("call Media_Fread failed size(%d)", size);
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;

}*/

static int avio_close(AVIOContext *s)
{
    int iRet = 0;
    void *pvFh = NULL;

    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        iRet = -1;
        goto end;    
    }

    pvFh = s->opaque;
    if(NULL == pvFh)
    {
        Media_Error("invalid pvFh(%p)", pvFh);
        iRet = -1;
        goto end;
    }

    if(Media_Fclose(pvFh) < 0)
    {
        Media_Error("call Media_Fclose failed");
    }
    s->opaque = NULL;
    
end:
    return iRet;
}

#if 0
static void fill_buffer(AVIOContext *s, int _iSize)
{
    int iRet = 0;
    
#define IO_BUFFER_SIZE 32768

    int max_buffer_size = s->max_packet_size ?
                          s->max_packet_size : IO_BUFFER_SIZE;
    
    if(_iSize >= s->buffer_size)
    {
        Media_Error("size(%d) >= (s->buf_end - s->buf_ptr)(%d)", size, s->buf_end - s->buf_ptr);
        iRet = -1;
        goto end;
    }
    
    if(avio_read(s, s->buffer, _iSize) < 0)
    {
        Media_Error("call avio_read failed _iSize(%d)", _iSize);
        iRet = -1;
        goto end;
    }

    s->buf_ptr = s->buffer;
    
end:
    return iRet;
}
#endif

/* XXX: put an inline version */
static unsigned int avio_r8(AVIOContext *s)
{
    unsigned int value = 0;
    
    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        goto end;    
    }
    
    if(avio_read(s,s->buffer, 1) < 0)
    {
        Media_Error("call avio_read failed 1");
        goto end;
    }
    #if 0
    if (s->buf_ptr >= s->buf_end)
    {
        fill_buffer(s, 1);
    }
    
    if (s->buf_ptr < s->buf_end)
    {
        return *s->buf_ptr++;
    }
    #endif

    value = *s->buffer;
    
end:
    return value;
}

#if 0
int avio_read(AVIOContext *s, unsigned char *buf, int size)
{
    int len, size1;

    size1 = size;
    #if 0
    while (size > 0) {
        len = FFMIN(s->buf_end - s->buf_ptr, size);
        if (len == 0 || s->write_flag) {
            if((s->direct || size > s->buffer_size) && !s->update_checksum) {
                // bypass the buffer and read data directly into buf
                if(s->read_packet)
                    len = s->read_packet(s->opaque, buf, size);

                if (len <= 0) {
                    /* do not modify buffer if EOF reached so that a seek back can
                    be done without rereading data */
                    s->eof_reached = 1;
                    if(len<0)
                        s->error= len;
                    break;
                } else {
                    s->pos += len;
                    s->bytes_read += len;
                    size -= len;
                    buf += len;
                    // reset the buffer
                    s->buf_ptr = s->buffer;
                    s->buf_end = s->buffer/* + len*/;
                }
            } else {
                fill_buffer(s);
                len = s->buf_end - s->buf_ptr;
                if (len == 0)
                    break;
            }
        } else {
            memcpy(buf, s->buf_ptr, len);
            buf += len;
            s->buf_ptr += len;
            size -= len;
        }
    }
    if (size1 == size) {
        if (s->error)      return s->error;
        if (avio_feof(s))  return AVERROR_EOF;
    }
    #endif
    return size1 - size;
}
#endif


static unsigned int avio_rb16(AVIOContext *s)
{
    unsigned int val;
    val = avio_r8(s) << 8;
    val |= avio_r8(s);
    return val;
}

/*static unsigned int avio_rb24(AVIOContext *s)
{
    unsigned int val;
    val = avio_rb16(s) << 8;
    val |= avio_r8(s);
    return val;
}*/
static unsigned int avio_rb32(AVIOContext *s)
{
    unsigned int val;
    val = avio_rb16(s) << 16;
    val |= avio_rb16(s);
    return val;
}

static int64_t avio_seek(AVIOContext *s, int64_t offset, int whence)
{
    int iRet = 0;
    void *pvFh = NULL;

    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        iRet = -1;
        goto end;    
    }

    pvFh = s->opaque;

    iRet = Media_Fseek(pvFh, (off_t)offset, whence);
    if(iRet < 0)
    {
        Media_Error("call Media_Fseek failed");
        iRet = -1;
        goto end; 

    }
    /*cwm:todo*/
    #if 0
    int64_t offset1;
    int64_t pos;
    int force = whence & AVSEEK_FORCE;
    int buffer_size;
    whence &= ~AVSEEK_FORCE;

    if(!s)
        return AVERROR(EINVAL);

    buffer_size = s->buf_end - s->buffer;
    // pos is the absolute position that the beginning of s->buffer corresponds to in the file
    pos = s->pos - (s->write_flag ? 0 : buffer_size);

    if (whence != SEEK_CUR && whence != SEEK_SET)
        return AVERROR(EINVAL);

    if (whence == SEEK_CUR) {
        offset1 = pos + (s->buf_ptr - s->buffer);
        if (offset == 0)
            return offset1;
        offset += offset1;
    }
    if (offset < 0)
        return AVERROR(EINVAL);

    offset1 = offset - pos; // "offset1" is the relative offset from the beginning of s->buffer
    if (!s->must_flush && (!s->direct || !s->seek) &&
        offset1 >= 0 && offset1 <= buffer_size - s->write_flag) {
        /* can do the seek inside the buffer */
        s->buf_ptr = s->buffer + offset1;
    } else if ((!s->seekable ||
               offset1 <= buffer_size + s->short_seek_threshold) &&
               !s->write_flag && offset1 >= 0 &&
               (!s->direct || !s->seek) &&
              (whence != SEEK_END || force)) {
        while(s->pos < offset && !s->eof_reached)
            fill_buffer(s);
        if (s->eof_reached)
            return AVERROR_EOF;
        s->buf_ptr = s->buf_end - (s->pos - offset);
    } else if(!s->write_flag && offset1 < 0 && -offset1 < buffer_size>>1 && s->seek && offset > 0) {
        int64_t res;

        pos -= FFMIN(buffer_size>>1, pos);
        if ((res = s->seek(s->opaque, pos, SEEK_SET)) < 0)
            return res;
        s->buf_end =
        s->buf_ptr = s->buffer;
        s->pos = pos;
        s->eof_reached = 0;
        fill_buffer(s);
        return avio_seek(s, offset, SEEK_SET | force);
    } else {
        int64_t res;
        if (s->write_flag) {
            flush_buffer(s);
            s->must_flush = 1;
        }
        if (!s->seek)
            return AVERROR(EPIPE);
        if ((res = s->seek(s->opaque, offset, SEEK_SET)) < 0)
            return res;
        s->seek_count ++;
        if (!s->write_flag)
            s->buf_end = s->buffer;
        s->buf_ptr = s->buffer;
        s->pos = offset;
    }
    s->eof_reached = 0;
    #endif
end:

    return offset;
}


static int64_t avio_skip(AVIOContext *s, int64_t offset)
{
    int64_t i64Seek = 0;
    
    /*cwm:todo*/
    i64Seek = avio_seek(s, offset, SEEK_CUR);
    
    return i64Seek;//avio_seek(s, offset, SEEK_CUR);
}

static int64_t avio_tell(AVIOContext *s)
{
    /*cwm:todo*/
    void *pvFh = NULL;
    int64_t llpos = 0;
    
    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        llpos = -1;
        goto end; 
    }

    pvFh = s->opaque;

    llpos = Media_Ftell(pvFh);

end:
    
    return llpos;
}

static int avio_feof(AVIOContext *s)
{
    /*cwm:todo*/
    int iRet = 0;
    void *pvFh = NULL;
    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        iRet = -1;
        goto end; 
    }

    pvFh = s->opaque;
    iRet = Media_Feof(pvFh);

end:

    return iRet;
}


#   define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])



#   define AV_RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])
                



static AVStream *avformat_new_stream(AVFormatContext *s, const void/*AVCodec*/ *c)
{
    AVStream *st = NULL;
    int i = 0;

    #if 0
    AVStream **streams = NULL;
    if (s->nb_streams >= FFMIN(s->max_streams, INT_MAX/sizeof(*streams))) {
        if (s->max_streams < INT_MAX/sizeof(*streams))
            av_log(s, AV_LOG_ERROR, "Number of streams exceeds max_streams parameter (%d), see the documentation if you wish to increase it\n", s->max_streams);
        return NULL;
    }
    streams = av_realloc_array(s->streams, s->nb_streams + 1, sizeof(*streams));
    if (!streams)
        return NULL;
    s->streams = streams;
    st = av_mallocz(sizeof(AVStream));
    if (!st)
        return NULL;
    #endif

    c = c;
    
    for(i = 0; i < (int)(sizeof(s->streams) / sizeof(s->streams[0])); i++)
    {
        if(-1 == s->streams[i].index)
        {
            st = s->streams + i;
            Media_Debug("find available AVStream(%d)", i);
            break;
        }
    }
    if(NULL == st)
    {
        Media_Error("not available AVStream");
        goto end;
    }

    #if 0
    if (!(st->info = av_mallocz(sizeof(*st->info)))) {
        av_free(st);
        return NULL;
    }
    #endif
    /*cwm: infoʹ������*/
    st->info->last_dts = AV_NOPTS_VALUE;

#if 0
#if FF_API_LAVF_AVCTX
FF_DISABLE_DEPRECATION_WARNINGS
    st->codec = avcodec_alloc_context3(c);
    if (!st->codec) {
        av_free(st->info);
        av_free(st);
        return NULL;
    }
FF_ENABLE_DEPRECATION_WARNINGS
#endif
#endif

    /*cwm: todo*/
    #if 0
    st->internal = av_mallocz(sizeof(*st->internal));
    if (!st->internal)
        goto fail;
    #endif
    
    /*cwm: codecparʹ������*/
    #if 0
    st->codecpar = avcodec_parameters_alloc();
    if (!st->codecpar)
        goto fail;
    #endif
    
    /*cwm: todo*/
    #if 0
    st->internal->avctx = avcodec_alloc_context3(NULL);
    if (!st->internal->avctx)
        goto fail;
    #endif

    /*cwm: todo*/
    #if 0
    if (0/*s->iformat*/) {
        #if 0
#if FF_API_LAVF_AVCTX
FF_DISABLE_DEPRECATION_WARNINGS
        /* no default bitrate if decoding */
        st->codec->bit_rate = 0;
FF_ENABLE_DEPRECATION_WARNINGS
#endif

        /* default pts setting is MPEG-like */
        avpriv_set_pts_info(st, 33, 1, 90000);
        /* we set the current DTS to 0 so that formats without any timestamps
         * but durations get some timestamps, formats with some unknown
         * timestamps have their first few packets buffered and the
         * timestamps corrected before they are returned to the user */
        st->cur_dts = RELATIVE_TS_BASE;
        #endif
    } else {
        st->cur_dts = AV_NOPTS_VALUE;
    }
    #endif
    
    st->index      = i;//s->nb_streams;/*cwm:todo ʹ�����������*/
    st->start_time = AV_NOPTS_VALUE;
    st->duration   = AV_NOPTS_VALUE;
    st->first_dts     = AV_NOPTS_VALUE;
    st->probe_packets = MAX_PROBE_PACKETS;
    st->pts_wrap_reference = AV_NOPTS_VALUE;
    st->pts_wrap_behavior = AV_PTS_WRAP_IGNORE;

    st->last_IP_pts = AV_NOPTS_VALUE;
    st->last_dts_for_order_check = AV_NOPTS_VALUE;
    for (i = 0; i < MAX_REORDER_DELAY + 1; i++)
        st->pts_buffer[i] = AV_NOPTS_VALUE;

    /*cwm:todo*/
    #if 0
    st->sample_aspect_ratio = (AVRational) { 0, 1 };
    #endif

    /*cwm:todo*/
#if 0/*FF_API_R_FRAME_RATE*/
    st->info->last_dts      = AV_NOPTS_VALUE;
#endif
    st->info->fps_first_dts = AV_NOPTS_VALUE;
    st->info->fps_last_dts  = AV_NOPTS_VALUE;

    /*cwm:todo*/
    #if 0
    st->inject_global_side_data = s->internal->inject_global_side_data;

    st->internal->need_context_update = 1;
    #endif
    
    /*cwm:todo st��Դ��&(s->streams[i])*/
    #if 0
    s->streams[s->nb_streams++] = st;
    #endif

end:
    return st;
    
    #if 0
fail:
    free_stream(&st);
    return NULL;
    #endif
}


static void av_init_packet(AVPacket *pkt)
{
    pkt->pts                  = AV_NOPTS_VALUE;
    pkt->dts                  = AV_NOPTS_VALUE;
    pkt->pos                  = -1;
    pkt->duration             = 0;
/*cwm:todo*/
#if 0/*FF_API_CONVERGENCE_DURATION*/
FF_DISABLE_DEPRECATION_WARNINGS
    pkt->convergence_duration = 0;
FF_ENABLE_DEPRECATION_WARNINGS
#endif
    pkt->flags                = 0;
    pkt->stream_index         = 0;
    /*cwm:todo*/
    #if 0
    pkt->buf                  = NULL;
    pkt->side_data            = NULL;
    #endif
    pkt->side_data_elems      = 0;
}

static int av_get_packet(AVIOContext *s, AVPacket *pkt, int size)
{
    av_init_packet(pkt);
    if(size > (int)sizeof(pkt->m_ucBuffer))
    {
        Media_Error("size(%d) > sizeof(pkt->m_ucBuffer)(%ld)", size, sizeof(pkt->m_ucBuffer));
        size = -1;
        goto end;
    }
    
    pkt->data = pkt->m_ucBuffer;//NULL;
    pkt->size = 0;
    pkt->pos  = avio_tell(s);

    Media_Debug("read size(%d)", size);
    if(avio_read(s, pkt->data, size) < 0)
    {
        Media_Error("call avio_read failed, size(%d)", size);
        size = -1;
        goto end;
    }
    pkt->size = size;
    Media_Debug("read size(%d) ok", size);
end:
    return size;
}


/**
* Parse MPEG-PES five-byte timestamp
*/
static inline int64_t ff_parse_pes_pts(const uint8_t *buf) {
  return (int64_t)(*buf & 0x0e) << 29 |
          ((unsigned short)AV_RB16(buf+1) >> 1) << 15 |
           (unsigned short)AV_RB16(buf+3) >> 1;
}

//mpeg.c

static int64_t get_pts(AVIOContext *pb, int c)
{
    uint8_t buf[5];

    buf[0] = (uint8_t)(c < 0 ? avio_r8(pb) : (unsigned int)c);
    if(avio_read(pb, buf + 1, 4) < 0)
    {
        Media_Error("call avio_read failed");
    }

    return ff_parse_pes_pts(buf);
}

/*
*��ȡ����0x000001��ͷ�Ŀ�ʼ�룬һ���ĸ��ֽڣ�����0x000001ba��0x000001bb��0x000001bc��0x000001e0��0x000001c0
*
*
*/
static int find_next_start_code(AVIOContext *pb, int *size_ptr,
                                int32_t *header_state)
{
    unsigned int state = 0;
    unsigned int v = 0;
    int val = 0;
    int n = 0;

    state = (unsigned int)(*header_state);
    n     = *size_ptr;
    while (n > 0) {
        if (avio_feof(pb))
            break;
        v = avio_r8(pb);
        n--;
        //Media_Debug("n=%d state=0x%x v=0x%x", n, state, v);
        if (state == 0x000001) {
            Media_Debug("find ok n=%d state=0x%x v=0x%x", n, state, v);
            if(0 == v)
            {
                Media_Debug("find ok n=%d state=0x%x v=0x%x", n, state, v);
                state = (unsigned int)(*header_state);
                Media_Debug("find ok n=%d state=0x%x v=0x%x", n, state, v);
                state = ((state << 8)/* | v*/) & 0xffffff;/*cwm:todo state = ((state << 8) | v) & 0xffffff*/
                Media_Debug("find ok n=%d state=0x%x v=0x%x", n, state, v);
                continue;
            }
            state = ((state << 8) | v) & 0xffffff;
            val   = (int)state;
            goto found;
        }
        state = ((state << 8) | v) & 0xffffff;
    }
    val = -1;

found:
    *header_state = (int)state;
    *size_ptr     = n;
    return val;
}


static int FindStartCode(AVIOContext *pb, int64_t _i64FilePos, int64_t _i64FileSize, int _iStartCode, int64_t *_pi64StartCodePos)
{
    int32_t header_state = 0xff;
    unsigned int state = 0;
    unsigned int v = 0;
    int val = 0;
    int n = 0;
    int64_t i64RawPos = -1;
    unsigned char cBufferTmp[65536] = {0};
    int iBufferIndex = 0;
    int iBufferLen = sizeof(cBufferTmp);
    int iReadIndex = 0;

    if(NULL == pb || NULL == _pi64StartCodePos)
    {
        Media_Error("invalid pb(%p) _pi64StartCodePos(%p)", pb, _pi64StartCodePos);
        goto end;
    }

    if(_i64FilePos >= _i64FileSize)
    {
        Media_Error("invalid _i64FilePos(0x%lx) >= _i64FileSize(0x%lx)", _i64FilePos, _i64FileSize);
        goto end;
    }
    
    i64RawPos = avio_tell(pb);
    
    Media_Debug("i64RawPos=%ld", i64RawPos);
    
    Media_Debug("_i64FilePos=%ld", _i64FilePos);
    if(avio_seek(pb, _i64FilePos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        goto end;
    }
    
    iBufferIndex = iBufferLen;
    state = (unsigned int)header_state;
    n     = 3 * (1 << 20);

    while (n > 0) {
        if((_i64FilePos + iReadIndex) >= _i64FileSize)
        {
            Media_Error("read file end, (_i64FilePos + iReadIndex)(%ld) >= _i64FileSize(%ld)", _i64FilePos + iReadIndex, _i64FileSize);
            break;
        }

        if(iBufferIndex == iBufferLen)
        {
            int iReadLen = 0;

            if((_i64FileSize - (_i64FilePos + iReadIndex)) > iBufferLen)
            {
                iReadLen = iBufferLen;
                Media_Debug("iReadLen=%d", iReadLen);
            }else
            {
                iReadLen = (int)(_i64FileSize - (_i64FilePos + (int64_t)iReadIndex));
                Media_Debug("iReadLen=%d", iReadLen);
            }
            
            Media_Debug("iReadLen=%d", iReadLen);
            if(avio_read(pb, cBufferTmp, iReadLen) < 0)
            {
                Media_Error("call avio_read failed");
            }
            iBufferIndex = 0;
        }
        
        v = cBufferTmp[iBufferIndex];
        n--;
        iBufferIndex++;
        iReadIndex++;
        //Media_Debug("iBufferIndexn=%d iReadIndex=%d", iBufferIndex, iReadIndex);
        
        //Media_Debug("n=%d state=0x%x v=0x%x", n, state, v);
        if (state == 0x000001) {
            Media_Debug("find ok n=%d state=0x%x v=0x%x _i64FilePos + iReadIndex=(0x%lx)", n, state, v, _i64FilePos + iReadIndex);
            if(_iStartCode != (int)v)
            {
                state = (unsigned int)header_state;
                state = ((state << 8)/* | v*/) & 0xffffff;/*cwm:todo state = ((state << 8) | v) & 0xffffff*/
                Media_Debug("find ok n=%d state=0x%x v=0x%x", n, state, v);
                continue;
            }
            state = ((state << 8) | v) & 0xffffff;
            val   = (int)state;
            goto found;
        }
        state = ((state << 8) | v) & 0xffffff;
    }

end:
    
    val = -1;

found:
    
    Media_Debug("find ok n=%d state=0x%x v=0x%x iReadIndex=%d", n, state, v, iReadIndex);
    if (NULL != pb && -1 != i64RawPos && avio_seek(pb, i64RawPos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
    }

    if(NULL != _pi64StartCodePos)
    {
        if(-1 != val)
        {
            *_pi64StartCodePos = (_i64FilePos + iReadIndex) > 4 ? (_i64FilePos + iReadIndex) - 4 : _i64FilePos + iReadIndex;
            Media_Debug("find ok *_pi64StartCodePos(0x%lx) _iStartCode(0x%x)", *_pi64StartCodePos, _iStartCode);
        }else
        {
            *_pi64StartCodePos = _i64FilePos;
        }
    }
    
    header_state = (int)state;
    
    return val;
}

static AVCodecID GetCodecIdByEsType(int _esType)
{
    AVCodecID res = (AVCodecID)0;

    switch(_esType)
    {
        case STREAM_TYPE_VIDEO_MPEG4:
            res = AV_CODEC_ID_MPEG4;
            break;
        case STREAM_TYPE_VIDEO_H264:
            res = AV_CODEC_ID_H264;
            break;
        case STREAM_TYPE_VIDEO_HEVC:
            res = AV_CODEC_ID_HEVC;
            break;
        case STREAM_TYPE_VIDEO_MJPEG:
            res = AV_CODEC_ID_MJPEG;
            break;
        case STREAM_TYPE_VIDEO_SVC:
            res = AV_CODEC_ID_SVC;
            break;
        case STREAM_TYPE_AUDIO_PCM_ALAW:
            res = AV_CODEC_ID_PCM_ALAW;
            break;
        case STREAM_TYPE_AUDIO_PCM_MULAW:
            res = AV_CODEC_ID_PCM_MULAW;
            break;
        case STREAM_TYPE_AUDIO_G722_1:
            res = AV_CODEC_ID_G722_1;
            break;
        case STREAM_TYPE_AUDIO_G723_1:
            res = AV_CODEC_ID_G723_1;
            break;
        case STREAM_TYPE_AUDIO_G729:
            res = AV_CODEC_ID_G729;
            break;
        case STREAM_TYPE_AUDIO_SAC:
            res = AV_CODEC_ID_SAC;
            break;
        case STREAM_TYPE_AUDIO_AAC:
            res = AV_CODEC_ID_AAC;
            break;
        case STREAM_TYPE_AUDIO_ADPCM_IMA_APC:
            res = AV_CODEC_ID_ADPCM_IMA_APC;
            break;
        default:
            break;
    }
    
    return res;
}

/**
 * Extract stream types from a program stream map
 * According to ISO/IEC 13818-1 ('MPEG-2 Systems') table 2-35
 *
 * @return number of bytes occupied by PSM in the bitstream
 */
static long mpegps_psm_parse(MpegDemuxContext *m, AVIOContext *pb)
{
    int iRet = 0;
    int psm_length = 0;
    int ps_info_length = 0;
    int es_map_length = 0;
    int iIsGetPsm = 0;
    unsigned char ucBufferStreamInfo[256] = {0};
    PSMHeader *pstPSMHeader = NULL;
    
    psm_length = (int)avio_rb16(pb);
    if(psm_length <= 0)
    {
        iRet = -1;
        Media_Error("invalid psm_length(%d)", psm_length);
        goto end;
    }
    (void)avio_r8(pb);
    (void)avio_r8(pb);
    ps_info_length = (int)avio_rb16(pb);
    #if 0
    if(ps_info_length <= 0)
    {
        iRet = -1;
        Media_Error("invalid ps_info_length(%d)", ps_info_length);
        goto end;
    }
    #endif
    iIsGetPsm = m->m_iIsGetPsm;
    pstPSMHeader = &m->m_stPSMHeaderDec;
    if(ps_info_length > 0)
    {
        if(0 == iIsGetPsm)
        {
            if(avio_read(pb, ucBufferStreamInfo, ps_info_length) < 0)
            {
                Media_Error("call avio_read failed");
            }
            
            if(Mpeg_SetProgramStreamInfo(pstPSMHeader, (char *)ucBufferStreamInfo, (unsigned int)ps_info_length) < 0)
            {
                Media_Error("call Mpeg_InitPsmHeader failed!");
            }
        }else
        {
            /* skip program_stream_info */
            (void)avio_skip(pb, (int64_t)ps_info_length);
        }
    }
    
    
    /*es_map_length = */(void)avio_rb16(pb);
    /* Ignore es_map_length, trust psm_length */
    es_map_length = (psm_length - ps_info_length) - 10;
    if(es_map_length <= 0)
    {
        iRet = -1;
        Media_Error("invalid es_map_length(%d)", es_map_length);
        goto end;
    }
    
    Media_Debug("es_map_length=%d", es_map_length);

    /* at least one es available? */
    while (es_map_length >= 4) {
        unsigned char type      = (unsigned char)avio_r8(pb);
        unsigned char es_id     = (unsigned char)avio_r8(pb);
        Media_Debug("type=%d", type);
        Media_Debug("es_id=%d", es_id);
        uint16_t es_info_length = (uint16_t)avio_rb16(pb);

        /* remember mapping from stream id to stream type */
        m->psm_es_type[es_id] = type;
        if(0 == iIsGetPsm)
        {
            Media_Debug("read es");
            AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
            
            if(avio_read(pb, ucBufferStreamInfo, es_info_length) < 0)
            {
                Media_Error("call avio_read failed");
            }

            eAVCodecID = GetCodecIdByEsType(type);
            if(VIDEO_ID == es_id)
            {
                Media_Debug("read video es");
                if(Mpeg_SetElementaryStream(pstPSMHeader, 0, AVMEDIA_TYPE_VIDEO, eAVCodecID, (char *)ucBufferStreamInfo, es_info_length) < 0)
                {
                    Media_Error("call Mpeg_InitPsmHeader failed!");
                }
            }else if(AUDIO_ID == es_id)
            {
                Media_Debug("read audio es");
                if(Mpeg_SetElementaryStream(pstPSMHeader, 1, AVMEDIA_TYPE_AUDIO, eAVCodecID, (char *)ucBufferStreamInfo, es_info_length) < 0)
                {
                    Media_Error("call Mpeg_InitPsmHeader failed!");
                }
            }
            
            //m->m_iIsGetPsm = 1;
            
        }else
        {
            Media_Debug("skip es");
            /* skip program_stream_info */
            (void)avio_skip(pb, (int64_t)es_info_length);
        }
        es_map_length -= 4 + es_info_length;
    }
    (void)avio_rb32(pb); /* crc32 */
    iRet = 2 + psm_length;

end:
    return iRet;
}

/* read the next PES header. Return its position in ppos
 * (if not NULL), and its start code, pts and dts.
 */
 /*
*
*����ֵΪ���صĳ��ȣ���������ͷ
*
*/
static int mpegps_read_pes_header(AVFormatContext *s,
                                  int64_t *ppos, int *pstart_code,
                                  int64_t *ppts, int64_t *pdts)
{    
    MpegDemuxContext *m = &s->m_stMDemux;//s->priv_data; /*cwm:todo*/
    int len = 0;
    int size = 0;
    int startcode = 0;
    int c = 0;
    int flags = 0;
    int header_len = 0;
    int pes_ext = 0;
    int ext2_len = 0;
    int id_ext = 0;
    int skip = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t last_sync = 0;
    
    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        goto end;
    }
    
    last_sync = avio_tell(s->pb);
    Media_Debug("last_sync=%ld", last_sync);
    s->m_iIsPsm = 0;
    
error_redo:
    (void)avio_seek(s->pb, last_sync, SEEK_SET);
redo:
    /* next start code (should be immediately after) */
    m->header_state = 0xff;
    size      = MAX_SYNC_SIZE;
    startcode = find_next_start_code(s->pb, &size, &m->header_state);/*��ȡ����0x000001��־��4�ֽڿ�ʼ��*/
    last_sync = avio_tell(s->pb);/*��¼��ʼ�����һ���ֽ������ļ���λ��*/
    Media_Debug("startcode=0x%x", startcode);
    if (startcode < 0) {
        if (avio_feof(s->pb))
        {
            Media_Debug("startcode=0x%x read file end", startcode);
            return AVERROR_EOF;
        }
        // FIXME we should remember header_state
        return FFERROR_REDO;
    }
    
    Media_Debug("startcode=0x%x", startcode);

    if(0x1e0 == s->m_iStartcodePrev && 0x1e0 != startcode)
    {
        //video end
        int64_t nextStart = 0;
        
        nextStart = last_sync;
        if(nextStart > 4)
        {
            nextStart = nextStart - 4;
        }
        (void)avio_seek(s->pb, nextStart, SEEK_SET);
        s->m_iVideoEnd = 1;
        len = 0;
        s->m_iStartcodePrev = 0xff;
        *pstart_code = 0x1e0;/*cwm:todo �ж�st->id == startcodeʱ�����Եõ�����������Ϣ*/
        goto end;
    }    
    
    s->m_iStartcodePrev = startcode;
    s->m_iVideoEnd = 0;

    if ((unsigned int)startcode == PACK_START_CODE)
    {
        Media_Debug("startcode=0x%x", startcode);
        goto redo;
    }
    if ((unsigned int)startcode == SYSTEM_HEADER_START_CODE)
    {
        Media_Debug("startcode=0x%x", startcode);
        goto redo;
    }
    if (startcode == PADDING_STREAM) {
        Media_Debug("startcode=0x%x", startcode);
        int len1 = 0;
        len1 = (int)avio_rb16(s->pb);
        if(len1 <= 0)
        {
            (void)avio_skip(s->pb, (int64_t)-2);
            Media_Error("len1=%d", len1);
            goto redo;
        }
        (void)avio_skip(s->pb, (int64_t)avio_rb16(s->pb));
        goto redo;
    }
    if (startcode == PRIVATE_STREAM_2) {
        Media_Debug("startcode=0x%x, m->sofdec=%d", startcode, m->sofdec);
		m->sofdec = 0;
		Media_Debug("startcode=0x%x, m->sofdec=%d", startcode, m->sofdec);
        if (!m->sofdec) {
            /* Need to detect whether this from a DVD or a 'Sofdec' stream */
            int len1 = 0;
            len1 = (int)avio_rb16(s->pb);
            if(len1 == 0)
            {
                (void)avio_skip(s->pb, (int64_t)-2);
                Media_Debug("len1=%d", len1);
                goto redo;
            }
            int bytesread = 0;
            uint8_t *ps2buf = (uint8_t *)av_malloc((unsigned int)len1);

            if (ps2buf) {
                bytesread = avio_read(s->pb, ps2buf, len1);
                //INT64_MIN
                if (bytesread != len1) {
                    int iSeek = 0;

                    if(len1 >= 0 && bytesread >= 0)
                    {
                        iSeek = (int)(len1 - bytesread);
                    }
                    
                    (void)avio_skip(s->pb, (int64_t)iSeek);
                } else {
                    uint8_t *p = 0;
                    if (len1 >= 6)
                        p = (uint8_t *)memchr(ps2buf, 'S', (unsigned int)(len1 - 5));

                    if (p)
                        m->sofdec = !memcmp(p+1, "ofdec", 5);

                    m->sofdec -= (m->sofdec == 0 ? 1 : 0);

                    if (m->sofdec < 0) {
                        if (len1 == 980  && ps2buf[0] == 0) {
                            /* PCI structure? */
                            uint32_t startpts = AV_RB32(ps2buf + 0x0d);
                            uint32_t endpts = AV_RB32(ps2buf + 0x11);
                            uint8_t hours = ((ps2buf[0x19] >> 4) * 10) + (ps2buf[0x19] & 0x0f);
                            uint8_t mins  = ((ps2buf[0x1a] >> 4) * 10) + (ps2buf[0x1a] & 0x0f);
                            uint8_t secs  = ((ps2buf[0x1b] >> 4) * 10) + (ps2buf[0x1b] & 0x0f);

                            m->dvd = (hours <= 23 &&
                                      mins  <= 59 &&
                                      secs  <= 59 &&
                                      (ps2buf[0x19] & 0x0f) < 10 &&
                                      (ps2buf[0x1a] & 0x0f) < 10 &&
                                      (ps2buf[0x1b] & 0x0f) < 10 &&
                                      endpts >= startpts);
                        } else if (len1 == 1018 && ps2buf[0] == 1) {
                            /* DSI structure? */
                            uint8_t hours = ((ps2buf[0x1d] >> 4) * 10) + (ps2buf[0x1d] & 0x0f);
                            uint8_t mins  = ((ps2buf[0x1e] >> 4) * 10) + (ps2buf[0x1e] & 0x0f);
                            uint8_t secs  = ((ps2buf[0x1f] >> 4) * 10) + (ps2buf[0x1f] & 0x0f);

                            m->dvd = (hours <= 23 &&
                                      mins  <= 59 &&
                                      secs  <= 59 &&
                                      (ps2buf[0x1d] & 0x0f) < 10 &&
                                      (ps2buf[0x1e] & 0x0f) < 10 &&
                                      (ps2buf[0x1f] & 0x0f) < 10);
                        }
                    }
                }

                av_free(ps2buf);

                /* If this isn't a DVD packet or no memory
                 * could be allocated, just ignore it.
                 * If we did, move back to the start of the
                 * packet (plus 'length' field) */
                int iSkip = 0;

                if(len1 > 0 && ((0x7FFFFFFF - 2) > len1))
                {
                    iSkip = -(len1 + 2);
                }
                if (!m->dvd || avio_skip(s->pb, (int64_t)iSkip/*(-(len1 + 2))*/) < 0) {
                    /* Skip back failed.
                     * This packet will be lost but that can't be helped
                     * if we can't skip back
                     */
                    Media_Warn("len1=%d", len1);
                    goto redo;
                }
            } else {
                /* No memory */
                (void)avio_skip(s->pb, (int64_t)len1);
                goto redo;
            }
        } else if (!m->dvd) {
            int len2 = 0;
            len2 = (int)avio_rb16(s->pb);
            (void)avio_skip(s->pb, (int64_t)len2);
            goto redo;
        }
    }
    if (startcode == PROGRAM_STREAM_MAP) {
        Media_Debug("startcode=0x%x", startcode);
        int64_t psmPos = avio_tell(s->pb);
        Media_Debug("mpegps_psm_parse psmPos=%ld", psmPos);
        if(mpegps_psm_parse(m, s->pb) < 0)
        {
            (void)avio_seek(s->pb, psmPos, SEEK_SET);
            Media_Error("seek psmPos(%ld) for parse psm failed!", psmPos);
        }else
        {
            s->m_iIsPsm = 1;
            s->m_i64PsmPos = psmPos;
        }
        goto redo;
    }

    /* find matching stream */
    if (!((startcode >= 0x1c0 && startcode <= 0x1df) ||
          (startcode >= 0x1e0 && startcode <= 0x1ef) ||
          (startcode == 0x1bd) ||
          (startcode == PRIVATE_STREAM_2) ||
          (startcode == 0x1fd)))
    {
        Media_Debug("startcode=0x%x", startcode);
        goto redo;
    }
    if (ppos) {
        *ppos = avio_tell(s->pb) - 4;
    }
    len = (int)avio_rb16(s->pb);//PES_packet_length
    pts =
    dts = AV_NOPTS_VALUE;
    if (startcode != PRIVATE_STREAM_2)
    {
        /* stuffing */
        for (;;) {
            if (len < 1)
            {
                Media_Debug("len=%d", len);
                s->m_iStartcodePrev = 0xff;
                goto error_redo;
            }
            c = (int)avio_r8(s->pb);/*cwm:2bit reserved +2bit PES_scrambling_control + 1bit PES_priority + 1bit data_alignment_indicator + 1bit copyright + 1bit original_or_copy*/
            len--;/*cwm:ȥ���Ѷ�һ���ֽڳ���*/
            /* XXX: for MPEG-1, should test only bit 7 */
            if (c != 0xff)
                break;
        }
        if ((c & 0xc0) == 0x40) {
            /* buffer scale & size */
            /*cwm:��mpeg 2*/
            (void)avio_r8(s->pb);
            c    = (int)avio_r8(s->pb);
            len -= 2;
        }
        if ((c & 0xe0) == 0x20) {
            /*cwm:��mpeg 2*/
            dts  =
            pts  = get_pts(s->pb, c);
            len -= 4;
            if (c & 0x10) {
                dts  = get_pts(s->pb, -1);
                len -= 5;
            }
        } else if ((c & 0xc0) == 0x80) {
            /*cwm:mpeg 2*/
            /* mpeg 2 PES */
            flags      = (int)avio_r8(s->pb);/*cwm:2bit PTS_DTS_flags + 1bit ESCR_flag + 1bit ES_rate_flag + 1bit DSM_trick_mode_flag + 1bit additional_copy_info_flag + 1bit PES_CRC_flag + 1bit PES_extension_flag*/
            header_len = (int)avio_r8(s->pb);/*cwm:8bit PES_header_data_length*/
            len       -= 2;
            if (header_len > len)
                goto error_redo;
            len -= header_len;
            /*cwm:��ȡʱ���*/
            if (flags & 0x80) {
                dts         = pts = get_pts(s->pb, -1);
                header_len -= 5;
                if (flags & 0x40) {
                    dts         = get_pts(s->pb, -1);
                    header_len -= 5;
                }
            }
            if (flags & 0x3f && header_len == 0) {
                flags &= 0xC0;
                Media_Warn( "Further flags set but no bytes left\n");
            }
            if (flags & 0x01) { /* PES extension */
                pes_ext = (int)avio_r8(s->pb);
                header_len--;
                /* Skip PES private data, program packet sequence counter
                 * and P-STD buffer */
                skip  = ((unsigned int)pes_ext >> 4) & 0xb;
                skip += skip & 0x9;
                if (pes_ext & 0x40 || skip > header_len) {
                    Media_Warn( "pes_ext %X is invalid\n", pes_ext);
                    pes_ext = skip = 0;
                }
                (void)avio_skip(s->pb, (int64_t)skip);
                header_len -= skip;

                if (pes_ext & 0x01) { /* PES extension 2 */
                    ext2_len = (int)avio_r8(s->pb);
                    header_len--;
                    if ((ext2_len & 0x7f) > 0) {
                        id_ext = (int)avio_r8(s->pb);
                        if ((id_ext & 0x80) == 0)
                            startcode = ((startcode & 0xff) << 8) | id_ext;
                        header_len--;
                    }
                }
            }
            if (header_len < 0)
                goto error_redo;
            (void)avio_skip(s->pb, (int64_t)header_len);
        } else if (c != 0xf)
        {
            goto redo;
        }
    }

    if (startcode == PRIVATE_STREAM_1) {
        Media_Debug("startcode=0x%x", startcode);
        //startcode = avio_r8(s->pb);
        (void)avio_r8(s->pb);
        len--;
        Media_Debug("startcode=0x%x", startcode);
    }
    if (len < 0) {
        goto error_redo;
    }
    /*cwm:����ȥ��todo*/
    /*cwm:todo*/
    #if 0
    if (dts != AV_NOPTS_VALUE && ppos) {
        int i;
        for (i = 0; i < s->nb_streams; i++) {
            if (startcode == s->streams[i]->id &&
                s->pb->seekable /* index useless on streams anyway */) {
                ff_reduce_index(s, i);
                av_add_index_entry(s->streams[i], *ppos, dts, 0, 0,
                                   AVINDEX_KEYFRAME /* FIXME keyframe? */);
            }
        }
    }
    #endif
    
    *pstart_code = startcode;
    *ppts        = pts;
    *pdts        = dts;

end:
    
    return len;
}

/*
*��ȡ������ͷ����ȡ��ԭʼ������
*
*
*
*/
static int mpegps_read_packet(AVFormatContext *s,
                              AVPacket *pkt)
{
    MpegDemuxContext *m = &s->m_stMDemux;//s->priv_data; /*cwm:todo*/
    AVStream *st = NULL;
    int len = 0;
    int startcode = 0;
    int i = 0;
    int es_type = 0;
    int ret = 0;
    int lpcm_header_len = -1; //Init to suppress warning
    int request_probe= 0;
    enum AVCodecID codec_id = AV_CODEC_ID_NONE;
    enum AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t dummy_pos = 0;; // dummy_pos is needed for the index building to work

redo:
    len = mpegps_read_pes_header(s, &dummy_pos, &startcode, &pts, &dts);/*cwm:len�������ݵĳ��ȣ�startcode 4�ֽڿ�ʼ��*/
    if (len < 0)
    {
        Media_Error("call mpegps_read_pes_header failed! len=%d", len);
        return len;
    }

    if (startcode >= 0x80 && startcode <= 0xcf) {
        if (len < 4)
            goto skip;

        /* audio: skip header */
        (void)avio_r8(s->pb);
        lpcm_header_len = (int)avio_rb16(s->pb);
        len -= 3;
        if (startcode >= 0xb0 && startcode <= 0xbf) {
            /* MLP/TrueHD audio has a 4-byte header */
            (void)avio_r8(s->pb);
            len--;
        }
    }

    s->nb_streams = sizeof(s->streams) / sizeof(s->streams[0]);
    /* now find stream */
    for (i = 0; i < (int)s->nb_streams; i++) {
        st = &(s->streams[i]);/*cwm:todo �Ӷ�άst = s->streams[i];��һάst = &(s->streams[i]);*/
        if (st->id == startcode)
        {/*����Ѵ���ֱ������������*/
            Media_Debug("found st->id(0x%x) == startcode(0x%x)", st->id, startcode);
            es_type = m->psm_es_type[startcode & 0xff];
            codec_id = GetCodecIdByEsType(es_type);
            if(st->codecpar->codec_id == codec_id)
            {
                Media_Debug("found st->id(0x%x) == startcode(0x%x)", st->id, startcode);
                goto found;
            }else
            {
                Media_Debug("audio or video codec type is changed, reload codec type st->codecpar->codec_id(%d) = codec_id(%d)", st->codecpar->codec_id, codec_id);
                st->codecpar->codec_id = codec_id;
            }
        }
    }

    /*��mpegps_psm_parse�и�ֵ*/
    es_type = m->psm_es_type[startcode & 0xff];
    Media_Debug("es_type(0x%x)", es_type);
    if (es_type == STREAM_TYPE_VIDEO_MPEG1) {
        codec_id = AV_CODEC_ID_MPEG2VIDEO;
        type     = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AV_CODEC_ID_MPEG2VIDEO");
    } else if (es_type == STREAM_TYPE_VIDEO_MPEG2) {
        codec_id = AV_CODEC_ID_MPEG2VIDEO;
        type     = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AV_CODEC_ID_MPEG2VIDEO");
    } else if (es_type == STREAM_TYPE_AUDIO_MPEG1 ||
               es_type == STREAM_TYPE_AUDIO_MPEG2) {
        codec_id = AV_CODEC_ID_MP3;
        type     = AVMEDIA_TYPE_AUDIO;
        Media_Debug("AV_CODEC_ID_MP3");
    } else if (es_type == STREAM_TYPE_AUDIO_AAC) {
        codec_id = AV_CODEC_ID_AAC;
        type     = AVMEDIA_TYPE_AUDIO;
        Media_Debug("AV_CODEC_ID_AAC");
    } else if (es_type == STREAM_TYPE_VIDEO_MPEG4) {
        codec_id = AV_CODEC_ID_MPEG4;
        type     = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AV_CODEC_ID_MPEG4");
    } else if (es_type == STREAM_TYPE_VIDEO_H264) {
        codec_id = AV_CODEC_ID_H264;
        type     = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AV_CODEC_ID_H264");
    } else if (es_type == STREAM_TYPE_VIDEO_SVC) {
        codec_id = AV_CODEC_ID_SVC;
        type     = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AV_CODEC_ID_SVC");
    } else if (es_type == STREAM_TYPE_VIDEO_HEVC) {
        codec_id = AV_CODEC_ID_HEVC;
        type     = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AV_CODEC_ID_HEVC");
    } else if (es_type == STREAM_TYPE_VIDEO_MJPEG) {
        codec_id = AV_CODEC_ID_MJPEG;
        type     = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AV_CODEC_ID_MJPEG");
    }  else if (es_type == STREAM_TYPE_AUDIO_AC3) {
        codec_id = AV_CODEC_ID_AC3;
        type     = AVMEDIA_TYPE_AUDIO;
        Media_Debug("AV_CODEC_ID_AC3");
    }  else if (es_type == STREAM_TYPE_AUDIO_SAC) {
        codec_id = AV_CODEC_ID_SAC;
        type     = AVMEDIA_TYPE_AUDIO;
        Media_Debug("AV_CODEC_ID_AC3");
    } else if (m->imkh_cctv && es_type == 0x91) {
        codec_id = AV_CODEC_ID_PCM_MULAW;
        type     = AVMEDIA_TYPE_AUDIO;
        Media_Debug("AV_CODEC_ID_PCM_MULAW");
    } else if (startcode >= 0x1e0 && startcode <= 0x1ef) {
        static const unsigned char avs_seqh[4] = { 0, 0, 1, 0xb0 };
        unsigned char buf[8];

        if(avio_read(s->pb, buf, 8) < 0)
        {
            Media_Error("call avio_read failed!");
        }
        (void)avio_seek(s->pb, (int64_t)-8, SEEK_CUR);
        if (!memcmp(buf, avs_seqh, 4) && (buf[6] != 0 || buf[7] != 1))
            codec_id = AV_CODEC_ID_CAVS;
        else
            request_probe= 1;
        type = AVMEDIA_TYPE_VIDEO;
        Media_Debug("AVMEDIA_TYPE_VIDEO");
    } else if (startcode == PRIVATE_STREAM_2) {
        type = AVMEDIA_TYPE_DATA;
        codec_id = AV_CODEC_ID_DVD_NAV;
        Media_Debug("AV_CODEC_ID_DVD_NAV");
    } else if (startcode >= 0x1c0 && startcode <= 0x1df) {
        type     = AVMEDIA_TYPE_AUDIO;
            Media_Debug("es_type=%d", es_type);
        if (m->sofdec > 0) {
            Media_Debug("es_type=%d", es_type);
            codec_id = AV_CODEC_ID_ADPCM_ADX;
            // Auto-detect AC-3
            request_probe = 50;
        } else if (m->imkh_cctv && startcode == 0x1c0 && len > 80) {
            Media_Debug("es_type=%d", es_type);
            codec_id = AV_CODEC_ID_PCM_ALAW;
            request_probe = 50;
        } else {
            Media_Debug("es_type=%d", es_type);
            codec_id = GetCodecIdByEsType(es_type);/*cwm:mod def AV_CODEC_ID_MP2*/;
            Media_Debug("codec_id=%d", codec_id);
            if (m->imkh_cctv)
                request_probe = 25;
        }
    } else if (startcode >= 0x80 && startcode <= 0x87) {
        type     = AVMEDIA_TYPE_AUDIO;
        codec_id = AV_CODEC_ID_AC3;
        Media_Debug("AV_CODEC_ID_AC3");
    } else if ((startcode >= 0x88 && startcode <= 0x8f) ||
               (startcode >= 0x98 && startcode <= 0x9f)) {
        /* 0x90 - 0x97 is reserved for SDDS in DVD specs */
        type     = AVMEDIA_TYPE_AUDIO;
        codec_id = AV_CODEC_ID_DTS;
        Media_Debug("AV_CODEC_ID_DTS");
    } else if (startcode >= 0xa0 && startcode <= 0xaf) {
        type     = AVMEDIA_TYPE_AUDIO;
        if (lpcm_header_len == 6 || startcode == 0xa1) {
            codec_id = AV_CODEC_ID_MLP;
        } else {
            codec_id = AV_CODEC_ID_PCM_DVD;
        }
    } else if (startcode >= 0xb0 && startcode <= 0xbf) {
        type     = AVMEDIA_TYPE_AUDIO;
        codec_id = AV_CODEC_ID_TRUEHD;
        Media_Debug("AV_CODEC_ID_TRUEHD");
    } else if (startcode >= 0xc0 && startcode <= 0xcf) {
        /* Used for both AC-3 and E-AC-3 in EVOB files */
        type     = AVMEDIA_TYPE_AUDIO;
        codec_id = AV_CODEC_ID_AC3;
        Media_Debug("AV_CODEC_ID_AC3");
    } else if (startcode >= 0x20 && startcode <= 0x3f) {
        type     = AVMEDIA_TYPE_SUBTITLE;
        codec_id = AV_CODEC_ID_DVD_SUBTITLE;
        Media_Debug("AV_CODEC_ID_DVD_SUBTITLE");
    } else if (startcode >= 0xfd55 && startcode <= 0xfd5f) {
        type     = AVMEDIA_TYPE_VIDEO;
        codec_id = AV_CODEC_ID_VC1;
        Media_Debug("AV_CODEC_ID_VC1");
    } else {
skip:
        Media_Warn("skip len(%d)", len);
        /* skip packet */
        (void)avio_skip(s->pb, (int64_t)len);
        goto redo;
    }
    Media_Debug("startcode=0x%x", startcode);
    if(codec_id <= 0)
    {
        Media_Debug("codec_id(0x%x) <= 0", codec_id);
        goto skip;
    }
    /* no stream found: add a new stream */
    st = avformat_new_stream(s, NULL);
    if (!st)
    {
        Media_Error("call avformat_new_stream failed");
        goto skip;
    }
    st->id                = startcode;
    st->codecpar->codec_type = type;
    st->codecpar->codec_id   = codec_id;
    if (   st->codecpar->codec_id == AV_CODEC_ID_PCM_MULAW
        || st->codecpar->codec_id == AV_CODEC_ID_PCM_ALAW) {
        st->codecpar->channels = 1;
        st->codecpar->channel_layout = AV_CH_LAYOUT_MONO;
        st->codecpar->sample_rate = 8000;
    }
    st->request_probe     = request_probe;
    st->need_parsing      = AVSTREAM_PARSE_FULL;
    st->discard = AVDISCARD_DEFAULT;
    Media_Debug("st->codecpar->codec_type=%d", st->codecpar->codec_type);
    Media_Debug("st->codecpar->codec_id=%d", st->codecpar->codec_id);

found:
    if (st->discard >= AVDISCARD_ALL)
    {
        Media_Debug("st->discard(%d) >= AVDISCARD_ALL(%d)", st->discard, AVDISCARD_ALL);
        goto skip;
    }
    if (startcode >= 0xa0 && startcode <= 0xaf) {
      if (st->codecpar->codec_id == AV_CODEC_ID_MLP) {
            if (len < 6)
                goto skip;
            (void)avio_skip(s->pb, 6LL);
            len -=6;
      }
    }
    
    int64_t readPos = avio_tell(s->pb);
    Media_Debug("readPos=%ld", readPos);
    Media_Debug("len=%d", len);
    ret = av_get_packet(s->pb, pkt, len);

    pkt->pts          = pts;
    pkt->dts          = dts;
    pkt->pos          = dummy_pos;
    pkt->stream_index = st->index;

    Media_Debug("dummy_pos=%ld", dummy_pos);
    
    readPos = avio_tell(s->pb);
    Media_Debug("readPos=%ld", readPos);
    readPos = readPos;
    /*cwm:todo*/
    #if 0
    if (s->debug & FF_FDEBUG_TS)
        av_log(s, AV_LOG_TRACE, "%d: pts=%0.3f dts=%0.3f size=%d\n",
            pkt->stream_index, pkt->pts / 90000.0, pkt->dts / 90000.0,
            pkt->size);
    #endif
    
    return (ret < 0) ? ret : 0;
}

#if 0

static int InitSyncFrame(SyncFrame *_pstSyncFrame, int64_t _i64Pos, int64_t _i64Timestamp)
{
    int iRet = 0;
    if(NULL == _pstSyncFrame)
    {
        Media_Error("invalid _pstSyncFrame(%p)", _pstSyncFrame);
        iRet = -1;
        goto end;
    }

    _pstSyncFrame->m_i64Pos = _i64Pos;
    _pstSyncFrame->m_i64Timestamp = _i64Timestamp;
    
end:

    return iRet;
}

static int InitSyncFrameInfo(SyncFrameInfo *_pstSyncFrameInfo)
{
    int iRet = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        iRet = -1;
        goto end;
    }

    _pstSyncFrameInfo->m_i64StartTime = 0;
    _pstSyncFrameInfo->m_i64EndTime = 0;
    _pstSyncFrameInfo->m_i64FileSize = 0;
    _pstSyncFrameInfo->m_iMaxFrameLenV = 0;
    
end:

    return iRet;

}
#endif

static int UninitSyncFrameInfo(SyncFrameInfo *_pstSyncFrameInfo)
{
    int iRet = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        iRet = -1;
        goto end;
    }

    _pstSyncFrameInfo->m_i64StartTime = 0;
    _pstSyncFrameInfo->m_i64EndTime = 0;
    _pstSyncFrameInfo->m_i64FileSize = 0;
    _pstSyncFrameInfo->m_iMaxFrameLenV = 0;
    
end:

    return iRet;

}



static int64_t GetSyncFrameInfoStartTime(SyncFrameInfo *_pstSyncFrameInfo)
{
    int64_t i64StartTime = -1;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        goto end;
    }

    i64StartTime = _pstSyncFrameInfo->m_i64StartTime;
    
end:
    
    return i64StartTime;
}

static int64_t SetSyncFrameInfoStartTime(SyncFrameInfo *_pstSyncFrameInfo, int64_t _i64StartTime)
{
    int iRet = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        iRet = -1;
        goto end;
    }

    _pstSyncFrameInfo->m_i64StartTime = (_i64StartTime / 90) * 90;
    Media_Debug("_pstSyncFrameInfo->m_i64StartTime=%ld", _pstSyncFrameInfo->m_i64StartTime);
    
end:
    
    return iRet;
}

static int64_t GetSyncFrameInfoEndTime(SyncFrameInfo *_pstSyncFrameInfo)
{
    int64_t i64EndTime = -1;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        goto end;
    }

    i64EndTime = _pstSyncFrameInfo->m_i64EndTime;
    Media_Debug("i64EndTime=%ld", i64EndTime);
    
end:
    
    return i64EndTime;
}

static int64_t SetSyncFrameInfoEndTime(SyncFrameInfo *_pstSyncFrameInfo, int64_t _i64EndTime)
{
    int iRet = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        iRet = -1;
        goto end;
    }

    _pstSyncFrameInfo->m_i64EndTime = _i64EndTime;
    Media_Debug("_pstSyncFrameInfo->m_i64EndTime=%ld", _pstSyncFrameInfo->m_i64EndTime);
    
end:
    
    return iRet;
}

static int SetSyncFrameInfoMaxFrameLength(SyncFrameInfo *_pstSyncFrameInfo, int _iMaxFrameLenV)
{
    int iRet = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        iRet = -1;
        goto end;
    }

    _pstSyncFrameInfo->m_iMaxFrameLenV = _iMaxFrameLenV;
    Media_Debug("_pstSyncFrameInfo->m_iMaxFrameLenV=%d", _pstSyncFrameInfo->m_iMaxFrameLenV);
    
end:
    
    return iRet;
}

static int GetSyncFrameInfoMaxFrameLength(SyncFrameInfo *_pstSyncFrameInfo)
{
    int iRet = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        iRet = -1;
        goto end;
    }    

    iRet = _pstSyncFrameInfo->m_iMaxFrameLenV + _pstSyncFrameInfo->m_iMaxFrameLenV / 5 + 4096;
    Media_Debug("_pstSyncFrameInfo->m_iMaxFrameLenV=%d", _pstSyncFrameInfo->m_iMaxFrameLenV);
    
end:
    
    return iRet;
}


static int SetSyncFrameInfoFileSize(SyncFrameInfo *_pstSyncFrameInfo, int64_t _i64FileSize)
{
    int iRet = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        iRet = -1;
        goto end;
    }

    _pstSyncFrameInfo->m_i64FileSize = _i64FileSize;
    Media_Debug("_pstSyncFrameInfo->m_i64FileSize=%ld", _pstSyncFrameInfo->m_i64FileSize);
    
end:
    
    return iRet;
}



static int64_t GetSyncFrameInfoFileSize(SyncFrameInfo *_pstSyncFrameInfo)
{
    int64_t i64Ret = 0;
    if(NULL == _pstSyncFrameInfo)
    {
        Media_Error("invalid _pstSyncFrameInfo(%p)", _pstSyncFrameInfo);
        i64Ret = -1;
        goto end;
    }    

    i64Ret = _pstSyncFrameInfo->m_i64FileSize;
    Media_Debug("_pstSyncFrameInfo->m_i64FileSize=%ld", _pstSyncFrameInfo->m_i64FileSize);
    
end:
    
    return i64Ret;
}

#if 0

static int MpegPsReadSyncFrameInfo(AVFormatContext *s, SyncFrameInfo * _pstSyncFrameInfo)
{
    int iRet = 0;
    int length = 0;
    int startcode = 0;
    int64_t pos = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t dtsEnd = 0;
    int iFrameSize = 0;
    int iFrameSizeMax = 0;

    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        iRet = -1;
        goto end;
    }

    if (avio_seek(s->pb, 0LL, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }

    if(InitSyncFrameInfo(_pstSyncFrameInfo) < 0)
    {
        Media_Error("call InitSyncFrameInfo failed");
        iRet = -1;
        goto end;
    }

    
    for (;;) {
        length = mpegps_read_pes_header(s, &pos, &startcode, &pts, &dts);
        if (length < 0) {
            Media_Error("none length=%d\n", length);
            iRet = -1;
            if(1 == avio_feof(s->pb))
            {
                iRet = 0;
                Media_Warn("avio_feof true");
            }
            break;
        }

        if(1 == s->m_iIsPsm && dts != AV_NOPTS_VALUE)
        {
            Media_Debug("pos=%ld, dts=%ld", pos, dts);
            dtsEnd = dts;
            if(0 == GetSyncFrameInfoStartTime(_pstSyncFrameInfo))
            {
                if(SetSyncFrameInfoStartTime(_pstSyncFrameInfo, dts) < 0)
                {
                    Media_Error("call SetSyncFrameInfoStartTime failed! dts(%ld)", dts);

                }
            }
            
        }

        iFrameSize += length;
        if(1 == s->m_iVideoEnd)
        {
            Media_Debug("iFrameSize=%d", iFrameSize);
            if(iFrameSize > iFrameSizeMax)
            {
                iFrameSizeMax = iFrameSize;
                Media_Debug("iFrameSizeMax=%d", iFrameSizeMax);
            }
            iFrameSize = 0;
        }
        
        Media_Debug("length=%d", length);
        (void)avio_skip(s->pb, (int64_t)length);
    }
     
    if(SetSyncFrameInfoMaxFrameLength(_pstSyncFrameInfo, iFrameSizeMax) < 0)
    {
        Media_Error("call SetSyncFrameInfoMaxFrameLength failed! iFrameSizeMax(%d)", iFrameSizeMax);
    }
    
    if(0 == GetSyncFrameInfoEndTime(_pstSyncFrameInfo))
    {
        if(SetSyncFrameInfoEndTime(_pstSyncFrameInfo, dtsEnd) < 0)
        {
            Media_Error("call SetSyncFrameInfoEndTime failed! dtsEnd(%ld)", dtsEnd);
        }
    }

end:
    
    if (NULL != s && avio_seek(s->pb, 0LL, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }

    Media_Debug("avio_tell=%ld", avio_tell(s->pb));
    
    Media_Debug("pos=%ld, dts=%ld", pos, dts);
    
    return iRet;
}
#endif

static int MpegCalcFrameSize(AVFormatContext *s)
{
    int iRet = 0;
    int length = 0;
    int startcode = 0;
    int startcodeFirst = 0;
    int64_t pos = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int iFrameSize = 0;
    int64_t i64RawPos = -1;
    int iFindFirstIFrame = 0;
    
    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        iRet = -1;
        goto end;
    }

    i64RawPos = avio_tell(s->pb);
    
    if (avio_seek(s->pb, 0LL, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }
    
    s->m_iStartcodePrev = 0xff;
    for (;;) {
        length = mpegps_read_pes_header(s, &pos, &startcode, &pts, &dts);
        if (length < 0) {
            Media_Error("none length=%d\n", length);
            iRet = -1;
            if(1 == avio_feof(s->pb))
            {
                iRet = 0;
                Media_Warn("avio_feof true");
            }
            break;
        }

        if(1 == s->m_iIsPsm && dts != AV_NOPTS_VALUE)
        {
            Media_Debug("pos=0x%lx, dts=%ld", pos, dts);
            if(0 == iFindFirstIFrame)
            {
                iFindFirstIFrame = 1;
                startcodeFirst = startcode;
                Media_Debug("find IFrame");
            }            
        }

        if(1 == iFindFirstIFrame)
        {
            iFrameSize += length;
            Media_Debug("iFrameSize=%d", iFrameSize);
            if (startcodeFirst != startcode || 1 == s->m_iVideoEnd)
            {
                Media_Debug("end iFrameSize=%d startcodeFirst=0x%x startcode=0x%x s->m_iVideoEnd=%d", iFrameSize, startcodeFirst, startcode, s->m_iVideoEnd);
                break;
            }
        }
        
        Media_Debug("length=%d", length);
        (void)avio_skip(s->pb, (int64_t)length);
    }
    
    iRet = iFrameSize;
    s->m_iStartcodePrev = 0xff;

    if (-1 != i64RawPos && avio_seek(s->pb, i64RawPos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }

end:


    Media_Debug("avio_tell=%ld", avio_tell(s->pb));
    
    Media_Debug("pos=0x%lx, dts=%ld", pos, dts);
    
    return iRet;
}


static int MpegGetFileSize(AVFormatContext *s, int64_t *_pi64FileSize)
{
    int iRet = 0;
    int64_t i64RawPos = -1;
    int64_t i64FileSize = 0;
    
    if(NULL == s || NULL == _pi64FileSize)
    {
        Media_Error("invalid s(%p) _pi64FileSize(%p)", s, _pi64FileSize);
        iRet = -1;
        goto end;
    }

    i64RawPos = avio_tell(s->pb);

    if (avio_seek(s->pb, 0LL, SEEK_END) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }

    i64FileSize = avio_tell(s->pb);

    if(i64FileSize < 0)
    {
        Media_Error("call avio_tell failed");
        iRet = -1;
        goto end;
    }
    
    *_pi64FileSize = i64FileSize;
    
end:
    
    if (NULL != s && -1 != i64RawPos && avio_seek(s->pb, i64RawPos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }
    
    return iRet;
}

static int MpegSeekByFilePos(AVFormatContext *s, int64_t _i64FilePos, SyncFrame *_pstSyncFrame, int _iIsSync)
{
    int iRet = 0;
    int length = 0;
    int startcode = 0;
    int64_t pos = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t i64RawPos = -1;
    
    if(NULL == s || NULL == _pstSyncFrame)
    {
        Media_Error("invalid s(%p) _pstSyncFrame(%p)", s, _pstSyncFrame);
        iRet = -1;
        goto end;
    }

    i64RawPos = avio_tell(s->pb);
    
    Media_Debug("_i64FilePos=0x%lx", _i64FilePos);
    if (avio_seek(s->pb, _i64FilePos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }
    
    for (;;) {
        s->m_iStartcodePrev = 0xff;
        length = mpegps_read_pes_header(s, &pos, &startcode, &pts, &dts);
        if (length < 0) {
            Media_Error("none length=%d\n", length);
            iRet = -1;
            if(1 == avio_feof(s->pb))
            {
                iRet = 0;
                Media_Warn("avio_feof true");
            }
            break;
        }
        
        Media_Debug("pos=0x%lx, dts=%ld s->m_iIsPsm =%d", pos, dts, s->m_iIsPsm);

        if(dts != AV_NOPTS_VALUE)
        {
            if(0 == _iIsSync)
            {
                Media_Debug("pos=0x%lx, dts=%ld s->m_iIsPsm=%d", pos, dts, s->m_iIsPsm);

                _pstSyncFrame->m_i64Pos = pos;
                _pstSyncFrame->m_i64Timestamp = dts;
                break;
            }else
            {
                if(1 == s->m_iIsPsm)
                {
                    Media_Debug("pos=0x%lx, dts=%ld s->m_iIsPsm=%d", pos, dts, s->m_iIsPsm);

                    _pstSyncFrame->m_i64Pos = pos;
                    _pstSyncFrame->m_i64Timestamp = dts;
                    break;
                }
            }
            
        }
       
        Media_Debug("length=%d", length);
        (void)avio_skip(s->pb, (int64_t)length);
    }
    
    s->m_iStartcodePrev = 0xff;

end:

    if (NULL != s && -1 != i64RawPos && avio_seek(s->pb, i64RawPos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }

    Media_Debug("avio_tell=%ld", avio_tell(s->pb));
    
    Media_Debug("pos=0x%lx, dts=%ld", pos, dts);
    
    return iRet;
}

static int MpegFindEndTime(AVFormatContext *s, int64_t _i64FilePos, SyncFrame *_pstSyncFrame)
{
    int iRet = 0;
    int length = 0;
    int startcode = 0;
    int64_t pos = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t i64RawPos = -1;
    
    if(NULL == s || NULL == _pstSyncFrame)
    {
        Media_Error("invalid s(%p) _pstSyncFrame(%p)", s, _pstSyncFrame);
        iRet = -1;
        goto end;
    }

    i64RawPos = avio_tell(s->pb);
    
    if (avio_seek(s->pb, _i64FilePos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }
    
    s->m_iStartcodePrev = 0xff;
    for (;;) {
        length = mpegps_read_pes_header(s, &pos, &startcode, &pts, &dts);
        if (length < 0) {
            Media_Error("none length=%d\n", length);
            iRet = -1;
            if(1 == avio_feof(s->pb))
            {
                iRet = 0;
                Media_Warn("avio_feof true");
            }
            break;
        }
        
        Media_Debug("pos=0x%lx, dts=%ld s->m_iIsPsm =%d", pos, dts, s->m_iIsPsm);

        if(dts != AV_NOPTS_VALUE)
        {
            Media_Debug("pos=0x%lx, dts=%ld", pos, dts);
            _pstSyncFrame->m_i64Pos = pos;
            _pstSyncFrame->m_i64Timestamp = dts;            
        }
        
        if(1 == avio_feof(s->pb))
        {
            iRet = 0;
            Media_Warn("avio_feof true");
            break;
        }
        
        Media_Debug("length=%d", length);
        (void)avio_skip(s->pb, (int64_t)length);
    }
    
    s->m_iStartcodePrev = 0xff;

end:

    if (NULL != s && -1 != i64RawPos && avio_seek(s->pb, i64RawPos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed");
        iRet = -1;
        goto end;
    }

    Media_Debug("avio_tell=%ld", avio_tell(s->pb));
    
    Media_Debug("pos=0x%lx, dts=%ld", pos, dts);
    
    return iRet;
}

static int CalcFileSyncFrameInfo(AVFormatContext *s, SyncFrameInfo * _pstSyncFrameInfo)
{
    int iRet = 0;
    SyncFrame stSyncFrame = {0, 0};
    int iMaxFrameLength = 0;
    int64_t i64FileSize = 0;
    int64_t i64EndFramePos = 0;
    int iOffset = 0;
    int i = 0;
    
    if(NULL == s)
    {
        Media_Error("invalid s(%p)", s);
        iRet = -1;
        goto end;
    }
    //get file size
    if(MpegGetFileSize(s, &i64FileSize) < 0)
    {
        Media_Error("call MpegGetFileSize failed!");
        iRet = -1;
        goto end;
    }

    if(SetSyncFrameInfoFileSize(_pstSyncFrameInfo, i64FileSize) < 0)
    {
        Media_Error("call SetSyncFrameInfoFileSize failed!");
        iRet = -1;
        goto end;
    }

    Media_Debug("i64FileSize=0x%ld", i64FileSize);
    Media_Debug("==============================================");

    //get max frame length
    iMaxFrameLength = MpegCalcFrameSize(s);
    if(iMaxFrameLength <= 0)
    {
        Media_Error("call MpegCalcFrameSize failed!");
        iRet = -1;
        goto end;
    }    
        
    if(SetSyncFrameInfoMaxFrameLength(_pstSyncFrameInfo, iMaxFrameLength) < 0)
    {
        Media_Error("call SetSyncFrameInfoStartTime failed!");
        iRet = -1;
        goto end;
    }

    Media_Debug("iMaxFrameLength=%d", iMaxFrameLength);
    Media_Debug("==============================================");

    //get start time
    if(MpegSeekByFilePos(s, 0LL, &stSyncFrame, 1) < 0)
    {
        Media_Error("call MpegSeekByFilePos failed!");
        iRet = -1;
        goto end;
    }
    if(SetSyncFrameInfoStartTime(_pstSyncFrameInfo, stSyncFrame.m_i64Timestamp) < 0)
    {
        Media_Error("call SetSyncFrameInfoStartTime failed!");
        iRet = -1;
        goto end;
    }    
    Media_Debug("m_i64Pos=0x%lx m_i64Timestamp=%ld", stSyncFrame.m_i64Pos, stSyncFrame.m_i64Timestamp);
    Media_Debug("==============================================");
    
#define FIND_COUNT  100

    for(i = 1; i < FIND_COUNT; i++)
    {
        Media_Debug("i=%d", i);
        stSyncFrame.m_i64Timestamp = 0;
        #if 1
        if(i > (((i64FileSize - 1) + iMaxFrameLength) / iMaxFrameLength))
        {
            Media_Error("i(%d) > (((i64FileSize - 1) + iMaxFrameLength) / iMaxFrameLength)(%ld)", i, (((i64FileSize - 1) + iMaxFrameLength) / iMaxFrameLength));
            iRet = -1;
            goto end;
        }
        #endif

        if(i > 0 && iMaxFrameLength > 0 && ((INT64_MAX / iMaxFrameLength) < i))
        {
			Media_Error("invalid i(%d) iMaxFrameLength(%d)", i, iMaxFrameLength);
            iRet = -1;
            goto end;
        }
        
        iOffset = i * iMaxFrameLength;
        if(i64FileSize > 0 && i > 0 && iMaxFrameLength > 0/* && (INT64_MAX / iMaxFrameLength) > i*/)
        {
            i64EndFramePos = i64FileSize - iOffset;
            if(i64EndFramePos < 0)
            {
                Media_Debug("find end time failed! i64EndFramePos(%ld)=i64FileSize(%ld) - iOffset(%d)", i64EndFramePos, i64FileSize, iOffset);
                i64EndFramePos = 0;
                Media_Debug("continue find and last times, from file start i64EndFramePos(%ld)", i64EndFramePos);
            }
            Media_Debug("i64EndFramePos=0x%lx", i64EndFramePos);
        }else
        {
			Media_Error("invalid i64FileSize(%ld) i(%d) iMaxFrameLength(%d)", i64FileSize, i, iMaxFrameLength);
			iRet = -1;
			goto end;
        }
        //get end time
        if(MpegFindEndTime(s, i64EndFramePos, &stSyncFrame) < 0)
        {
			Media_Debug("m_i64Pos=0x%lx m_i64Timestamp=%ld", stSyncFrame.m_i64Pos, stSyncFrame.m_i64Timestamp);
			if(0 != stSyncFrame.m_i64Timestamp)
			{
				Media_Error("find end frame m_i64Pos=0x%lx m_i64Timestamp=%ld", stSyncFrame.m_i64Pos, stSyncFrame.m_i64Timestamp);
				if(SetSyncFrameInfoEndTime(_pstSyncFrameInfo, stSyncFrame.m_i64Timestamp) < 0)
				{
					Media_Error("call SetSyncFrameInfoStartTime failed!");
					iRet = -1;
					goto end;
				}
				break;
			}

            Media_Error("call MpegFindEndTime failed! continue");
            continue;
        }
        Media_Debug("m_i64Pos=0x%lx m_i64Timestamp=%ld", stSyncFrame.m_i64Pos, stSyncFrame.m_i64Timestamp);
        if(0 != stSyncFrame.m_i64Timestamp)
        {
            Media_Debug("find end frame m_i64Pos=0x%lx m_i64Timestamp=%ld", stSyncFrame.m_i64Pos, stSyncFrame.m_i64Timestamp);
            if(SetSyncFrameInfoEndTime(_pstSyncFrameInfo, stSyncFrame.m_i64Timestamp) < 0)
            {
                Media_Error("call SetSyncFrameInfoStartTime failed!");
                iRet = -1;
                goto end;
            }
            break;
        }

        if(0 == i64EndFramePos)
        {
            Media_Error("i64EndFramePos(%ld), haved find final, break", i64EndFramePos);
            break;
        }

    }    
    Media_Debug("==============================================");
    
end:
    return iRet;
}

static int initAVCodecParameters(AVCodecParameters *_pAVCodecParameters)
{
    int iRet = 0;

    if(NULL == _pAVCodecParameters)
    {
        Media_Error("invalid _pAVPacket(%p)", _pAVCodecParameters);
        iRet = -1;
        goto end;
    }
    _pAVCodecParameters->codec_type = AVMEDIA_TYPE_UNKNOWN;
    _pAVCodecParameters->codec_id = AV_CODEC_ID_NONE;
    _pAVCodecParameters->channel_layout = AV_CH_LAYOUT_MONO;
    _pAVCodecParameters->channels = 0;
    _pAVCodecParameters->sample_rate = 0;   

end:
    
    return iRet;
}

static int initAVStream(AVStream *_pstAVStream)
{
    int iRet = 0;

    if(NULL == _pstAVStream)
    {
        Media_Error("invalid _pstAVStream(%p)", _pstAVStream);
        iRet = -1;
        goto end;
    }
    
    _pstAVStream->index = -1;
    _pstAVStream->id = 0;
    _pstAVStream->discard = AVDISCARD_DEFAULT;
    _pstAVStream->need_parsing = AVSTREAM_PARSE_NONE;
    _pstAVStream->request_probe = 0;
    if(initAVCodecParameters(&(_pstAVStream->codecpar[0])) < 0)
    {
        Media_Error("call initAVPacket failed");
    }
    
end:
    
    return iRet;

}

static int InitMpegDemuxContext(MpegDemuxContext *_pstMpegDemuxContext)
{
    int iRet = 0;

    if(NULL == _pstMpegDemuxContext)
    {
        Media_Error("invalid _pstMpegDemuxContext(%p)", _pstMpegDemuxContext);
        iRet = -1;
        goto end;
    }

    _pstMpegDemuxContext->header_state = 0;
    _pstMpegDemuxContext->sofdec = 0;
    _pstMpegDemuxContext->dvd = 0;
    _pstMpegDemuxContext->imkh_cctv = 0;
    memset(_pstMpegDemuxContext->psm_es_type, 0, sizeof(_pstMpegDemuxContext->psm_es_type));
    _pstMpegDemuxContext->m_iIsGetPsm = 0;
    if(Mpeg_InitPsmHeader(&_pstMpegDemuxContext->m_stPSMHeaderDec) < 0)
    {
        Media_Error("call Mpeg_InitPsmHeader failed");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;
}

static int InitAVFormatContext(AVFormatContext *_pstAVFormatContext, AVIOContext *_pPb)
{
    int iRet = 0;
    int i = 0;

    if(NULL == _pstAVFormatContext || NULL == _pPb)
    {
        Media_Error("invalid _pstAVFormatContext(%p) _pPb(%p)", _pstAVFormatContext, _pPb);
        iRet = -1;
        goto end;
    }

    _pstAVFormatContext->priv_data = NULL;
    _pstAVFormatContext->m_iStartcodePrev = 0;
    _pstAVFormatContext->m_iVideoEnd = 0;
    _pstAVFormatContext->m_iIsPsm = 0;
    _pstAVFormatContext->m_i64PsmPos = 0;
    _pstAVFormatContext->pb = NULL;
    _pstAVFormatContext->ctx_flags = 0;
    _pstAVFormatContext->nb_streams = 0;
    memset(_pstAVFormatContext->filename, 0, sizeof(_pstAVFormatContext->filename));
    _pstAVFormatContext->start_time = 0;
    _pstAVFormatContext->duration = 0;
    _pstAVFormatContext->bit_rate = 0;
    _pstAVFormatContext->packet_size = 0;
    _pstAVFormatContext->max_delay = 0;
    _pstAVFormatContext->probesize = 0;
    _pstAVFormatContext->max_analyze_duration = 0;
    _pstAVFormatContext->key = NULL;
    _pstAVFormatContext->keylen = 0;
    _pstAVFormatContext->nb_programs = 0;
    _pstAVFormatContext->video_codec_id = AV_CODEC_ID_NONE;
    _pstAVFormatContext->audio_codec_id = AV_CODEC_ID_NONE;
    _pstAVFormatContext->subtitle_codec_id = AV_CODEC_ID_NONE;
    _pstAVFormatContext->max_index_size = 0;
    _pstAVFormatContext->max_picture_buffer = 0;
    _pstAVFormatContext->nb_chapters = 0;
    _pstAVFormatContext->start_time_realtime = 0;
    _pstAVFormatContext->fps_probe_size = 0;
    _pstAVFormatContext->error_recognition = 0;
    _pstAVFormatContext->debug = 0;
    _pstAVFormatContext->max_interleave_delta = 0;
    _pstAVFormatContext->strict_std_compliance = 0;
    _pstAVFormatContext->event_flags = 0;
    _pstAVFormatContext->max_ts_probe = 0;
    _pstAVFormatContext->avoid_negative_ts = 0;
    _pstAVFormatContext->ts_id = 0;
    _pstAVFormatContext->audio_preload = 0;
    _pstAVFormatContext->max_chunk_duration = 0;
    _pstAVFormatContext->max_chunk_size = 0;
    _pstAVFormatContext->use_wallclock_as_timestamps = 0;
    _pstAVFormatContext->avio_flags = 0;
    _pstAVFormatContext->skip_initial_bytes = 0;
    _pstAVFormatContext->correct_ts_overflow = 0;
    _pstAVFormatContext->seek2any = 0;
    _pstAVFormatContext->flush_packets = 0;
    _pstAVFormatContext->probe_score = 0;
    _pstAVFormatContext->format_probesize = 0;
    _pstAVFormatContext->codec_whitelist = NULL;
    _pstAVFormatContext->format_whitelist = NULL;
    _pstAVFormatContext->io_repositioned = 0;
    _pstAVFormatContext->metadata_header_padding = 0;
    _pstAVFormatContext->opaque = NULL;
    _pstAVFormatContext->output_ts_offset = 0;
    _pstAVFormatContext->dump_separator = NULL;
    _pstAVFormatContext->data_codec_id = AV_CODEC_ID_NONE;
    _pstAVFormatContext->protocol_whitelist = NULL;
    _pstAVFormatContext->protocol_blacklist = NULL;
    _pstAVFormatContext->max_streams = 0;
    
    for(i = 0; i < (int)(sizeof(_pstAVFormatContext->streams) / sizeof(_pstAVFormatContext->streams[0])) ; i++)
    {
        if(initAVStream(_pstAVFormatContext->streams + i) < 0)
        {
            Media_Error("call initAVStream failed i(%d)", i);
        }
    }

    _pstAVFormatContext->pb = _pPb;
    
end:
    return iRet;
}



#if 0
static void PrintHexData(unsigned char *_pData, int _pDataLen, int _displayCount)
{
    (void)printf("start _pData _pDataLen=%d", _pDataLen);
    int i = 0;

    (void)printf("\n");
    for(i = 0; i < _pDataLen; i++)
    {
        if(_pData[i] < 0x10) {
            (void)printf("0x0%x, ", _pData[i]);
        } else {
            (void)printf("0x%x, ", _pData[i]);
        }
        if((!((i + 1) %_displayCount)) && (i > 0))
        {
            (void)printf("\n");
        }
    }
    (void)printf("\n");
    (void)printf("\n");
    (void)printf("end _pData\n");

    return;
}
#endif

static int InitAVIOContext(AVIOContext *_pstAVIOContext)
{
    int iRet = 0;

    if(NULL == _pstAVIOContext)
    {
        Media_Error("invalid _pstAVIOContext(%p)", _pstAVIOContext);
        iRet = -1;
        goto end;
    }

    memset(_pstAVIOContext->buffer, 0, sizeof(_pstAVIOContext->buffer));
    _pstAVIOContext->buffer_size = 0;
    _pstAVIOContext->buf_ptr = NULL;
    _pstAVIOContext->buf_end = NULL;
    _pstAVIOContext->opaque = NULL;
    _pstAVIOContext->pos = 0;
    _pstAVIOContext->must_flush = 0;
    _pstAVIOContext->eof_reached = 0;
    _pstAVIOContext->write_flag = 0;
    _pstAVIOContext->max_packet_size = 0;
    _pstAVIOContext->checksum = 0;
    _pstAVIOContext->checksum_ptr = NULL;
    _pstAVIOContext->update_checksum = NULL;
    _pstAVIOContext->error = 0;
    _pstAVIOContext->read_pause = NULL;
    _pstAVIOContext->read_seek = NULL;
    _pstAVIOContext->seekable = 0;
    _pstAVIOContext->maxsize = 0;
    _pstAVIOContext->direct = 0;
    _pstAVIOContext->bytes_read = 0;
    _pstAVIOContext->seek_count = 0;
    _pstAVIOContext->writeout_count = 0;
    _pstAVIOContext->orig_buffer_size = 0;
    _pstAVIOContext->short_seek_threshold = 0;
    _pstAVIOContext->protocol_whitelist = NULL;
    _pstAVIOContext->protocol_blacklist = NULL;
    _pstAVIOContext->ignore_boundary_point = 0;
    _pstAVIOContext->last_time = 0;
    
end:

    return iRet;
}

int Mpeg_DecProbe(char *_pFileName)
{
    int iRet = 0;
    AVIOContext stAVIOContext;
    int startcode = 0;
    int header_state = 0xff;
    int size      = MAX_SYNC_SIZE;
    int iBaNum = 0;
    int iBcNum = 0;
    int iE0Num = 0;

    if(InitAVIOContext(&stAVIOContext) < 0)
    {
        Media_Error("call InitAVIOContext failed!");
        iRet = -1;
        goto end;
    }
    
    if(avio_open(&stAVIOContext, _pFileName, "rb"))
    {
        Media_Error("call avio_open failed!");
        iRet = -1;
        goto end;
    }

    (void)avio_seek(&stAVIOContext, 0LL, SEEK_SET);

    while(size > 0)
    {
        /* next start code (should be immediately after) */
        startcode = find_next_start_code(&stAVIOContext, &size, &header_state);
        Media_Debug("startcode=0x%x", startcode);
        if(PACK_START_CODE == (unsigned int)startcode)
        {
        	if (iBaNum < 0x7FFFFFFE) {
            	iBaNum++;
        	}
        }else if(PROGRAM_STREAM_MAP_START_CODE == (unsigned int)startcode)
        {
        	if (iBcNum < 0x7FFFFFFE) {
            	iBcNum++;
        	}
        }else if(VIDEO_ID == (startcode & 0xFF))
        {
        	if (iE0Num < 0x7FFFFFFE) {
            	iE0Num++;
        	}
        }
            
        if(iBaNum > 0 && iBcNum > 0 && iE0Num > 0)
        {
            Media_Debug("iBaNum(%d) iBcNum(%d) iE0Num(%d), is ps file", iBaNum, iBcNum, iE0Num);
            break;
        }
        
        if (avio_feof(&stAVIOContext))
        {
            Media_Warn("read file end");
            break;
        }
    }
    
    if(iBaNum > 0 && iBcNum > 0 && iE0Num > 0)
    {
        iRet = 1;
        Media_Debug("iBaNum(%d) iBcNum(%d) iE0Num(%d), is ps file", iBaNum, iBcNum, iE0Num);
    }else
    {
        iRet = 0;
        Media_Debug("iBaNum(%d) iBcNum(%d) iE0Num(%d), is not ps file", iBaNum, iBcNum, iE0Num);
    }
end:

    if(avio_close(&stAVIOContext) < 0)
    {
        Media_Error("call avio_close _pFileName(%s)", _pFileName);
    }
    
    return iRet;
}

int Mpeg_DecInit(MpegDec *_pstMpegDec, char *_pFileName)
{
    int iRet = 0;
    AVFormatContext *pstAVFormatContext = NULL;
    AVIOContext *pstAVIOContext = NULL;
    AVPacket *pstAVPacket = NULL;
    SyncFrameInfo *pstSyncFrameInfo = NULL;
    if(NULL == _pstMpegDec || NULL == _pFileName)
    {
        Media_Error("invalid _pstMpegDec(%p) _pFileName(%p)", _pstMpegDec, _pFileName);
        iRet = -1;
        goto end;        
    }

    pstAVFormatContext = &_pstMpegDec->m_stAVFormatContext;
    pstAVIOContext = &_pstMpegDec->m_stAVIOContext;
    pstAVPacket = &_pstMpegDec->m_stAVPacket;
    pstSyncFrameInfo = &_pstMpegDec->m_stSyncFrameInfo;

    av_init_packet(pstAVPacket);
    if(InitAVIOContext(pstAVIOContext) < 0)
    {
        Media_Error("call InitAVIOContext failed!");
        iRet = -1;
        goto end;
    }

    if(InitAVFormatContext(pstAVFormatContext, pstAVIOContext) < 0)
    {
        Media_Error("call InitAVFormatContext failed!");
        iRet = -1;
        goto end;
    }

    if(InitMpegDemuxContext(&pstAVFormatContext->m_stMDemux) < 0)
    {
        Media_Error("call InitMpegDemuxContext failed!");
        iRet = -1;
        goto end;
    }

    if(avio_open(pstAVIOContext, _pFileName, "rb") < 0)
    {
        Media_Error("call avio_open failed!");
        iRet = -1;
        goto end;
    }

    if(CalcFileSyncFrameInfo(pstAVFormatContext, pstSyncFrameInfo) < 0)
    {
        Media_Error("call CalcFileSyncFrameInfo failed!");
        iRet = -1;
        goto end;
    }

    if(InitAVFormatContext(pstAVFormatContext, pstAVIOContext) < 0)
    {
        Media_Error("call InitAVFormatContext failed!");
        iRet = -1;
        goto end;
    }

end:
    return iRet;
}

int Mpeg_DecGetProgramInfo(MpegDec *_pstMpegDec, char **_ppPSInfo, int *_puiLen)
{
    int iRet = 0;

    if(NULL == _pstMpegDec || NULL == _ppPSInfo || NULL == _puiLen)
    {
        Media_Error("invalid _pstMpegDec(%p) _ppPSInfo(%p) _puiLen(%p)", _pstMpegDec, _ppPSInfo, _puiLen);
        iRet = -1;
        goto end;
    }

    if(Mpeg_GetProgramStreamInfoData(&_pstMpegDec->m_stAVFormatContext.m_stMDemux.m_stPSMHeaderDec, _ppPSInfo, (unsigned int *)_puiLen) < 0)
    {
        Media_Error("call Mpeg_GetProgramStreamInfoData failed!");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;
}


AVCodecID Mpeg_DecGetVideoCodecId(MpegDec *_pstMpegDec)
{
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    
    if(NULL == _pstMpegDec)
    {
        Media_Error("invalid _pstMpegDec(%p)", _pstMpegDec);
        goto end;
    }

    eAVCodecID = Mpeg_GetVideoCodecId(&_pstMpegDec->m_stAVFormatContext.m_stMDemux.m_stPSMHeaderDec);

end:
    return eAVCodecID;
}

AVCodecID Mpeg_DecGetAudioCodecId(MpegDec *_pstMpegDec)
{
    AVCodecID eAVCodecID = AV_CODEC_ID_NONE;
    
    if(NULL == _pstMpegDec)
    {
        Media_Error("invalid _pstMpegDec(%p)", _pstMpegDec);
        goto end;
    }

    eAVCodecID = Mpeg_GetAudioCodecId(&_pstMpegDec->m_stAVFormatContext.m_stMDemux.m_stPSMHeaderDec);

end:
    return eAVCodecID;
}


int Mpeg_DecGetVideoESInfo(MpegDec *_pstMpegDec, char **_ppcESInfoData, int *_piESInfoLen)
{
    int iRet = 0;

    if(NULL == _pstMpegDec || NULL == _ppcESInfoData || NULL == _piESInfoLen)
    {
        Media_Error("invalid _pstMpegDec(%p) _ppcESInfoData(%p) _piESInfoLen(%p)", _pstMpegDec, _ppcESInfoData, _piESInfoLen);
        iRet = -1;
        goto end;
    }
    
    if(Mpeg_GetElementaryStreamData(&_pstMpegDec->m_stAVFormatContext.m_stMDemux.m_stPSMHeaderDec, 0, _ppcESInfoData, _piESInfoLen) < 0)
    {
        Media_Error("call Mpeg_GetElementaryStreamData failed!");
        iRet = -1;
        goto end;
    }

end:
    return iRet;
}

int Mpeg_DecGetAudioESInfo(MpegDec *_pstMpegDec, char **_ppcESInfoData, int *_piESInfoLen)
{
    int iRet = 0;

    if(NULL == _pstMpegDec || NULL == _ppcESInfoData || NULL == _piESInfoLen)
    {
        Media_Error("invalid _pstMpegDec(%p) _ppcESInfoData(%p) _piESInfoLen(%p)", _pstMpegDec, _ppcESInfoData, _piESInfoLen);
        iRet = -1;
        goto end;
    }
    
    if(Mpeg_GetElementaryStreamData(&_pstMpegDec->m_stAVFormatContext.m_stMDemux.m_stPSMHeaderDec, 1, _ppcESInfoData, _piESInfoLen) < 0)
    {
        Media_Error("call Mpeg_GetElementaryStreamData failed!");
        iRet = -1;
        goto end;
    }

end:
    return iRet;
}

int Mpeg_DecMaxFrameLength(MpegDec *_pstMpegDec)
{
    int iRet = 0;

    if(NULL == _pstMpegDec)
    {
        Media_Error("invalid _pstMpegDec(%p)", _pstMpegDec);
        iRet = -1;
        goto end;
    }

    iRet = GetSyncFrameInfoMaxFrameLength(&_pstMpegDec->m_stSyncFrameInfo);
    if(iRet < 0)
    {
        Media_Error("call GetSyncFrameInfoMaxFrameLength failed!");
        iRet = -1;
        goto end;
    }
    
end:
    return iRet;
}

int Mpeg_DecReadES(MpegDec *_pstMpegDec, int *_piIsSyncFrame)
{
    int iRet = 0;

    if(NULL == _pstMpegDec || NULL == _piIsSyncFrame)
    {
        Media_Error("invalid _pstMpegDec(%p) _piIsSyncFrame(%p)", _pstMpegDec, _piIsSyncFrame);
        iRet = -1;
        goto end;
    }

    _pstMpegDec->m_stAVPacket.size = 0;
    if(mpegps_read_packet(&_pstMpegDec->m_stAVFormatContext, &_pstMpegDec->m_stAVPacket) < 0)
    {
        Media_Error("call mpegps_read_packet failed!");
        iRet = -1;
        goto end;
    }

    if(1 == _pstMpegDec->m_stAVFormatContext.m_iIsPsm)
    {
        *_piIsSyncFrame = 1;
    }
    
end:
    return iRet;
}

static int FindNearbyStartCode(AVIOContext *pb, int64_t _i64FilePos, int64_t _i64FileSize, int _iStartCode, int64_t *_pi64StartCodePos)
{
    int iRet = 0;
    int64_t i64FilePosCur = 0;

    if(NULL == pb || NULL == _pi64StartCodePos)
    {
        Media_Error("invalid pb(%p) _pi64StartCodePos(%p)", pb, _pi64StartCodePos);
        iRet = -1;
        goto end;
    }

    i64FilePosCur = _i64FilePos;
    
    while(i64FilePosCur < _i64FileSize)
    {
        if(FindStartCode(pb, i64FilePosCur, _i64FileSize, _iStartCode, _pi64StartCodePos) < 0)
        {
            Media_Error("call FindStartCode failed! continue i64FilePosCur=%ld", i64FilePosCur);
            *_pi64StartCodePos = 0;
            i64FilePosCur = (i64FilePosCur + (3 * (1 << 20))) - 4;
            continue;
        }
        Media_Debug("find ok _iStartCode(0x%x) *_pi64StartCodePos(%ld)", _iStartCode, *_pi64StartCodePos);
        goto end;
    }

    i64FilePosCur = _i64FilePos;

    while(i64FilePosCur > 0)
    {
        i64FilePosCur = i64FilePosCur - (3 * (1 << 20));
        if(i64FilePosCur < 0)
        {
            Media_Error("i64FilePosCur=%ld set = 0", i64FilePosCur);
            i64FilePosCur = 0;
        }
        
        if(FindStartCode(pb, i64FilePosCur, _i64FileSize, _iStartCode, _pi64StartCodePos) < 0)
        {
            Media_Error("call FindStartCode failed! continue i64FilePosCur=%ld", i64FilePosCur);
            *_pi64StartCodePos = 0;
            if(i64FilePosCur <= 0)
            {
                Media_Error("call FindStartCode failed! continue i64FilePosCur=%ld", i64FilePosCur);
                iRet = -1;
                goto end;
            }
            continue;
        }
        Media_Debug("find ok _iStartCode(0x%x) *_pi64StartCodePos(%ld)", _iStartCode, *_pi64StartCodePos);
        goto end;
    }
    
end:

    return iRet;
}

int Mpeg_DecSeek(MpegDec *_pstMpegDec, int64_t _i64Timestamp)
{
    int iRet = 0;
    AVFormatContext *pstAVFormatContext = NULL;
    SyncFrameInfo *pstSyncFrameInfo = NULL;
    int64_t i64FileSize = 0;
    int64_t i64FilePosSeek = 0;
    int64_t i64FilePosSeekStartCode = 0;
    int64_t i64StartTime = 0;
    int64_t i64EndTime = 0;
    int64_t i64FilePosLeft = 0;
    int64_t i64FilePosRight = 0;
    SyncFrame *pstSyncFrame = NULL;
    SyncFrame stSyncFrame;
    int iCount = 0;
    int64_t i64RawPos = 0;
    
    if(NULL == _pstMpegDec)
    {
        Media_Error("invalid _pstMpegDec(%p)", _pstMpegDec);
        iRet = -1;
        goto end;
    }

    pstSyncFrameInfo = &_pstMpegDec->m_stSyncFrameInfo;
    pstAVFormatContext = &_pstMpegDec->m_stAVFormatContext;

    i64FileSize = GetSyncFrameInfoFileSize(pstSyncFrameInfo);
    i64StartTime = GetSyncFrameInfoStartTime(pstSyncFrameInfo);
    i64EndTime = GetSyncFrameInfoEndTime(pstSyncFrameInfo);

    i64RawPos = avio_tell(pstAVFormatContext->pb);
    Media_Debug("i64RawPos(0x%lx)", i64RawPos);

    if(i64StartTime > i64EndTime)
    {
        Media_Error("invalid i64StartTime(%ld) > i64EndTime(%ld)", i64StartTime, i64EndTime);
        iRet = -1;
        goto end;
    }
	
    if(_i64Timestamp < i64StartTime)
    {
        Media_Error("invalid _i64Timestamp(%ld) < i64StartTime(%ld)", _i64Timestamp, i64StartTime);
        iRet = 0;
        stSyncFrame.m_i64Pos = 0;
        stSyncFrame.m_i64Timestamp = i64StartTime;
        goto found;
    }

    if(_i64Timestamp > i64EndTime)
    {
        Media_Error("invalid _i64Timestamp(%ld) > i64EndTime(%ld)", _i64Timestamp, i64EndTime);
        iRet = -1;
        goto end;
    }

    if(i64EndTime > i64StartTime)
    {
        i64FilePosSeek = ((_i64Timestamp - i64StartTime) * i64FileSize) / (i64EndTime - i64StartTime);
    }else
    {
        Media_Error("i64EndTime(%ld) <= i64StartTime(%ld), small file", i64EndTime, i64StartTime);
        iRet = 0;
        stSyncFrame.m_i64Pos = 0;
        stSyncFrame.m_i64Timestamp = i64StartTime;
        goto found;
    }
    
    Media_Debug("_i64Timestamp(%ld) i64StartTime(%ld) i64EndTime(%ld) i64FileSize(0x%lx) i64FilePosSeek(0x%lx) avio_tell(0x%lx)", _i64Timestamp, i64StartTime, i64EndTime, i64FileSize, i64FilePosSeek, avio_tell(&_pstMpegDec->m_stAVIOContext));

    i64FilePosRight = i64FileSize;    
    Media_Debug("i64FilePosLeft(0x%lx) i64FilePosRight(0x%lx) i64FilePosSeek=(0x%lx)", i64FilePosLeft, i64FilePosRight, i64FilePosSeek);
    
    while(i64FilePosLeft != i64FilePosRight)
    {
        iCount++;
        if(iCount > 100)
        {
            Media_Error("find iCount(%d)", iCount);
            break;
        }
        i64FilePosSeekStartCode = 0;
        if(FindStartCode(pstAVFormatContext->pb, i64FilePosSeek, i64FileSize, 0xba, &i64FilePosSeekStartCode) < 0)
        {
            Media_Error("call FindStartCode failed! break");
            break;
        }
        
        //get time
        if(MpegSeekByFilePos(pstAVFormatContext, i64FilePosSeekStartCode, &stSyncFrame, 0) < 0)
        {
            Media_Error("call MpegSeekByFilePos failed! break");
            break;
        }
        Media_Debug("i64FilePosSeekStartCode(0x%lx), is m_i64Pos(0x%lx) m_i64Timestamp(%ld)", i64FilePosSeekStartCode, stSyncFrame.m_i64Pos, stSyncFrame.m_i64Timestamp);

#define TIME_BASE   90000

        if(_i64Timestamp == stSyncFrame.m_i64Timestamp)
        {
            Media_Debug("find _i64Timestamp(%ld) == m_i64Timestamp(%ld) ok, is m_i64Pos(0x%lx)", _i64Timestamp, stSyncFrame.m_i64Timestamp, stSyncFrame.m_i64Pos);
            break;
        }else if(_i64Timestamp < stSyncFrame.m_i64Timestamp)
        {
            if((stSyncFrame.m_i64Timestamp - _i64Timestamp) <= TIME_BASE)
            {
                Media_Debug("find (stSyncFrame.m_i64Timestamp(%ld) - _i64Timestamp)(%ld) <= TIME_BASE(%d)", stSyncFrame.m_i64Timestamp,  _i64Timestamp, TIME_BASE);
                break;
            }
            
            i64FilePosRight = i64FilePosSeek - 1;
            i64FilePosSeek = (i64FilePosLeft + i64FilePosRight) / 2;
            Media_Debug("i64FilePosLeft(0x%lx) i64FilePosRight(0x%lx) stSyncFrame.m_i64Pos(0x%lx) i64FilePosSeek=(0x%lx)", i64FilePosLeft, i64FilePosRight, stSyncFrame.m_i64Pos, i64FilePosSeek);
        }else if(_i64Timestamp > stSyncFrame.m_i64Timestamp)
        {
            if((_i64Timestamp - stSyncFrame.m_i64Timestamp) <= TIME_BASE)
            {
                Media_Debug("find (_i64Timestamp(%ld) - stSyncFrame.m_i64Timestamp(%ld)) <= TIME_BASE(%d)", _i64Timestamp, stSyncFrame.m_i64Timestamp, TIME_BASE);
                break;
            }
            
            i64FilePosLeft = i64FilePosSeek + 1;
            i64FilePosSeek = (i64FilePosLeft + i64FilePosRight) / 2;  
            Media_Debug("i64FilePosLeft(0x%lx) i64FilePosRight(0x%lx) stSyncFrame.m_i64Pos(0x%lx) i64FilePosSeek=(0x%lx)", i64FilePosLeft, i64FilePosRight, stSyncFrame.m_i64Pos, i64FilePosSeek);
        }

        if(i64FilePosLeft >= i64FilePosRight)
        {
            Media_Debug("find _i64Timestamp(%ld) stSyncFrame.m_i64Timestamp(%ld), i64FilePosLeft(0x%lx) >= i64FilePosRight(0x%lx)", _i64Timestamp, stSyncFrame.m_i64Timestamp, i64FilePosLeft, i64FilePosRight);
            break;
        }
    }

    i64FilePosSeekStartCode = 0;
    if(FindNearbyStartCode(pstAVFormatContext->pb, i64FilePosSeek, i64FileSize, 0xbc, &i64FilePosSeekStartCode) < 0)
    {
        Media_Error("call FindStartCode failed!");
        iRet = -1;
        goto end;
    }
    
    Media_Debug("find i64FilePosSeekStartCode(0x%lx)", i64FilePosSeekStartCode);
    if(MpegSeekByFilePos(pstAVFormatContext, i64FilePosSeekStartCode, &stSyncFrame, 1) < 0)
    {
        Media_Error("call MpegSeekByFilePos failed!");
        iRet = -1;
        goto end;
    }

    stSyncFrame.m_i64Pos = i64FilePosSeekStartCode;
found:
    pstSyncFrame = &stSyncFrame;
    
    Media_Debug("find _i64Timestamp(%ld) ok, is psm i64FilePosSeekStartCode(%ld) m_i64Timestamp(%ld) iCount=%d", _i64Timestamp, i64FilePosSeekStartCode, pstSyncFrame->m_i64Timestamp, iCount);
    iCount = iCount;
    
    if(avio_seek(&_pstMpegDec->m_stAVIOContext, pstSyncFrame->m_i64Pos, SEEK_SET) < 0)
    {
        Media_Error("call avio_seek failed! m_i64Pos(%ld) m_i64Timestamp(%ld)", pstSyncFrame->m_i64Pos, pstSyncFrame->m_i64Timestamp);
        iRet = -1;
        goto end;
    }

    
end:

    if(iRet < 0)
    {        
        Media_Error("seek failed! i64RawPos(0x%lx)", i64RawPos);
        if (NULL != pstAVFormatContext && NULL != pstAVFormatContext->pb && -1 != i64RawPos && avio_seek(pstAVFormatContext->pb, i64RawPos, SEEK_SET) < 0)
        {
            Media_Error("call avio_seek failed! i64RawPos(0x%lx)", i64RawPos);
        }

    }
    
    if(NULL != pstAVFormatContext)
    {
        pstAVFormatContext->m_iStartcodePrev = 0xff;
    }
    
    return iRet;

}


int64_t Mpeg_DecGetStartTime(MpegDec *_pstMpegDec)
{
    int64_t i64Timestamp = -1;

    if(NULL == _pstMpegDec)
    {
        Media_Error("invalid _pstMpegDec(%p)", _pstMpegDec);
        goto end;
    }

    i64Timestamp = _pstMpegDec->m_stSyncFrameInfo.m_i64StartTime;
    
end:
    return i64Timestamp;

}


int64_t Mpeg_DecGetEndTime(MpegDec *_pstMpegDec)
{
    int64_t i64Timestamp = -1;

    if(NULL == _pstMpegDec)
    {
        Media_Error("invalid _pstMpegDec(%p)", _pstMpegDec);
        goto end;
    }

    i64Timestamp = _pstMpegDec->m_stSyncFrameInfo.m_i64EndTime;
    
end:
    return i64Timestamp;

}



int Mpeg_DecUninit(MpegDec *_pstMpegDec)
{
    int iRet = 0;
    
    if(NULL == _pstMpegDec)
    {
        Media_Error("invalid _pstMpegDec(%p) ", _pstMpegDec);
        iRet = -1;
        goto end;        
    }

    if(UninitSyncFrameInfo(&_pstMpegDec->m_stSyncFrameInfo) < 0)
    {
        Media_Error("call UninitSyncFrameInfo failed!");
    }
    
    if(avio_close(&_pstMpegDec->m_stAVIOContext) < 0)
    {
        Media_Error("call avio_close failed!");
    }
end:
    return iRet;
}

#if 0
int Demo_Demux(int argc, char **argv)
{
    MpegDec stMpegDec;
    void *pFVideo = NULL;
    void *pFAudio = NULL;

    argc = argc;
    argv = argv;
    
    if(Mpeg_DecInit(&stMpegDec, "h264-8192bps-25fps-25-g711a-8k_111.ps") < 0)
    {
        Media_Error("call Mpeg_DecInits failed!");
        return -1;
    }

    
    pFVideo = Media_Fopen(NULL, "h264-8192bps-25fps-25-g711a-8k_111.video", "w+");
    if(NULL == pFVideo)
    {
        Media_Error("call Media_Fopen failed!");
        return -1;
    }

    pFAudio = Media_Fopen(NULL, "h264-8192bps-25fps-25-g711a-8k_111.audio", "w+");
    if(NULL == pFAudio)
    {
        Media_Error("call Media_Fopen failed!");
        return -1;
    }

    int iStreamIndex = 0;
    int iStreamCount = 0;
    AVMediaType ePrevAVMediaType = AVMEDIA_TYPE_UNKNOWN;
    
    iStreamCount = sizeof(stMpegDec.m_stAVFormatContext.streams) / sizeof(stMpegDec.m_stAVFormatContext.streams[0]);
    
    do {        
        stMpegDec.m_stAVPacket.size = 0;

        if(Mpeg_DecReadES(&stMpegDec) < 0)
        {
            Media_Error("call Mpeg_DecReadES failed!");
            break;
        }

        
        iStreamIndex = stMpegDec.m_stAVPacket.stream_index;
        
        if(iStreamIndex < 0 || iStreamIndex >= iStreamCount)
        {
            Media_Error("invalid iStreamIndex(%d) iStreamCount(%d)", iStreamIndex, iStreamCount);
            break;
        }
        
        if(AVMEDIA_TYPE_VIDEO == stMpegDec.m_stAVFormatContext.streams[iStreamIndex].codecpar->codec_type)
        {
            Media_Debug("video pts=%ld", stMpegDec.m_stAVPacket.pts);
            Media_Debug("video dts=%ld", stMpegDec.m_stAVPacket.dts);
            if(Media_Fwrite(stMpegDec.m_stAVPacket.data, 1, stMpegDec.m_stAVPacket.size, pFVideo) != stMpegDec.m_stAVPacket.size)
            {
                Media_Error("call Media_Fwrite failed, stAVPacket.size(%d)", stMpegDec.m_stAVPacket.size);
                return -1;

            }
            PrintHexData((unsigned char *)stMpegDec.m_stAVPacket.data, stMpegDec.m_stAVPacket.size > 64 ? 64 : stMpegDec.m_stAVPacket.size, 32);
        }else if(AVMEDIA_TYPE_AUDIO == stMpegDec.m_stAVFormatContext.streams[iStreamIndex].codecpar->codec_type)
        {
            Media_Debug("video pts=%ld", stMpegDec.m_stAVPacket.pts);
            Media_Debug("video dts=%ld", stMpegDec.m_stAVPacket.dts);
            if(AVMEDIA_TYPE_VIDEO == ePrevAVMediaType)
            {

            }
            if(Media_Fwrite(stMpegDec.m_stAVPacket.data, 1, (unsigned int)stMpegDec.m_stAVPacket.size, pFAudio) != stMpegDec.m_stAVPacket.size)
            {
                Media_Error("call Media_Fwrite failed, stAVPacket.size(%d)", stMpegDec.m_stAVPacket.size);
                return -1;

            }
            PrintHexData((unsigned char *)stMpegDec.m_stAVPacket.data, stMpegDec.m_stAVPacket.size > 64 ? 64 : stMpegDec.m_stAVPacket.size, 32);
        }

        ePrevAVMediaType = stMpegDec.m_stAVFormatContext.streams[iStreamIndex].codecpar->codec_type;
        

    } while(stMpegDec.m_stAVPacket.size > 0);

    if(NULL != pFVideo)
    {
        if(Media_Fclose(pFVideo) < 0)
        {
            Media_Error("call Media_Fclose failed!");
        }
        pFVideo = NULL;
    }
    
    if(NULL != pFAudio)
    {
        if(Media_Fclose(pFAudio) < 0)
        {
            Media_Error("call Media_Fclose failed!");
        }
        pFAudio = NULL;
    }

    return 0;
}
#endif

