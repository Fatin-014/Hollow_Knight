#ifndef PLATFORM_H
#define PLATFORM_H
#include "raylib.h"
#include "raymath.h"
#include "constants.h"

typedef struct Platform {
    Rectangle rect;
} Platform;

static inline Platform InitPlatform(void) {
    Platform pf;
    pf.rect = (Rectangle){ PLATFORM_X, PLATFORM_Y, PLATFORM_WIDTH, PLATFORM_HEIGHT };
    return pf;
}

#endif