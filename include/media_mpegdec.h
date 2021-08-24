
#ifndef _MEDIAMPEGDEC_H
#define _MEDIAMPEGDEC_H

    
#include "media_general.h"

#define MAX_SYNC_SIZE 100000
#define AVERROR_EOF     -1 /*FFERRTAG( 'E','O','F',' ')*/ ///< End of file
#define FFERROR_REDO    -1 /*FFERRTAG('R','E','D','O')*/
#define AV_NOPTS_VALUE          ((int64_t)UINT64_C(0x8000000000000000))
#define AV_CH_FRONT_CENTER           0x00000004
#define AV_CH_LAYOUT_MONO              (AV_CH_FRONT_CENTER)
#define MAX_PROBE_PACKETS 2500

/**
 * Options for behavior on timestamp wrap detection.
 */
#define AV_PTS_WRAP_IGNORE      0   ///< ignore the wrap
#define AV_PTS_WRAP_ADD_OFFSET  1   ///< add the format specific offset on wrap detection
#define AV_PTS_WRAP_SUB_OFFSET  -1  ///< subtract the format specific offset on wrap detection

enum {
    MAX_CONFIG_ATTRIBUTES  = 4,
    MAX_GLOBAL_PARAMS      = 4,
    MAX_PICTURE_REFERENCES = 2,
    MAX_PICTURE_SLICES     = 1,
    MAX_PARAM_BUFFERS      = 16,
    MAX_REORDER_DELAY      = 16,
    MAX_PARAM_BUFFER_SIZE  = 1024,
};

#define av_malloc malloc
#define av_free free

enum AVStreamParseType {
    AVSTREAM_PARSE_NONE,
    AVSTREAM_PARSE_FULL,       /**< full parsing and repack */
    AVSTREAM_PARSE_HEADERS,    /**< Only parse headers, do not repack. */
    AVSTREAM_PARSE_TIMESTAMPS, /**< full parsing and interpolation of timestamps for frames not starting on a packet boundary */
    AVSTREAM_PARSE_FULL_ONCE,  /**< full parsing and repack of the first frame only, only implemented for H.264 currently */
};

/**
 * @ingroup lavc_decoding
 */
enum AVDiscard{
    /* We leave some space between them for extensions (drop some
     * keyframes for intra-only or drop just some bidir frames). */
    AVDISCARD_NONE    =-16, ///< discard nothing
    AVDISCARD_DEFAULT =  0, ///< discard useless packets like 0 size packets in avi
    AVDISCARD_NONREF  =  8, ///< discard all non reference
    AVDISCARD_BIDIR   = 16, ///< discard all bidirectional frames
    AVDISCARD_NONINTRA= 24, ///< discard all non intra frames
    AVDISCARD_NONKEY  = 32, ///< discard all frames except keyframes
    AVDISCARD_ALL     = 48, ///< discard all
};


/*buffer*/

typedef struct AVIOContext {
    /**
    * A class for private options.
    *
    * If this AVIOContext is created by avio_open2(), av_class is set and
    * passes the options down to protocols.
    *
    * If this AVIOContext is manually allocated, then av_class may be set by
    * the caller.
    *
    * warning -- this field can be NULL, be sure to not pass this AVIOContext
    * to any av_opt_* functions in that case.
    */

    /*cwm:todo*/
    //const AVClass *av_class;

    /*
    * The following shows the relationship between buffer, buf_ptr, buf_end, buf_size,
    * and pos, when reading and when writing (since AVIOContext is used for both):
    *
    **********************************************************************************
    *                                   READING
    **********************************************************************************
    *
    *                            |              buffer_size              |
    *                            |---------------------------------------|
    *                            |                                       |
    *
    *                         buffer          buf_ptr       buf_end
    *                            +---------------+-----------------------+
    *                            |/ / / / / / / /|/ / / / / / /|         |
    *  read buffer:              |/ / consumed / | to be read /|         |
    *                            |/ / / / / / / /|/ / / / / / /|         |
    *                            +---------------+-----------------------+
    *
    *                                                         pos
    *              +-------------------------------------------+-----------------+
    *  input file: |                                           |                 |
    *              +-------------------------------------------+-----------------+
    *
    *
    **********************************************************************************
    *                                   WRITING
    **********************************************************************************
    *
    *                                          |          buffer_size          |
    *                                          |-------------------------------|
    *                                          |                               |
    *
    *                                       buffer              buf_ptr     buf_end
    *                                          +-------------------+-----------+
    *                                          |/ / / / / / / / / /|           |
    *  write buffer:                           | / to be flushed / |           |
    *                                          |/ / / / / / / / / /|           |
    *                                          +-------------------+-----------+
    *
    *                                         pos
    *               +--------------------------+-----------------------------------+
    *  output file: |                          |                                   |
    *               +--------------------------+-----------------------------------+
    *
    */
    unsigned char buffer[128];  /**< Start of the buffer. *//*cwm:todo*/
    int buffer_size;        /**< Maximum buffer size */
    unsigned char *buf_ptr; /**< Current position in the buffer */
    unsigned char *buf_end; /**< End of the data, may be less than
     buffer+buffer_size if the read function returned
     less data than requested, e.g. for streams where
     no more data has been received yet. */
    void *opaque;           /**< A private pointer, passed to the read/write/seek/...
     functions. */
    int (*read_packet)(void *opaque, uint8_t *buf, int buf_size);
    int (*write_packet)(void *opaque, uint8_t *buf, int buf_size);
    int64_t (*seek)(void *opaque, int64_t offset, int whence);
    int64_t pos;            /**< position in the file of the current buffer */
    int must_flush;         /**< true if the next seek should flush */
    int eof_reached;        /**< true if eof reached */
    int write_flag;         /**< true if open for writing */
    int max_packet_size;
    unsigned long checksum;
    unsigned char *checksum_ptr;
    unsigned long (*update_checksum)(unsigned long checksum, const uint8_t *buf, unsigned int size);
    int error;              /**< contains the error code or 0 if no error happened */
    /**
    * Pause or resume playback for network streaming protocols - e.g. MMS.
    */
    int (*read_pause)(void *opaque, int pause);
    /**
    * Seek to a given timestamp in stream with the specified stream_index.
    * Needed for some network streaming protocols which don't support seeking
    * to byte position.
    */
    int64_t (*read_seek)(void *opaque, int stream_index,
    int64_t timestamp, int flags);
    /**
    * A combination of AVIO_SEEKABLE_ flags or 0 when the stream is not seekable.
    */
    int seekable;

    /**
    * max filesize, used to limit allocations
    * This field is internal to libavformat and access from outside is not allowed.
    */
    int64_t maxsize;

    /**
    * avio_read and avio_write should if possible be satisfied directly
    * instead of going through a buffer, and avio_seek will always
    * call the underlying seek function directly.
    */
    int direct;

    /**
    * Bytes read statistic
    * This field is internal to libavformat and access from outside is not allowed.
    */
    int64_t bytes_read;

    /**
    * seek statistic
    * This field is internal to libavformat and access from outside is not allowed.
    */
    int seek_count;

    /**
    * writeout statistic
    * This field is internal to libavformat and access from outside is not allowed.
    */
    int writeout_count;

    /**
    * Original buffer size
    * used internally after probing and ensure seekback to reset the buffer size
    * This field is internal to libavformat and access from outside is not allowed.
    */
    int orig_buffer_size;

    /**
    * Threshold to favor readahead over seek.
    * This is current internal only, do not use from outside.
    */
    int short_seek_threshold;

    /**
    * ',' separated list of allowed protocols.
    */
    const char *protocol_whitelist;

    /**
    * ',' separated list of disallowed protocols.
    */
    const char *protocol_blacklist;

    /**
    * A callback that is used instead of write_packet.
    */
    /*cwm:todo*/
#if 0
    int (*write_data_type)(void *opaque, uint8_t *buf, int buf_size,
    enum AVIODataMarkerType type, int64_t time);
#endif
    /**
    * If set, don't call write_data_type separately for AVIO_DATA_MARKER_BOUNDARY_POINT,
    * but ignore them and treat them as AVIO_DATA_MARKER_UNKNOWN (to avoid needlessly
    * small chunks of data returned from the callback).
    */
    int ignore_boundary_point;

    /**
    * Internal, not meant to be used from outside of AVIOContext.
    */
    /*cwm:todo*/
    //enum AVIODataMarkerType current_type;
    int64_t last_time;
} AVIOContext;


