#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "openimage.h"

int main()
{
    int fd_out = -1;
    int image_fd = -1;

    const enum AVCodecID codec_id_enc = AV_CODEC_ID_H263P;

    const int width_enc = 1152;
    const int height_enc = 648;


    const char *output_path = "/data/screen";
    const char *p_pipe_name = "video.263";
    const char *p_native_name = "video.h263";
    const char *p_if_name = "mirror";
    void *image_map = MAP_FAILED;

    const enum image_type if_type = ID_BMP;

    DIR *dir;

    uint64_t pts_index = 1;
    struct filter filter = {
        .p_codec_ctx_enc = NULL,
        .p_codec_ctx_dec = NULL,
        .p_frame_dec = NULL,
        .p_frame_enc = NULL,
        .p_fmt_ctx_dec = NULL,
        .p_fmt_ctx_enc = NULL,
        .p_buffer_dec = NULL,
        .p_buffer_enc = NULL,
        .width_enc = -1,
        .height_enc = -1,
        .codec_id_enc = (enum AVCodecID) 0
    };

    int video_stream_index = -1;

    char if_filepath[MAX_SIZE] = {'\0', };
    char of_filepath[MAX_SIZE] = {'\0', };

    //just test the parent dir of screencap video file is exist, if not exist, and creat it
    if ((dir = opendir(output_path)) == NULL) {
        if (mkdir(output_path, 0755) == -1) {
            fprintf(stderr, "file %s, line%d, mkdir error\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    closedir(dir);

    switch (if_type) {
    case ID_JPEG:
        sprintf(if_filepath, "%s/%s.jpg", output_path, p_if_name);
        break;
    case ID_BMP:
        sprintf(if_filepath, "%s/%s.bmp", output_path, p_if_name);
        break;
    default:
        break;
    }

#ifndef USE_PIPE
    //just save the screencap video to native test, but not to send
    sprintf(of_filepath, "%s/%s", output_path, p_native_name);

    fd_out = open(of_filepath, O_WRONLY | O_CREAT, 0644);
    if (fd_out == -1 ) {
        fprintf(stderr, "file %s,line%d, open of_filepath error \n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

#else
    //use pipe to send the screencap video
    sprintf(of_filepath, "%s/%s", output_path, p_pipe_name);

    if (remove(of_filepath) == -1) {
        fprintf(stderr, "remove of_filepath error\n");
//        exit(EXIT_FAILURE);
    }

    if (mkfifo(of_filepath, 0644) == -1) {
        fprintf(stderr, "mkfifo of_filepath error\n");
        exit(EXIT_FAILURE);
    }

    fd_out = open(of_filepath, O_RDWR);
    if (fd_out == -1 ) {
        fprintf(stderr, "file %s,line%d, open of_filepath error \n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif

    image_fd = open(if_filepath, O_RDWR | O_CREAT, 0644);
    if (image_fd == -1) {
        fprintf(stderr, "file %s,line%d, open if_filepath error \n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    // enlarge the file
    lseek(image_fd, IMAGE_MAP_SIZE - 1 , SEEK_SET);
    write(image_fd, "", MAX_SIZE);

    //just map the image file to memory
    image_map = mmap(NULL, IMAGE_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, image_fd, 0);
    if (image_map == MAP_FAILED) {
        fprintf(stderr, "file %s,line%d, mmap error \n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    register_ffmpeg();

#if 1
    //initialize some data structure for decode and encode
    initialize_filter(&filter, width_enc, height_enc, codec_id_enc);

    for (; ; pts_index++) {
        GetPixels(image_map, if_type, width_enc, height_enc);

        //initialize avframe for decode file
        initialize_avframe_dec(if_filepath, &filter, if_type);

        // process input file
        video_stream_index = open_input_file(if_filepath, &filter);
        if (video_stream_index < 0) {
            fprintf(stderr, "open_input_file error\n");
            exit(EXIT_FAILURE);
        }

        // decode file into avframe and encode avframe into another video
        process_file(fd_out, &filter, pts_index, video_stream_index);

        local_sleep();
    }

#endif
    close(image_fd);
    close(fd_out);
    //destroy the data structure
    uninitialize_filter(&filter);

    if (image_map != MAP_FAILED) {
        munmap((void *)image_map, IMAGE_MAP_SIZE);
    }

    return 0;
}
