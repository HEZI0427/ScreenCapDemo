#ifndef __SCREENCAP_H__
#define __SCREENCAP_H__

extern "C" {

enum image_type {
    ID_BMP,
    ID_JPEG,
};

void GetPixels(void *image_map, enum image_type if_type, int width, int height);
}
#endif
