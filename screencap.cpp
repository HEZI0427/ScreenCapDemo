/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>


#include <binder/ProcessState.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>

#include <ui/PixelFormat.h>

#include <SkImageEncoder.h>
#include <SkBitmap.h>
#include <SkData.h>
#include <SkStream.h>

#include "screencap.h"
extern "C" {
#include "bmp.h"
}

using namespace android;

static uint32_t DEFAULT_DISPLAY_ID = ISurfaceComposer::eDisplayIdMain;

//static SkBitmap::Config flinger2skia(PixelFormat f)
static SkBitmap::Config flinger2skia(uint32_t f)
{
    switch (f) {
        case PIXEL_FORMAT_A_8:
            return SkBitmap::kA8_Config;
        case PIXEL_FORMAT_RGB_565:
            return SkBitmap::kRGB_565_Config;
        case PIXEL_FORMAT_RGBA_4444:
            return SkBitmap::kARGB_4444_Config;
        default:
            return SkBitmap::kARGB_8888_Config;
    }
}

void GetPixels(void *image_map, enum image_type if_type, int width, int height)
{
    ProcessState::self()->startThreadPool();

    int32_t displayId = DEFAULT_DISPLAY_ID;

    void const* base = NULL;
    uint32_t w, s, h, f;
    size_t size = 0;

    ScreenshotClient screenshot;
    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(displayId);

    if (display != NULL && screenshot.update(display, width, height) == NO_ERROR) {
        base = screenshot.getPixels();
        size = screenshot.getSize();
        w = screenshot.getWidth();
        h = screenshot.getHeight();
        s = screenshot.getStride();
        f = screenshot.getFormat();
    }

    if (base) {
        switch (if_type) {
        case ID_BMP:
            {
            //just generate the BMP image by screencap
            GenBmpFile((uint8_t *)base, 32, width, height, (uint8_t *) image_map);
            }
            break;

        case ID_JPEG:
            {

            //just generate the JPEG image by screencap
            SkBitmap b;
            SkDynamicMemoryWStream stream;

            b.setConfig(flinger2skia(f), w, h, s*bytesPerPixel(f));
            b.setPixels((void*)base);
            SkImageEncoder::EncodeStream(&stream, b,
                    SkImageEncoder::kJPEG_Type, SkImageEncoder::kDefaultQuality);

            SkData* streamData = stream.copyToData();
            memcpy((void *)image_map, streamData->data(), streamData->size());
            streamData->unref();
            }
            break;
        default:
            fprintf(stderr, "file %s, line %d, image file type error\n", __FILE__, __LINE__);
            break;
        }
    }
}
