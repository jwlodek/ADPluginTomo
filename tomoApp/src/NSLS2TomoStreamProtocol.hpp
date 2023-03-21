#ifndef NSLS2_TOMO_STREAM_PROTOCOL_H
#define NSLS2_TOMO_STREAM_PROTOCOL_H

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define NSLS2_TOMO_STREAM_PROTOCOL_VERSION_MAJOR 0
#define NSLS2_TOMO_STREAM_PROTOCOL_VERSION_MINOR 0
#define NSLS2_TOMO_STREAM_PROTOCOL_VERSION_PATCH 1

typedef enum NSLS2TomoStreamProtocolFrameType {
    PROJECTION_FRAME = 0,
    DARK_FRAME = 1,
    BACKGROUND_FRAME = 2,
} NSLS2TomoStreamProtocolFrameType_t;


typedef enum NSLS2TomoStreamProtocolRefType {
    REF_TIMESTAMP = 0,
    REF_ANGLE = 1,
} NSLS2TomoStreamProtocolRefType_t;


typedef struct NSLS2TomoStreamProtocolHeader {

    uint8_t protocol_version_major = NSLS2_TOMO_STREAM_PROTOCOL_VERSION_MAJOR;
    uint8_t protocol_version_minor = NSLS2_TOMO_STREAM_PROTOCOL_VERSION_MINOR;
    uint8_t protocol_version_patch = NSLS2_TOMO_STREAM_PROTOCOL_VERSION_PATCH;
    uint8_t frame_type;
    uint8_t reference_type;
    uint8_t dataType;
    double reference;
    size_t num_bytes;
    size_t x_size;
    size_t y_size;
    size_t color_channels;

} NSLS2TomoStreamProtocolHeader_t;


#endif
