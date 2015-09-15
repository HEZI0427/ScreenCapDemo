#ifndef __OPENIMAGE_H__
#define __OPENIMAGE_H__

extern "C" {
#include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>

#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavutil/pixfmt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

#include <libswscale/swscale.h>

}

#include "screencap.h"

#define USE_PIPE

static const int BUF_SIZE = 4096;
static const int  MAX_SIZE = 4096;
static const int frame_rate = 15;
static const int IMAGE_MAP_SIZE = 16 * 1024 * 1024;

//process input file and out put it to output file
struct filter {

    AVCodecContext *p_codec_ctx_enc;
    AVCodecContext *p_codec_ctx_dec;

    AVFrame *p_frame_dec;
    AVFrame *p_frame_enc;

    AVFormatContext *p_fmt_ctx_dec;
    AVFormatContext *p_fmt_ctx_enc;

    uint8_t *p_buffer_dec;
    uint8_t *p_buffer_enc;

    int width_enc;
    int height_enc;

    // outputfile codec
    enum AVCodecID codec_id_enc;
};

struct pixel {
    //pixel buffer
    void *buffer;

    //buffer size
    size_t size;

    //image type
    enum image_type type;
};

struct image_info {
    int width;
    int height;
    enum AVPixelFormat pix_fmt;
};


extern "C" {

void *local_server(void *arg);

int local_client(int *sfd);

//openimage.cpp
void register_ffmpeg();

void local_sleep();

void scale_avframe(struct filter *filter);

uint8_t *get_buffer(enum AVPixelFormat pixel_fmt, int width, int height);

void get_image_info(const char * filename, enum image_type type, struct image_info * image_info);

int open_input_file(const char *filename, struct filter *filter);

AVCodecContext *initialize_codec_ctx_enc(enum AVCodecID codec_id, int width, int height);

void encode_frame(const int of_fd, struct filter *filter, uint64_t &pts_index);

void process_file(const int of_fd, struct filter *filter, uint64_t &pts_index, int video_stream_index);

void initialize_avframe_dec(const char *if_filepath, struct filter *filter, enum image_type if_type);

void initialize_filter(struct filter *filter, int width_enc, int height_enc, enum AVCodecID codec_id);

void uninitialize_filter(struct filter *filter);

}

#endif
