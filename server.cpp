#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/un.h>

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

using namespace std;

/**
 * local_server read pictures buffers from local socket and
 * encode them into video stream.
 */
void * local_server(void *arg)
{
    struct sockaddr_un addr;
    int sfd, cfd;
    ssize_t numRead;
    char buf[BUF_SIZE];
    char file_path[256];

    int fd = -1;

    long fflag;
    DIR *dir;

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        fprintf(stderr, "socket error\n");
        exit(EXIT_FAILURE);
    }

    /* Construct server socket address, bind socket to it,
       and make this a listening socket */

    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT) {
        fprintf(stderr, "remove-%s", SV_SOCK_PATH);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        fprintf(stderr, "bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, BACKLOG) == -1) {
        fprintf(stderr, "listen error\n");
        exit(EXIT_FAILURE);
    }

    if ((dir = opendir(output_path)) == NULL) {
        if (mkdir(output_path, 0755) == -1) {
            fprintf(stderr, "file %s, line%d, mkdir error\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    closedir(dir);

    for (; ; pts_index++) {          /* Handle client connections iteratively */

        /* Accept a connection. The connection is returned on a new
           socket, 'cfd'; the listening socket ('sfd') remains open
           and can be used to accept further connections. */

        cfd = accept(sfd, NULL, NULL);

        if (cfd == -1) {
            fprintf(stderr, "accept error\n");
            exit(EXIT_FAILURE);
        }

//        sprintf(file_path, "%s/%d.jpg", output_path, i++);
        sprintf(file_path, "%s/mirror.jpg", output_path);
        fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0664);
        fprintf(stderr, "file name is %s\n", file_path);
        if (fd == -1) {
            fprintf(stderr, "open error\n");
            exit(EXIT_FAILURE);
        }

//        pthread_mutex_lock(&mtx);
        /* Transfer data from connected socket to stdout until EOF */
        while ((numRead = read(cfd, buf, BUF_SIZE)) > 0) {
//            fprintf(stderr, "file %s, line %d, numRead is %d\n", __FILE__, __LINE__, numRead);

            if (write(fd, buf, numRead) != numRead) {
                fprintf(stderr, "partial/failed write\n");
                exit(EXIT_FAILURE);
            }
        }

        encode_video(output_path, &pts_index);
//        video_encode_example("/sdcard/screen/video.h263", output_path, AV_CODEC_ID_H263P, 1280, 720, &pts_index);
//        pthread_mutex_unlock(&mtx);

        if (close(fd) == -1) {
             fprintf(stderr, "file %s, line %d, close error\n", __FILE__, __LINE__);
             exit(EXIT_FAILURE);
        }

        if (numRead == -1) {
            fprintf(stderr, "file %s, line %d, read error\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }

        if (close(cfd) == -1) {
            fprintf(stderr, "file %s, line %d, close error\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    return((void *)0);
}