typedef struct AVPacket {
    /**
     * A reference to the reference-counted buffer where the packet data is
     * stored.
     * May be NULL, then the packet data is not reference-counted.
     */
    /*cwm:todo unsigned char m_ucBuffer���AVBufferRef buf*/
    //AVBufferRef *buf;
    #define BUFFER_SIZE 65536
    unsigned char m_ucBuffer[BUFFER_SIZE];
    /**
     * Presentation timestamp in AVStream->time_base units; the time at which
     * the decompressed packet will be presented to the user.
     * Can be AV_NOPTS_VALUE if it is not stored in the file.
     * pts MUST be larger or equal to dts as presentation cannot happen before
     * decompression, unless one wants to view hex dumps. Some formats misuse
     * the terms dts and pts/cts to mean something different. Such timestamps
     * must be converted to true pts/dts before they are stored in AVPacket.
     */
    int64_t pts;
    /**
     * Decompression timestamp in AVStream->time_base units; the time at which
     * the packet is decompressed.
     * Can be AV_NOPTS_VALUE if it is not stored in the file.
     */
    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    /**
     * A combination of AV_PKT_FLAG values
     */
    int   flags;
    /**
     * Additional packet data that can be provided by the container.
     * Packet can contain several types of side information.
     */
    /*cwm:todo*/
    //AVPacketSideData *side_data;
    int side_data_elems;

    /**
     * Duration of this packet in AVStream->time_base units, 0 if unknown.
     * Equals next_pts - this_pts in presentation order.
     */
    int64_t duration;

    int64_t pos;                            ///< byte position in stream, -1 if unknown

#if FF_API_CONVERGENCE_DURATION
    /**
     * @deprecated Same as the duration field, but as int64_t. This was required
     * for Matroska subtitles, whose duration values could overflow when the
     * duration field was still an int.
     */
    attribute_deprecated
    int64_t convergence_duration;
#endif
} AVPacket;


#define ELEMENTARY_STREAM_INFO_LEN  64

typedef struct AVCodecParameters {
    //////////////////////cwm///////////////////////
    /**
     * General type of the encoded data.
     */
    enum AVMediaType m_eCodecType;
    /**
     * Specific type of the encoded data (the codec used).
     */
    enum AVCodecID   m_eCodecId;

    unsigned int m_uiESInfoLen;
    char m_cESInfoData[ELEMENTARY_STREAM_INFO_LEN];
    
    /**
     * Additional information about the codec (corresponds to the AVI FOURCC).
     */
    uint32_t         m_CodecTag;
    ///////////////////////cwm///////////////////////

    /**
     * General type of the encoded data.
     */
    enum AVMediaType codec_type;
    /**
     * Specific type of the encoded data (the codec used).
     */
    enum AVCodecID   codec_id;
    /**
     * Additional information about the codec (corresponds to the AVI FOURCC).
     */
    uint32_t         codec_tag;

    /**
     * Extra binary data needed for initializing the decoder, codec-dependent.
     *
     * Must be allocated with av_malloc() and will be freed by
     * avcodec_parameters_free(). The allocated size of extradata must be at
     * least extradata_size + AV_INPUT_BUFFER_PADDING_SIZE, with the padding
     * bytes zeroed.
     */
    uint8_t *extradata;
    /**
     * Size of the extradata content in bytes.
     */
    int      extradata_size;

    /**
     * - video: the pixel format, the value corresponds to enum AVPixelFormat.
     * - audio: the sample format, the value corresponds to enum AVSampleFormat.
     */
    int format;

    /**
     * The average bitrate of the encoded data (in bits per second).
     */
    int64_t bit_rate;

    /**
     * The number of bits per sample in the codedwords.
     *
     * This is basically the bitrate per sample. It is mandatory for a bunch of
     * formats to actually decode them. It's the number of bits for one sample in
     * the actual coded bitstream.
     *
     * This could be for example 4 for ADPCM
     * For PCM formats this matches bits_per_raw_sample
     * Can be 0
     */
    int bits_per_coded_sample;

    /**
     * This is the number of valid bits in each output sample. If the
     * sample format has more bits, the least significant bits are additional
     * padding bits, which are always 0. Use right shifts to reduce the sample
     * to its actual size. For example, audio formats with 24 bit samples will
     * have bits_per_raw_sample set to 24, and format set to AV_SAMPLE_FMT_S32.
     * To get the original sample use "(int32_t)sample >> 8"."
     *
     * For ADPCM this might be 12 or 16 or similar
     * Can be 0
     */
    int bits_per_raw_sample;

    /**
     * Codec-specific bitstream restrictions that the stream conforms to.
     */
    int profile;
    int level;

    /**
     * Video only. The dimensions of the video frame in pixels.
     */
    int width;
    int height;

    /**
     * Video only. The aspect ratio (width / height) which a single pixel
     * should have when displayed.
     *
     * When the aspect ratio is unknown / undefined, the numerator should be
     * set to 0 (the denominator may have any value).
     */
    /*cwm:todo*/
    //AVRational sample_aspect_ratio;

    /**
     * Video only. The order of the fields in interlaced video.
     */
    /*cwm:todo*/
    //enum AVFieldOrder                  field_order;

    /**
     * Video only. Additional colorspace characteristics.
     */
    /*cwm:todo*/
    #if 0
    enum AVColorRange                  color_range;
    enum AVColorPrimaries              color_primaries;
    enum AVColorTransferCharacteristic color_trc;
    enum AVColorSpace                  color_space;
    enum AVChromaLocation              chroma_location;
    #endif
    
    /**
     * Video only. Number of delayed frames.
     */
    int video_delay;

    /**
     * Audio only. The channel layout bitmask. May be 0 if the channel layout is
     * unknown or unspecified, otherwise the number of bits set must be equal to
     * the channels field.
     */
    uint64_t channel_layout;
    /**
     * Audio only. The number of audio channels.
     */
    int      channels;
    /**
     * Audio only. The number of audio samples per second.
     */
    int      sample_rate;
    /**
     * Audio only. The number of bytes per coded audio frame, required by some
     * formats.
     *
     * Corresponds to nBlockAlign in WAVEFORMATEX.
     */
    int      block_align;
    /**
     * Audio only. Audio frame size, if known. Required by some formats to be static.
     */
    int      frame_size;

    /**
     * Audio only. The amount of padding (in samples) inserted by the encoder at
     * the beginning of the audio. I.e. this number of leading decoded samples
     * must be discarded by the caller to get the original audio without leading
     * padding.
     */
    int initial_padding;
    /**
     * Audio only. The amount of padding (in samples) appended by the encoder to
     * the end of the audio. I.e. this number of decoded samples must be
     * discarded by the caller from the end of the stream to get the original
     * audio without any trailing padding.
     */
    int trailing_padding;
    /**
     * Audio only. Number of samples to skip after a discontinuity.
     */
    int seek_preroll;
} AVCodecParameters;


