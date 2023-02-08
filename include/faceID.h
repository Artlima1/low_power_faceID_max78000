#ifndef __FACEID_H__
#define __FACEID_H__

#include <stdint.h>
#include "img_capture.h"

#define HEIGHT 160
#define WIDTH 120
#define THICKNESS 4
#define FRAME_COLOR 0x535A

#define BYTE_PER_PIXEL 2

#define IMG_SHIFT_ANALYSIS 4

#define FACEID_TRIES 20

// #define FAST_FIFO           // if defined, it uses fast fifo instead of fifo

typedef struct {
    int8_t decision;
    char * name;
} faceID_decision_t;

faceID_decision_t faceid_run(void);

#endif // __FACEID_H__