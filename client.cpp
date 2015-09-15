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
 * local_client read screen from surface and put it into
 * local socket.
 */
int local_client(int *sfd)
{
    struct sockaddr_un addr;
    char buf[BUF_SIZE];

    *sfd = socket(AF_UNIX, SOCK_STREAM, 0);      /* Create client socket */
    if (*sfd == -1) {
        fprintf(stderr, "file %s, func %s, line %d, socket error\n", __FILE__, __func__, __LINE__);
        exit(EXIT_FAILURE);
    }

    /* Construct server address, and make the connection */

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(*sfd, (struct sockaddr *) &addr,
                sizeof(struct sockaddr_un)) == -1) {
        fprintf(stderr, "file %s, func %s, line %d, connect error\n", __FILE__, __func__, __LINE__);
        exit(EXIT_FAILURE);
    }

    return 0;
}