#define ELEMENTARY_STREAM_NUM       2
#define PROGRAM_STREAM_INFO_LEN     128

typedef struct PSMHeader{
    unsigned int m_uiPSInfoLen;/*program_stream_map_length*/
    char m_cPSInfoData[PROGRAM_STREAM_INFO_LEN];

    AVCodecParameters m_stCodec[ELEMENTARY_STREAM_NUM];
}PSMHeader;


typedef struct MpegDemuxContext {
    //AVClass *class;/*cwm:todo*/
    int32_t header_state;
    unsigned char psm_es_type[256];
    int sofdec;
    int dvd;
    int imkh_cctv;
    int m_iIsGetPsm;
    PSMHeader m_stPSMHeaderDec;
#if CONFIG_VOBSUB_DEMUXER
    AVFormatContext *sub_ctx;
    FFDemuxSubtitlesQueue q[32];
    char *sub_name;
#endif
} MpegDemuxContext;

/**
 * Stream structure.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(AVStream) must not be used outside libav*.
 */
typedef struct AVStream {
    int index;    /**< stream index in AVFormatContext */
    /**
     * Format-specific stream ID.
     * decoding: set by libavformat
     * encoding: set by the user, replaced by libavformat if left unset
     */
    int id;
#if FF_API_LAVF_AVCTX
    /**
     * @deprecated use the codecpar struct instead
     */
    attribute_deprecated
    AVCodecContext *codec;
#endif
    void *priv_data;

#if FF_API_LAVF_FRAC
    /**
     * @deprecated this field is unused
     */
    attribute_deprecated
    struct AVFrac pts;
#endif

    /**
     * This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented.
     *
     * decoding: set by libavformat
     * encoding: May be set by the caller before avformat_write_header() to
     *           provide a hint to the muxer about the desired timebase. In
     *           avformat_write_header(), the muxer will overwrite this field
     *           with the timebase that will actually be used for the timestamps
     *           written into the file (which may or may not be related to the
     *           user-provided one, depending on the format).
     */
    /*cwm:todo*/
    //AVRational time_base;

    /**
     * Decoding: pts of the first frame of the stream in presentation order, in stream time base.
     * Only set this if you are absolutely 100% sure that the value you set
     * it to really is the pts of the first frame.
     * This may be undefined (AV_NOPTS_VALUE).
     * @note The ASF header does NOT contain a correct start_time the ASF
     * demuxer must NOT set this.
     */
    int64_t start_time;

    /**
     * Decoding: duration of the stream, in stream time base.
     * If a source file does not specify a duration, but does specify
     * a bitrate, this value will be estimated from bitrate and file size.
     */
    int64_t duration;

    int64_t nb_frames;                 ///< number of frames in this stream if known or 0

    int disposition; /**< AV_DISPOSITION_* bit field */

    enum AVDiscard discard; ///< Selects which packets can be discarded at will and do not need to be demuxed.

    /**
     * sample aspect ratio (0 if unknown)
     * - encoding: Set by user.
     * - decoding: Set by libavformat.
     */
    /*cwm:todo*/
    //AVRational sample_aspect_ratio;

    /*cwm:todo*/
    //AVDictionary *metadata;

    /**
     * Average framerate
     *
     * - demuxing: May be set by libavformat when creating the stream or in
     *             avformat_find_stream_info().
     * - muxing: May be set by the caller before avformat_write_header().
     */
    /*cwm:todo*/
    //AVRational avg_frame_rate;

    /**
     * For streams with AV_DISPOSITION_ATTACHED_PIC disposition, this packet
     * will contain the attached picture.
     *
     * decoding: set by libavformat, must not be modified by the caller.
     * encoding: unused
     */
    AVPacket attached_pic;

    /**
     * An array of side data that applies to the whole stream (i.e. the
     * container does not allow it to change between packets).
     *
     * There may be no overlap between the side data in this array and side data
     * in the packets. I.e. a given side data is either exported by the muxer
     * (demuxing) / set by the caller (muxing) in this array, then it never
     * appears in the packets, or the side data is exported / sent through
     * the packets (always in the first packet where the value becomes known or
     * changes), then it does not appear in this array.
     *
     * - demuxing: Set by libavformat when the stream is created.
     * - muxing: May be set by the caller before avformat_write_header().
     *
     * Freed by libavformat in avformat_free_context().
     *
     * @see av_format_inject_global_side_data()
     */
    /*cwm:todo*/
    //AVPacketSideData *side_data;
    /**
     * The number of elements in the AVStream.side_data array.
     */
    int            nb_side_data;

    /**
     * Flags for the user to detect events happening on the stream. Flags must
     * be cleared by the user once the event has been handled.
     * A combination of AVSTREAM_EVENT_FLAG_*.
     */
    int event_flags;
#define AVSTREAM_EVENT_FLAG_METADATA_UPDATED 0x0001 ///< The call resulted in updated metadata.

    /*****************************************************************
     * All fields below this line are not part of the public API. They
     * may not be used outside of libavformat and can be changed and
     * removed at will.
     * New public fields should be added right above.
     *****************************************************************
     */

    /**
     * Stream information used internally by avformat_find_stream_info()
     */
#define MAX_STD_TIMEBASES (30*12+30+3+6)
    struct {
        int64_t last_dts;
        int64_t duration_gcd;
        int duration_count;
        int64_t rfps_duration_sum;
        double (*duration_error)[2][MAX_STD_TIMEBASES];
        int64_t codec_info_duration;
        int64_t codec_info_duration_fields;

        /**
         * 0  -> decoder has not been searched for yet.
         * >0 -> decoder found
         * <0 -> decoder with codec_id == -found_decoder has not been found
         */
        int found_decoder;

        int64_t last_duration;

        /**
         * Those are used for average framerate estimation.
         */
        int64_t fps_first_dts;
        int     fps_first_dts_idx;
        int64_t fps_last_dts;
        int     fps_last_dts_idx;
    }info[1];


    /*cwm: *infoָ���޸�Ϊ����*/
    int pts_wrap_bits; /**< number of bits in pts (used for wrapping control) */

    // Timestamp generation support:
    /**
     * Timestamp corresponding to the last dts sync point.
     *
     * Initialized when AVCodecParserContext.dts_sync_point >= 0 and
     * a DTS is received from the underlying container. Otherwise set to
     * AV_NOPTS_VALUE by default.
     */
    int64_t first_dts;
    int64_t cur_dts;
    int64_t last_IP_pts;
    int last_IP_duration;

    /**
     * Number of packets to buffer for codec probing
     */
    int probe_packets;

    /**
     * Number of frames that have been demuxed during avformat_find_stream_info()
     */
    int codec_info_nb_frames;

    /* av_read_frame() support */
    enum AVStreamParseType need_parsing;
    /*cwm:todo*/
    //struct AVCodecParserContext *parser;

    /**
     * last packet in packet_buffer for this stream when muxing.
     */
    /*cwm:todo*/
    //struct AVPacketList *last_in_packet_buffer;
    /*cwm:todo*/
    //AVProbeData probe_data;
#define MAX_REORDER_DELAY 16
    int64_t pts_buffer[MAX_REORDER_DELAY+1];

    /*cwm:todo*/
    /*AVIndexEntry *index_entries;*/ /**< Only used if the format does not
                                    support seeking natively. */
    int nb_index_entries;
    unsigned int index_entries_allocated_size;

    /**
     * Real base framerate of the stream.
     * This is the lowest framerate with which all timestamps can be
     * represented accurately (it is the least common multiple of all
     * framerates in the stream). Note, this value is just a guess!
     * For example, if the time base is 1/90000 and all frames have either
     * approximately 3600 or 1800 timer ticks, then r_frame_rate will be 50/1.
     *
     * Code outside avformat should access this field using:
     * av_stream_get/set_r_frame_rate(stream)
     */
    /*cwm:todo*/
    //AVRational r_frame_rate;

    /**
     * Stream Identifier
     * This is the MPEG-TS stream identifier +1
     * 0 means unknown
     */
    int stream_identifier;

    int64_t interleaver_chunk_size;
    int64_t interleaver_chunk_duration;

    /**
     * stream probing state
     * -1   -> probing finished
     *  0   -> no probing requested
     * rest -> perform probing with request_probe being the minimum score to accept.
     * NOT PART OF PUBLIC API
     */
    int request_probe;
    /**
     * Indicates that everything up to the next keyframe
     * should be discarded.
     */
    int skip_to_keyframe;

    /**
     * Number of samples to skip at the start of the frame decoded from the next packet.
     */
    int skip_samples;

    /**
     * If not 0, the number of samples that should be skipped from the start of
     * the stream (the samples are removed from packets with pts==0, which also
     * assumes negative timestamps do not happen).
     * Intended for use with formats such as mp3 with ad-hoc gapless audio
     * support.
     */
    int64_t start_skip_samples;

    /**
     * If not 0, the first audio sample that should be discarded from the stream.
     * This is broken by design (needs global sample count), but can't be
     * avoided for broken by design formats such as mp3 with ad-hoc gapless
     * audio support.
     */
    int64_t first_discard_sample;

    /**
     * The sample after last sample that is intended to be discarded after
     * first_discard_sample. Works on frame boundaries only. Used to prevent
     * early EOF if the gapless info is broken (considered concatenated mp3s).
     */
    int64_t last_discard_sample;

    /**
     * Number of internally decoded frames, used internally in libavformat, do not access
     * its lifetime differs from info which is why it is not in that structure.
     */
    int nb_decoded_frames;

    /**
     * Timestamp offset added to timestamps before muxing
     * NOT PART OF PUBLIC API
     */
    int64_t mux_ts_offset;

    /**
     * Internal data to check for wrapping of the time stamp
     */
    int64_t pts_wrap_reference;

    /**
     * Options for behavior, when a wrap is detected.
     *
     * Defined by AV_PTS_WRAP_ values.
     *
     * If correction is enabled, there are two possibilities:
     * If the first time stamp is near the wrap point, the wrap offset
     * will be subtracted, which will create negative time stamps.
     * Otherwise the offset will be added.
     */
    int pts_wrap_behavior;

    /**
     * Internal data to prevent doing update_initial_durations() twice
     */
    int update_initial_durations_done;

    /**
     * Internal data to generate dts from pts
     */
    int64_t pts_reorder_error[MAX_REORDER_DELAY+1];
    uint8_t pts_reorder_error_count[MAX_REORDER_DELAY+1];

    /**
     * Internal data to analyze DTS and detect faulty mpeg streams
     */
    int64_t last_dts_for_order_check;
    uint8_t dts_ordered;
    uint8_t dts_misordered;

    /**
     * Internal data to inject global side data
     */
    int inject_global_side_data;

    /**
     * String containing paris of key and values describing recommended encoder configuration.
     * Paris are separated by ','.
     * Keys are separated from values by '='.
     */
    char *recommended_encoder_configuration;

    /**
     * display aspect ratio (0 if unknown)
     * - encoding: unused
     * - decoding: Set by libavformat to calculate sample_aspect_ratio internally
     */
    /*cwm:todo*/
    //AVRational display_aspect_ratio;

    /*cwm:todo*/
    //struct FFFrac *priv_pts;

    /**
     * An opaque field for libavformat internal usage.
     * Must not be accessed in any way by callers.
     */
    /*cwm:todo*/
    //AVStreamInternal *internal;

    /*
     * Codec parameters associated with this stream. Allocated and freed by
     * libavformat in avformat_new_stream() and avformat_free_context()
     * respectively.
     *
     * - demuxing: filled by libavformat on stream creation or in
     *             avformat_find_stream_info()
     * - muxing: filled by the caller before avformat_write_header()
     */
    AVCodecParameters codecpar[1];
} AVStream;


