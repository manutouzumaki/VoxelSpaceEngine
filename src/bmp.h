#ifndef _BMP_H_
#define _BMP_H_

#include "defines.h"

struct Bmp {
    void *data;
    u32 width;
    u32 height;
};

Bmp BmpLoad(char *path);
void BmpDestroy(Bmp *bitmap);



#endif
