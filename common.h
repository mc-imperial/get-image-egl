#ifndef CPP_COMMON_H
#define CPP_COMMON_H

#include "EGL/egl.h"

bool init_gl(
    const int width,
    const int height,
    EGLDisplay& display,
    EGLConfig& config,
    EGLContext& context,
    EGLSurface& surface
);

#endif //CPP_COMMON_H