/**
 * Format I/O context.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(AVFormatContext) must not be used outside libav*, use
 * avformat_alloc_context() to create an AVFormatContext.
 *
 * Fields can be accessed through AVOptions (av_opt*),
 * the name string used matches the associated command line parameter name and
 * can be found in libavformat/options_table.h.
 * The AVOption/command line parameter names differ in some cases from the C
 * structure field names for historic reasons or brevity.
 */
typedef struct AVFormatContext {
    /**
     * A class for logging and @ref avoptions. Set by avformat_alloc_context().
     * Exports (de)muxer private options if they exist.
     */
    //const AVClass *av_class;/*cwm:todo*/

    /**
     * The input container format.
     *
     * Demuxing only, set by avformat_open_input().
     */
    //struct AVInputFormat *iformat;/*cwm:todo*/

    /**
     * The output container format.
     *
     * Muxing only, must be set by the caller before avformat_write_header().
     */
    //struct AVOutputFormat *oformat;/*cwm:todo*/

    /**
     * Format private data. This is an AVOptions-enabled struct
     * if and only if iformat/oformat.priv_class is not NULL.
     *
     * - muxing: set by avformat_write_header()
     * - demuxing: set by avformat_open_input()
     */
    void *priv_data;
    MpegDemuxContext m_stMDemux;/*cwm:todo add, replease priv_data*/
    int m_iStartcodePrev;/*cwm:todo add, ȷ�����һ��ʹ�ã���������Ƶ���*/
    int m_iVideoEnd;/*cwm:todo add,�Ƿ������һ����Ŀǰ������Ƶ�ж�*/
    int m_iIsPsm;/*cwm:todo add,��ǰpes���Ƿ��ǽ���psm��*/
    int64_t m_i64PsmPos;
    /**
     * I/O context.
     *
     * - demuxing: either set by the user before avformat_open_input() (then
     *             the user must close it manually) or set by avformat_open_input().
     * - muxing: set by the user before avformat_write_header(). The caller must
     *           take care of closing / freeing the IO context.
     *
     * Do NOT set this field if AVFMT_NOFILE flag is set in
     * iformat/oformat.flags. In such a case, the (de)muxer will handle
     * I/O in some other way and this field will be NULL.
     */
    AVIOContext *pb;

    /* stream info */
    /**
     * Flags signalling stream properties. A combination of AVFMTCTX_*.
     * Set by libavformat.
     */
    int ctx_flags;

    /**
     * Number of elements in AVFormatContext.streams.
     *
     * Set by avformat_new_stream(), must not be modified by any other code.
     */
    unsigned int nb_streams;
    /**
     * A list of all streams in the file. New streams are created with
     * avformat_new_stream().
     *
     * - demuxing: streams are created by libavformat in avformat_open_input().
     *             If AVFMTCTX_NOHEADER is set in ctx_flags, then new streams may also
     *             appear in av_read_frame().
     * - muxing: streams are created by the user before avformat_write_header().
     *
     * Freed by libavformat in avformat_free_context().
     */
    AVStream streams[5];/*cwm:todo �޸�Ϊ���飬��**streams��ά��streams[5]һά*/

    /**
     * input or output filename
     *
     * - demuxing: set by avformat_open_input()
     * - muxing: may be set by the caller before avformat_write_header()
     */
    char filename[1024];

    /**
     * Position of the first frame of the component, in
     * AV_TIME_BASE fractional seconds. NEVER set this value directly:
     * It is deduced from the AVStream values.
     *
     * Demuxing only, set by libavformat.
     */
    int64_t start_time;

    /**
     * Duration of the stream, in AV_TIME_BASE fractional
     * seconds. Only set this value if you know none of the individual stream
     * durations and also do not set any of them. This is deduced from the
     * AVStream values if not set.
     *
     * Demuxing only, set by libavformat.
     */
    int64_t duration;

    /**
     * Total stream bitrate in bit/s, 0 if not
     * available. Never set it directly if the file_size and the
     * duration are known as FFmpeg can compute it automatically.
     */
    int64_t bit_rate;

    unsigned int packet_size;
    int max_delay;

    /**
     * Flags modifying the (de)muxer behaviour. A combination of AVFMT_FLAG_*.
     * Set by the user before avformat_open_input() / avformat_write_header().
     */
    int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< Generate missing pts even if it requires parsing future frames.
#define AVFMT_FLAG_IGNIDX       0x0002 ///< Ignore index.
#define AVFMT_FLAG_NONBLOCK     0x0004 ///< Do not block when reading packets from input.
#define AVFMT_FLAG_IGNDTS       0x0008 ///< Ignore DTS on frames that contain both DTS & PTS
#define AVFMT_FLAG_NOFILLIN     0x0010 ///< Do not infer any values from other values, just return what is stored in the container
#define AVFMT_FLAG_NOPARSE      0x0020 ///< Do not use AVParsers, you also must set AVFMT_FLAG_NOFILLIN as the fillin code works on frames and no parsing -> no frames. Also seeking to frames can not work if parsing to find frame boundaries has been disabled
#define AVFMT_FLAG_NOBUFFER     0x0040 ///< Do not buffer frames when possible
#define AVFMT_FLAG_CUSTOM_IO    0x0080 ///< The caller has supplied a custom AVIOContext, don't avio_close() it.
#define AVFMT_FLAG_DISCARD_CORRUPT  0x0100 ///< Discard frames marked corrupted
#define AVFMT_FLAG_FLUSH_PACKETS    0x0200 ///< Flush the AVIOContext every packet.
/**
 * When muxing, try to avoid writing any random/volatile data to the output.
 * This includes any random IDs, real-time timestamps/dates, muxer version, etc.
 *
 * This flag is mainly intended for testing.
 */
#define AVFMT_FLAG_BITEXACT         0x0400
#define AVFMT_FLAG_MP4A_LATM    0x8000 ///< Enable RTP MP4A-LATM payload
#define AVFMT_FLAG_SORT_DTS    0x10000 ///< try to interleave outputted packets by dts (using this flag can slow demuxing down)
#define AVFMT_FLAG_PRIV_OPT    0x20000 ///< Enable use of private options by delaying codec open (this could be made default once all code is converted)
#define AVFMT_FLAG_KEEP_SIDE_DATA 0x40000 ///< Don't merge side data but keep it separate.
#define AVFMT_FLAG_FAST_SEEK   0x80000 ///< Enable fast, but inaccurate seeks for some formats
#define AVFMT_FLAG_SHORTEST   0x100000 ///< Stop muxing when the shortest stream stops.
#define AVFMT_FLAG_AUTO_BSF   0x200000 ///< Wait for packet data before writing a header, and add bitstream filters as requested by the muxer

    /**
     * Maximum size of the data read from input for determining
     * the input container format.
     * Demuxing only, set by the caller before avformat_open_input().
     */
    int64_t probesize;

    /**
     * Maximum duration (in AV_TIME_BASE units) of the data read
     * from input in avformat_find_stream_info().
     * Demuxing only, set by the caller before avformat_find_stream_info().
     * Can be set to 0 to let avformat choose using a heuristic.
     */
    int64_t max_analyze_duration;

    const uint8_t *key;
    int keylen;

    unsigned int nb_programs;
    //AVProgram **programs;/*cwm:todo*/

    /**
     * Forced video codec_id.
     * Demuxing: Set by user.
     */
    enum AVCodecID video_codec_id;

    /**
     * Forced audio codec_id.
     * Demuxing: Set by user.
     */
    enum AVCodecID audio_codec_id;

    /**
     * Forced subtitle codec_id.
     * Demuxing: Set by user.
     */
    enum AVCodecID subtitle_codec_id;

    /**
     * Maximum amount of memory in bytes to use for the index of each stream.
     * If the index exceeds this size, entries will be discarded as
     * needed to maintain a smaller size. This can lead to slower or less
     * accurate seeking (depends on demuxer).
     * Demuxers for which a full in-memory index is mandatory will ignore
     * this.
     * - muxing: unused
     * - demuxing: set by user
     */
    unsigned int max_index_size;

    /**
     * Maximum amount of memory in bytes to use for buffering frames
     * obtained from realtime capture devices.
     */
    unsigned int max_picture_buffer;

    /**
     * Number of chapters in AVChapter array.
     * When muxing, chapters are normally written in the file header,
     * so nb_chapters should normally be initialized before write_header
     * is called. Some muxers (e.g. mov and mkv) can also write chapters
     * in the trailer.  To write chapters in the trailer, nb_chapters
     * must be zero when write_header is called and non-zero when
     * write_trailer is called.
     * - muxing: set by user
     * - demuxing: set by libavformat
     */
    unsigned int nb_chapters;
    //AVChapter **chapters;/*cwm:todo*/

    /**
     * Metadata that applies to the whole file.
     *
     * - demuxing: set by libavformat in avformat_open_input()
     * - muxing: may be set by the caller before avformat_write_header()
     *
     * Freed by libavformat in avformat_free_context().
     */
    //AVDictionary *metadata;/*cwm:todo*/

    /**
     * Start time of the stream in real world time, in microseconds
     * since the Unix epoch (00:00 1st January 1970). That is, pts=0 in the
     * stream was captured at this real world time.
     * - muxing: Set by the caller before avformat_write_header(). If set to
     *           either 0 or AV_NOPTS_VALUE, then the current wall-time will
     *           be used.
     * - demuxing: Set by libavformat. AV_NOPTS_VALUE if unknown. Note that
     *             the value may become known after some number of frames
     *             have been received.
     */
    int64_t start_time_realtime;

    /**
     * The number of frames used for determining the framerate in
     * avformat_find_stream_info().
     * Demuxing only, set by the caller before avformat_find_stream_info().
     */
    int fps_probe_size;

    /**
     * Error recognition; higher values will detect more errors but may
     * misdetect some more or less valid parts as errors.
     * Demuxing only, set by the caller before avformat_open_input().
     */
    int error_recognition;

    /**
     * Custom interrupt callbacks for the I/O layer.
     *
     * demuxing: set by the user before avformat_open_input().
     * muxing: set by the user before avformat_write_header()
     * (mainly useful for AVFMT_NOFILE formats). The callback
     * should also be passed to avio_open2() if it's used to
     * open the file.
     */
    //AVIOInterruptCB interrupt_callback;/*cwm:todo*/

    /**
     * Flags to enable debugging.
     */
    int debug;
#define FF_FDEBUG_TS        0x0001

    /**
     * Maximum buffering duration for interleaving.
     *
     * To ensure all the streams are interleaved correctly,
     * av_interleaved_write_frame() will wait until it has at least one packet
     * for each stream before actually writing any packets to the output file.
     * When some streams are "sparse" (i.e. there are large gaps between
     * successive packets), this can result in excessive buffering.
     *
     * This field specifies the maximum difference between the timestamps of the
     * first and the last packet in the muxing queue, above which libavformat
     * will output a packet regardless of whether it has queued a packet for all
     * the streams.
     *
     * Muxing only, set by the caller before avformat_write_header().
     */
    int64_t max_interleave_delta;

    /**
     * Allow non-standard and experimental extension
     * @see AVCodecContext.strict_std_compliance
     */
    int strict_std_compliance;

    /**
     * Flags for the user to detect events happening on the file. Flags must
     * be cleared by the user once the event has been handled.
     * A combination of AVFMT_EVENT_FLAG_*.
     */
    int event_flags;
#define AVFMT_EVENT_FLAG_METADATA_UPDATED 0x0001 ///< The call resulted in updated metadata.

    /**
     * Maximum number of packets to read while waiting for the first timestamp.
     * Decoding only.
     */
    int max_ts_probe;

    /**
     * Avoid negative timestamps during muxing.
     * Any value of the AVFMT_AVOID_NEG_TS_* constants.
     * Note, this only works when using av_interleaved_write_frame. (interleave_packet_per_dts is in use)
     * - muxing: Set by user
     * - demuxing: unused
     */
    int avoid_negative_ts;
#define AVFMT_AVOID_NEG_TS_AUTO             -1 ///< Enabled when required by target format
#define AVFMT_AVOID_NEG_TS_MAKE_NON_NEGATIVE 1 ///< Shift timestamps so they are non negative
#define AVFMT_AVOID_NEG_TS_MAKE_ZERO         2 ///< Shift timestamps so that they start at 0

    /**
     * Transport stream id.
     * This will be moved into demuxer private options. Thus no API/ABI compatibility
     */
    int ts_id;

    /**
     * Audio preload in microseconds.
     * Note, not all formats support this and unpredictable things may happen if it is used when not supported.
     * - encoding: Set by user via AVOptions (NO direct access)
     * - decoding: unused
     */
    int audio_preload;

    /**
     * Max chunk time in microseconds.
     * Note, not all formats support this and unpredictable things may happen if it is used when not supported.
     * - encoding: Set by user via AVOptions (NO direct access)
     * - decoding: unused
     */
    int max_chunk_duration;

    /**
     * Max chunk size in bytes
     * Note, not all formats support this and unpredictable things may happen if it is used when not supported.
     * - encoding: Set by user via AVOptions (NO direct access)
     * - decoding: unused
     */
    int max_chunk_size;

    /**
     * forces the use of wallclock timestamps as pts/dts of packets
     * This has undefined results in the presence of B frames.
     * - encoding: unused
     * - decoding: Set by user via AVOptions (NO direct access)
     */
    int use_wallclock_as_timestamps;

    /**
     * avio flags, used to force AVIO_FLAG_DIRECT.
     * - encoding: unused
     * - decoding: Set by user via AVOptions (NO direct access)
     */
    int avio_flags;

    /**
     * The duration field can be estimated through various ways, and this field can be used
     * to know how the duration was estimated.
     * - encoding: unused
     * - decoding: Read by user via AVOptions (NO direct access)
     */
    //enum AVDurationEstimationMethod duration_estimation_method;/*cwm:todo*/

    /**
     * Skip initial bytes when opening stream
     * - encoding: unused
     * - decoding: Set by user via AVOptions (NO direct access)
     */
    int64_t skip_initial_bytes;

    /**
     * Correct single timestamp overflows
     * - encoding: unused
     * - decoding: Set by user via AVOptions (NO direct access)
     */
    unsigned int correct_ts_overflow;

    /**
     * Force seeking to any (also non key) frames.
     * - encoding: unused
     * - decoding: Set by user via AVOptions (NO direct access)
     */
    int seek2any;

    /**
     * Flush the I/O context after each packet.
     * - encoding: Set by user via AVOptions (NO direct access)
     * - decoding: unused
     */
    int flush_packets;

    /**
     * format probing score.
     * The maximal score is AVPROBE_SCORE_MAX, its set when the demuxer probes
     * the format.
     * - encoding: unused
     * - decoding: set by avformat, read by user via av_format_get_probe_score() (NO direct access)
     */
    int probe_score;

    /**
     * number of bytes to read maximally to identify format.
     * - encoding: unused
     * - decoding: set by user through AVOPtions (NO direct access)
     */
    int format_probesize;

    /**
     * ',' separated list of allowed decoders.
     * If NULL then all are allowed
     * - encoding: unused
     * - decoding: set by user through AVOptions (NO direct access)
     */
    char *codec_whitelist;

    /**
     * ',' separated list of allowed demuxers.
     * If NULL then all are allowed
     * - encoding: unused
     * - decoding: set by user through AVOptions (NO direct access)
     */
    char *format_whitelist;

    /**
     * An opaque field for libavformat internal usage.
     * Must not be accessed in any way by callers.
     */
    //AVFormatInternal *internal;/*cwm:todo*/

    /**
     * IO repositioned flag.
     * This is set by avformat when the underlaying IO context read pointer
     * is repositioned, for example when doing byte based seeking.
     * Demuxers can use the flag to detect such changes.
     */
    int io_repositioned;

    /**
     * Forced video codec.
     * This allows forcing a specific decoder, even when there are multiple with
     * the same codec_id.
     * Demuxing: Set by user via av_format_set_video_codec (NO direct access).
     */
    //AVCodec *video_codec;/*cwm:todo*/

    /**
     * Forced audio codec.
     * This allows forcing a specific decoder, even when there are multiple with
     * the same codec_id.
     * Demuxing: Set by user via av_format_set_audio_codec (NO direct access).
     */
    //AVCodec *audio_codec;/*cwm:todo*/

    /**
     * Forced subtitle codec.
     * This allows forcing a specific decoder, even when there are multiple with
     * the same codec_id.
     * Demuxing: Set by user via av_format_set_subtitle_codec (NO direct access).
     */
    //AVCodec *subtitle_codec;/*cwm:todo*/

    /**
     * Forced data codec.
     * This allows forcing a specific decoder, even when there are multiple with
     * the same codec_id.
     * Demuxing: Set by user via av_format_set_data_codec (NO direct access).
     */
    //AVCodec *data_codec;/*cwm:todo*/

    /**
     * Number of bytes to be written as padding in a metadata header.
     * Demuxing: Unused.
     * Muxing: Set by user via av_format_set_metadata_header_padding.
     */
    int metadata_header_padding;

    /**
     * User data.
     * This is a place for some private data of the user.
     */
    void *opaque;

    /**
     * Callback used by devices to communicate with application.
     */
    //av_format_control_message control_message_cb;/*cwm:todo*/

    /**
     * Output timestamp offset, in microseconds.
     * Muxing: set by user via AVOptions (NO direct access)
     */
    int64_t output_ts_offset;

    /**
     * dump format separator.
     * can be ", " or "\n      " or anything else
     * Code outside libavformat should access this field using AVOptions
     * (NO direct access).
     * - muxing: Set by user.
     * - demuxing: Set by user.
     */
    uint8_t *dump_separator;

    /**
     * Forced Data codec_id.
     * Demuxing: Set by user.
     */
    enum AVCodecID data_codec_id;

#if FF_API_OLD_OPEN_CALLBACKS
    /**
     * Called to open further IO contexts when needed for demuxing.
     *
     * This can be set by the user application to perform security checks on
     * the URLs before opening them.
     * The function should behave like avio_open2(), AVFormatContext is provided
     * as contextual information and to reach AVFormatContext.opaque.
     *
     * If NULL then some simple checks are used together with avio_open2().
     *
     * Must not be accessed directly from outside avformat.
     * @See av_format_set_open_cb()
     *
     * Demuxing: Set by user.
     *
     * @deprecated Use io_open and io_close.
     */
    attribute_deprecated
    int (*open_cb)(struct AVFormatContext *s, AVIOContext **p, const char *url, int flags, const AVIOInterruptCB *int_cb, AVDictionary **options);
#endif

    /**
     * ',' separated list of allowed protocols.
     * - encoding: unused
     * - decoding: set by user through AVOptions (NO direct access)
     */
    char *protocol_whitelist;

    /*
     * A callback for opening new IO streams.
     *
     * Whenever a muxer or a demuxer needs to open an IO stream (typically from
     * avformat_open_input() for demuxers, but for certain formats can happen at
     * other times as well), it will call this callback to obtain an IO context.
     *
     * @param s the format context
     * @param pb on success, the newly opened IO context should be returned here
     * @param url the url to open
     * @param flags a combination of AVIO_FLAG_*
     * @param options a dictionary of additional options, with the same
     *                semantics as in avio_open2()
     * @return 0 on success, a negative AVERROR code on failure
     *
     * @note Certain muxers and demuxers do nesting, i.e. they open one or more
     * additional internal format contexts. Thus the AVFormatContext pointer
     * passed to this callback may be different from the one facing the caller.
     * It will, however, have the same 'opaque' field.
     */
    /*cwm:todo*/
    #if 0
    int (*io_open)(struct AVFormatContext *s, AVIOContext **pb, const char *url,
                   int flags, AVDictionary **options);

    /**
     * A callback for closing the streams opened with AVFormatContext.io_open().
     */
    void (*io_close)(struct AVFormatContext *s, AVIOContext *pb);
    #endif
    /**
     * ',' separated list of disallowed protocols.
     * - encoding: unused
     * - decoding: set by user through AVOptions (NO direct access)
     */
    char *protocol_blacklist;

    /**
     * The maximum number of streams.
     * - encoding: unused
     * - decoding: set by user through AVOptions (NO direct access)
     */
    int max_streams;
} AVFormatContext;


