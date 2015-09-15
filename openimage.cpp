#include <stdio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
#include <jpeglib.h>

#include "openimage.h"
#include "bmp.h"

/**
 */
void register_ffmpeg()
{
    avcodec_register_all();
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avfilter_register_all();
    av_register_all();
    avformat_network_init();
}

void local_sleep()
{
    struct timespec previous_tp;
    struct timespec current_tp;
    struct timespec request;
    struct timespec remain;
    int ret;

    int frame_time = 1000000000.0/frame_rate;
    int delta_t;

    if (clock_gettime(CLOCK_REALTIME, &previous_tp) == -1) {
        fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

//    fprintf(stderr, "previous_tp.tv_sec %ld, previous_tp.tv_nsec %ld\n", previous_tp.tv_sec, previous_tp.tv_nsec);
    if (clock_gettime(CLOCK_REALTIME, &current_tp) == -1) {
        fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
//    fprintf(stderr, "current_tp.tv_sec %ld, current_tp.tv_nsec %ld\n", current_tp.tv_sec, current_tp.tv_nsec);

    delta_t = (current_tp.tv_sec - previous_tp.tv_sec) * 1000000000.0 +  (current_tp.tv_nsec - previous_tp.tv_nsec);

//    fprintf(stderr, "delta_t is %d\n", delta_t);
//    fprintf(stderr, "frame_time is %d\n", frame_time);

    if (delta_t < frame_time) {

        request.tv_sec = 0;
        request.tv_nsec = frame_time - delta_t;

//        fprintf(stderr, "request.tv_sec is %ld, request.tv_nsec is %ld\n", request.tv_sec, request.tv_nsec);

        for (;;) {
            ret = nanosleep(&request, &remain);

            if (ret == -1 && errno != EINTR) {
                fprintf(stderr, "file %s, line %d, nanosleep error\n", __FILE__, __LINE__);
            }

            if (ret == 0) {
                break;
            }
            request = remain;
        }
    }
}

/**
 * scale a frame
 */
void scale_avframe(struct filter *filter)
//AVFrame * resize_avframe(AVFrame *frame_dec, uint8_t *buffer, int width_enc, int height_enc)
{
    struct SwsContext *sws_ctx;

    filter->p_frame_enc->width = filter->width_enc;
    filter->p_frame_enc->height = filter->height_enc;
//    filter->p_frame_enc->format = filter->p_frame_dec->format;  // by chen
    filter->p_frame_enc->format = AV_PIX_FMT_YUV420P;

fprintf(stderr, "filter->p_frame_dec->format %d\n", filter->p_frame_dec->format);
fprintf(stderr, "filter->p_frame_enc->format %d\n", filter->p_frame_enc->format);
    avpicture_fill((AVPicture*) filter->p_frame_enc, filter->p_buffer_enc,
                   AV_PIX_FMT_YUV420P, filter->width_enc, filter->height_enc);

    sws_ctx = sws_getContext(filter->p_frame_dec->width, filter->p_frame_dec->height,
                 (enum AVPixelFormat) filter->p_frame_dec->format, filter->p_frame_enc->width, filter->p_frame_enc->height,
                 (enum AVPixelFormat) filter->p_frame_enc->format, SWS_POINT, NULL, NULL, NULL);
    if (!sws_ctx) {
       fprintf(stderr, "sws_getContextet error\n");
//       return NULL;
    }

    sws_scale(sws_ctx, (const uint8_t * const*) filter->p_frame_dec->data,
              filter->p_frame_dec->linesize, 0, filter->p_frame_dec->height,
              filter->p_frame_enc->data, filter->p_frame_enc->linesize);

    sws_freeContext(sws_ctx);
}

uint8_t *get_buffer(enum AVPixelFormat pixel_fmt, int width, int height)
{
    int nbytes = 0;
    uint8_t *buffer = NULL;

    nbytes = avpicture_get_size(pixel_fmt, width, height);
    buffer = (uint8_t *) av_malloc(nbytes * sizeof(uint8_t));

    return buffer;
}

void get_image_info(const char * filename, enum image_type type, struct image_info * image_info)
{
    FILE * infile = NULL;
    int yuv[3] = {0, };

    if ((infile = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        exit(EXIT_FAILURE);
    }

    switch (type) {
    case ID_JPEG:
        //initialize decompression
        {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);

        jpeg_stdio_src(&cinfo, infile);
        jpeg_read_header(&cinfo, TRUE);

        image_info->width = cinfo.image_width;
        image_info->height = cinfo.image_height;

        yuv[0] = cinfo.comp_info[0].h_samp_factor * cinfo.comp_info[0].v_samp_factor;
        yuv[1] = cinfo.comp_info[1].h_samp_factor * cinfo.comp_info[1].v_samp_factor;
        yuv[2] = cinfo.comp_info[2].h_samp_factor * cinfo.comp_info[2].v_samp_factor;

        if ((yuv[0] == 4) && (yuv[1] = 1) && (yuv[2] = 1)) {
            image_info->pix_fmt = AV_PIX_FMT_YUV420P;
        }
        if ((yuv[0] == 2) && (yuv[1] = 1) && (yuv[2] = 1)) {
            image_info->pix_fmt = AV_PIX_FMT_YUV422P;
        }
        if ((yuv[0] == 1) && (yuv[1] = 1) && (yuv[2] = 1)) {
            image_info->pix_fmt = AV_PIX_FMT_YUV444P;
        }
        jpeg_destroy_decompress(&cinfo);
        }
        break;
    case ID_BMP:

        BITMAPFILE bmpfile;
        fread(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, infile);
        fread(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, infile);

        image_info->width = bmpfile.biInfo.bmiHeader.biWidth;
        image_info->height = bmpfile.biInfo.bmiHeader.biHeight;
        image_info->pix_fmt = AV_PIX_FMT_BGRA;
        break;

    default:
        break;
    }
    fclose(infile);
}

int open_input_file(const char *filename, struct filter *filter)
{
    int ret = -1;
    AVCodec *p_codec = NULL;
    int video_stream_index = -1;

    if ((ret = avformat_open_input(&filter->p_fmt_ctx_dec, filename, NULL, NULL)) < 0) {
        fprintf(stderr, "Can't open image file '%s'\n", filename);
        return ret;
    }

    av_dump_format(filter->p_fmt_ctx_dec, 0, filename, 0);

//    if ((ret = avformat_find_stream_info(filter->p_fmt_ctx_dec, NULL)) < 0) {
//        fprintf(stderr, "avformat_find_stream_info error\n");
//        return ret;
//    }

    /* select the video stream */
    ret = av_find_best_stream(filter->p_fmt_ctx_dec, AVMEDIA_TYPE_VIDEO, -1, -1, &p_codec, 0);
    if (ret < 0) {
        fprintf(stderr, "av_find_best_stream error\n");
        return ret;
    }

    video_stream_index = ret;
//    filter->p_codec_ctx_dec = filter->p_fmt_ctx_dec->streams[video_stream_index]->codec;

    filter->p_codec_ctx_dec = avcodec_alloc_context3(p_codec);
    avcodec_copy_context(filter->p_codec_ctx_dec,
                         filter->p_fmt_ctx_dec->streams[video_stream_index]->codec);

fprintf(stderr, "p_codecctx->width %d, p_codecctx->height %d\n", filter->p_codec_ctx_dec->width, filter->p_codec_ctx_dec->height);
    // Open codec
    if((ret = avcodec_open2(filter->p_codec_ctx_dec, p_codec, NULL)) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return ret;
    }

    return video_stream_index;
}

// initiate and AVCodecContext
AVCodecContext *initialize_codec_ctx_enc(enum AVCodecID codec_id, int width, int height)
{
    AVCodecContext *p_codec_ctx = NULL;
    AVCodec *p_codec = NULL;

    /* find the video encoder */
    p_codec = avcodec_find_encoder(codec_id);
    if (!p_codec) {
        fprintf(stderr, "file %s, line %d, p_codec not found\n", __FILE__, __LINE__);
        return NULL;
    }

    /* for output file */
    p_codec_ctx = avcodec_alloc_context3(p_codec);
    if (!p_codec_ctx) {
        fprintf(stderr, "Could not allocate video p_codec context\n");
        return NULL;
    }

    /* put sample parameters */
    p_codec_ctx->bit_rate = 400000;

    /* resolution must be a multiple of two */
    p_codec_ctx->width = width;
    p_codec_ctx->height = height;
    /* frames per second */
    p_codec_ctx->time_base = (AVRational){1,frame_rate};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    p_codec_ctx->gop_size = 5;

    if (codec_id != AV_CODEC_ID_H263P) {
        p_codec_ctx->max_b_frames = 1;
    }

    p_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
//    p_codec_ctx->pix_fmt = AV_PIX_FMT_BGRA;
    av_opt_set(p_codec_ctx->priv_data, "preset", "superfast", 0);

    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(p_codec_ctx->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(p_codec_ctx, p_codec, NULL) < 0) {
        fprintf(stderr, "Could not open p_codec\n");
        return NULL;
    }

    return p_codec_ctx;
}

void encode_frame(const int of_fd, struct filter *filter, uint64_t &pts_index)
{
    AVPacket packet;
    int got_output;
    int ret = -1;

    /* encode video */
    fflush(stdout);

    av_init_packet(&packet);
    packet.data = NULL;    // packet data will be allocated by the encoder
    packet.size = 0;

    filter->p_frame_dec->pts = pts_index;
    /* encode the image */
    ret = avcodec_encode_video2(filter->p_codec_ctx_enc, &packet, (const AVFrame *)filter->p_frame_enc, &got_output);
    if (ret < 0) {
        fprintf(stderr, "Error encoding frame_dec\n");
        exit(1);
    }

    if (got_output) {
//    	fprintf(stderr, "Write frame_dec %4llu (size=%5d)\n", pts_index, packet.size);

		int num = 0;
		for (int i = 0; i < packet.size / 4096; i++) {
			num += write(of_fd, packet.data + i * 4096, 4096);
			if ((i + 1) == (int)(packet.size / 4096)) {
				num += write(of_fd, packet.data + (i + 1) * 4096, packet.size % 4096);
				break ;
			}
		}

        av_free_packet(&packet);
    }

#if 0
        /* get the delayed frames */
    for (got_output = 1; got_output; pts_index++) {
fprintf(stderr, "pts_index is %llu", pts_index);
        fflush(stdout);

        ret = avcodec_encode_video2(filter->p_codec_ctx_enc, &packet, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame_dec\n");
            exit(1);
        }

        if (got_output) {
            printf("Write frame_dec %4lld (size=%5d)\n", pts_index, packet.size);
            fwrite(packet.data, 1, packet.size, f);
            av_free_packet(&packet);
        }
    }
#endif
    // fclose(f);
}

void process_file(const int of_fd, struct filter *filter, uint64_t &pts_index, int video_stream_index)
{
    int got_frame = 0;
    AVPacket packet;
    int ret = -1;

    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    while (av_read_frame(filter->p_fmt_ctx_dec, &packet) == 0) {

        if (packet.stream_index == video_stream_index) {
            avcodec_get_frame_defaults(filter->p_frame_dec);
//            fprintf(stderr, "file %s, line %d, i is %d\n", __FILE__, __LINE__, i++);

            ret = avcodec_decode_video2(filter->p_codec_ctx_dec, filter->p_frame_dec, &got_frame, &packet);
            if (ret < 0) {
                fprintf(stderr, "avcodec_decode_video2 error\n");
                break;
            }

            if (got_frame) {
//                 fprintf(stderr, "Frame is decoded, size %d\n", ret);
                 scale_avframe(filter);
                encode_frame(of_fd, filter, pts_index);
                avcodec_close(filter->p_codec_ctx_dec);
                av_free(filter->p_codec_ctx_dec);
//                avformat_close_input(&filter->p_fmt_ctx_dec);
//                avformat_free_context(filter->p_fmt_ctx_dec);
//                return p_frame;
            }
        }
        av_free_packet(&packet);
    }
    avformat_close_input(&filter->p_fmt_ctx_dec);
}

void initialize_avframe_dec(const char *if_filepath, struct filter *filter, enum image_type if_type)
{
    struct image_info image_info;

    //get information of picture
    get_image_info(if_filepath, if_type, &image_info);

fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
    filter->p_frame_dec->width = image_info.width;
    filter->p_frame_dec->height = image_info.height;
    filter->p_frame_dec->format = image_info.pix_fmt;
    filter->p_frame_dec->quality = 4;

//    filter->p_buffer_dec = get_buffer(image_info.pix_fmt, image_info.width, image_info.height);
}

#if 1
void initialize_filter(struct filter *filter, int width_enc, int height_enc, enum AVCodecID codec_id)
{
    filter->width_enc = width_enc;
    filter->height_enc = height_enc;
    filter->codec_id_enc = codec_id;

    //AVCodecContext for encode video
    filter->p_codec_ctx_enc = initialize_codec_ctx_enc(codec_id, width_enc, height_enc);
    if (filter->p_codec_ctx_enc == NULL) {
        fprintf(stderr, "initialize_codec_ctx_enc error\n");
        exit(EXIT_FAILURE);
    }

//    filter->p_codec_ctx_dec = avcodec_alloc_context3();

    // AVFormatContext for decode file
    filter->p_fmt_ctx_dec = avformat_alloc_context();
    if (filter->p_fmt_ctx_dec == NULL) {
        fprintf(stderr, "avformat_alloc_context error\n");
        exit(EXIT_FAILURE);
    }

#if 0
    filter->p_fmt_ctx_enc = avformat_alloc_context();
    if (filter->p_fmt_ctx_enc == NULL) {
        fprintf(stderr, "avformat_alloc_context error\n");
        exit(EXIT_FAILURE);
    }
#endif

    // AVFrame for decode file
    filter->p_frame_dec = avcodec_alloc_frame();
    if (!filter->p_frame_dec) {
        printf("Can't allocate memory for frame_dec\n");
        exit(EXIT_FAILURE);
    }

    // AVFrame for encode file
    filter->p_frame_enc = avcodec_alloc_frame();
    if (!filter->p_frame_enc) {
        printf("Can't allocate memory for frame_enc\n");
        exit(EXIT_FAILURE);
    }

    //alloc dst buffer which attach to frame_enc->data
     filter->p_buffer_enc = get_buffer(AV_PIX_FMT_YUV420P, width_enc, height_enc);
//    filter->p_buffer_enc = get_buffer(AV_PIX_FMT_BGRA, width_enc, height_enc);
    if (filter->p_buffer_enc == NULL) {
        fprintf(stderr, "frame_enc error\n");
        exit(EXIT_FAILURE);
    }

}
#endif

void uninitialize_filter(struct filter *filter)
{
    if (filter->p_codec_ctx_enc) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
        avcodec_close(filter->p_codec_ctx_enc);
        av_free(filter->p_codec_ctx_enc);
    }

    if (filter->p_codec_ctx_dec) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
//        avcodec_close(filter->p_codec_ctx_dec);
//        av_free(filter->p_codec_ctx_dec);
    }

    if (filter->p_frame_dec) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
//        av_frame_free(&filter->p_frame_dec);
        av_free(filter->p_frame_dec);
    }

    if (filter->p_frame_enc) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
//        av_frame_free(&filter->p_frame_enc);
        av_free(filter->p_frame_enc);
    }

    if (filter->p_buffer_dec) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
        av_free(filter->p_buffer_dec);
    }

    if (filter->p_buffer_enc) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
        av_free(filter->p_buffer_dec);
    }

    if (filter->p_fmt_ctx_dec) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
//        avformat_free_context(filter->p_fmt_ctx_dec);
//        avformat_close_input(&filter->p_fmt_ctx_dec);
    }

    if (filter->p_fmt_ctx_enc) {
// fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
        avformat_free_context(filter->p_fmt_ctx_enc);
    }
}

}//extern "C"