#if 0
typedef struct AVCodecParameters {
    /**
     * General type of the encoded data.
     */
    enum AVMediaType codec_type;
    /**
     * Specific type of the encoded data (the codec used).
     */
    enum AVCodecID   codec_id;
    /**
     * Additional information about the codec (corresponds to the AVI FOURCC).
     */
    uint32_t         codec_tag;

    /**
     * Extra binary data needed for initializing the decoder, codec-dependent.
     *
     * Must be allocated with av_malloc() and will be freed by
     * avcodec_parameters_free(). The allocated size of extradata must be at
     * least extradata_size + AV_INPUT_BUFFER_PADDING_SIZE, with the padding
     * bytes zeroed.
     */
    uint8_t *extradata;
    /**
     * Size of the extradata content in bytes.
     */
    int      extradata_size;

    /**
     * - video: the pixel format, the value corresponds to enum AVPixelFormat.
     * - audio: the sample format, the value corresponds to enum AVSampleFormat.
     */
    int format;

    /**
     * The average bitrate of the encoded data (in bits per second).
     */
    int64_t bit_rate;

    /**
     * The number of bits per sample in the codedwords.
     *
     * This is basically the bitrate per sample. It is mandatory for a bunch of
     * formats to actually decode them. It's the number of bits for one sample in
     * the actual coded bitstream.
     *
     * This could be for example 4 for ADPCM
     * For PCM formats this matches bits_per_raw_sample
     * Can be 0
     */
    int bits_per_coded_sample;

    /**
     * This is the number of valid bits in each output sample. If the
     * sample format has more bits, the least significant bits are additional
     * padding bits, which are always 0. Use right shifts to reduce the sample
     * to its actual size. For example, audio formats with 24 bit samples will
     * have bits_per_raw_sample set to 24, and format set to AV_SAMPLE_FMT_S32.
     * To get the original sample use "(int32_t)sample >> 8"."
     *
     * For ADPCM this might be 12 or 16 or similar
     * Can be 0
     */
    int bits_per_raw_sample;

    /**
     * Codec-specific bitstream restrictions that the stream conforms to.
     */
    int profile;
    int level;

    /**
     * Video only. The dimensions of the video frame in pixels.
     */
    int width;
    int height;

    /**
     * Video only. The aspect ratio (width / height) which a single pixel
     * should have when displayed.
     *
     * When the aspect ratio is unknown / undefined, the numerator should be
     * set to 0 (the denominator may have any value).
     */
    /*cwm:todo*/
    //AVRational sample_aspect_ratio;

    /**
     * Video only. The order of the fields in interlaced video.
     */
    /*cwm:todo*/
    //enum AVFieldOrder                  field_order;

    /**
     * Video only. Additional colorspace characteristics.
     */
    /*cwm:todo*/
    #if 0
    enum AVColorRange                  color_range;
    enum AVColorPrimaries              color_primaries;
    enum AVColorTransferCharacteristic color_trc;
    enum AVColorSpace                  color_space;
    enum AVChromaLocation              chroma_location;
    #endif
    
    /**
     * Video only. Number of delayed frames.
     */
    int video_delay;

    /**
     * Audio only. The channel layout bitmask. May be 0 if the channel layout is
     * unknown or unspecified, otherwise the number of bits set must be equal to
     * the channels field.
     */
    uint64_t channel_layout;
    /**
     * Audio only. The number of audio channels.
     */
    int      channels;
    /**
     * Audio only. The number of audio samples per second.
     */
    int      sample_rate;
    /**
     * Audio only. The number of bytes per coded audio frame, required by some
     * formats.
     *
     * Corresponds to nBlockAlign in WAVEFORMATEX.
     */
    int      block_align;
    /**
     * Audio only. Audio frame size, if known. Required by some formats to be static.
     */
    int      frame_size;

    /**
     * Audio only. The amount of padding (in samples) inserted by the encoder at
     * the beginning of the audio. I.e. this number of leading decoded samples
     * must be discarded by the caller to get the original audio without leading
     * padding.
     */
    int initial_padding;
    /**
     * Audio only. The amount of padding (in samples) appended by the encoder to
     * the end of the audio. I.e. this number of decoded samples must be
     * discarded by the caller from the end of the stream to get the original
     * audio without any trailing padding.
     */
    int trailing_padding;
    /**
     * Audio only. Number of samples to skip after a discontinuity.
     */
    int seek_preroll;
} AVCodecParameters;
#endif


typedef struct SyncFrame
{
    int64_t m_i64Pos;/*cwm:�ļ�λ��*/
    int64_t m_i64Timestamp;/*cwm:ʱ���*/
} SyncFrame;

typedef struct SyncFrameInfo
{
    int64_t m_i64StartTime;
    int64_t m_i64EndTime;
    int64_t m_i64FileSize;
    int m_iMaxFrameLenV;
} SyncFrameInfo;

typedef struct MpegDec {
    AVFormatContext m_stAVFormatContext;
    AVIOContext m_stAVIOContext;
    AVPacket m_stAVPacket;
    SyncFrameInfo m_stSyncFrameInfo;
} MpegDec;

int Mpeg_DecProbe(char *_pFileName);

int Mpeg_DecInit(MpegDec *_pstMpegDec, char *_pFileName);

int Mpeg_DecGetProgramInfo(MpegDec *_pstMpegDec, char **_ppPSInfo, int *_puiLen);

AVCodecID Mpeg_DecGetVideoCodecId(MpegDec *_pstMpegDec);

AVCodecID Mpeg_DecGetAudioCodecId(MpegDec *_pstMpegDec);

int Mpeg_DecGetVideoESInfo(MpegDec *_pstMpegDec, char **_ppcESInfoData, int *_piESInfoLen);

int Mpeg_DecGetAudioESInfo(MpegDec *_pstMpegDec, char **_ppcESInfoData, int *_piESInfoLen);

int Mpeg_DecMaxFrameLength(MpegDec *_pstMpegDec);

int Mpeg_DecReadES(MpegDec *_pstMpegDec, int *_piIsSyncFrame);

int Mpeg_DecSeek(MpegDec *_pstMpegDec, int64_t _i64Timestamp);

int64_t Mpeg_DecGetStartTime(MpegDec *_pstMpegDec);

int64_t Mpeg_DecGetEndTime(MpegDec *_pstMpegDec);

int Mpeg_DecUninit(MpegDec *_pstMpegDec);

int Demo_Demux(int argc, char **argv);

#endif //_MEDIAMPEGDEC_H
