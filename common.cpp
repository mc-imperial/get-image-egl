
#include "common.h"

#include <iostream>

bool init_gl(
    const int width,
    const int height,
    EGLDisplay& display,
    EGLConfig& config,
    EGLContext& context,
    EGLSurface& surface
  ) {

  const EGLint config_attribute_list[] =
      {
          //EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_RED_SIZE, 4,
          EGL_GREEN_SIZE, 4,
          EGL_BLUE_SIZE, 4,
          EGL_ALPHA_SIZE, 4,

          EGL_CONFORMANT, EGL_OPENGL_ES3_BIT,
          EGL_DEPTH_SIZE, 16,
          EGL_NONE
      };

  const EGLint context_attrib_list[] =
      {
          EGL_CONTEXT_CLIENT_VERSION, 3,
          EGL_NONE
      };

  const EGLint pbuffer_attrib_list[] =
      {
          EGL_WIDTH, width,
          EGL_HEIGHT, height,
          EGL_TEXTURE_FORMAT,  EGL_NO_TEXTURE,
          EGL_TEXTURE_TARGET, EGL_NO_TEXTURE,
          EGL_LARGEST_PBUFFER, EGL_TRUE,
          EGL_NONE
      };

  display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGLint major;
  EGLint minor;

  if(eglInitialize(display, &major, &minor) == EGL_FALSE) {
    std::cerr << "eglInitialize failed." << std::endl;
    return false;
  }

  EGLint num_config;
  if(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config) == EGL_FALSE) {
    std::cerr << "eglChooseConfig failed." << std::endl;
    return false;
  }

  if(num_config != 1) {
    std::cerr << "eglChooseConfig did not return 1 config." << std::endl;
    return false;
  }

  context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attrib_list);

  if(context == EGL_NO_CONTEXT) {
    std::cerr << "eglCreateContext failed: " << std::hex << eglGetError() << std::endl;
    return false;
  }

  surface = eglCreatePbufferSurface(display, config, pbuffer_attrib_list);

  if(surface == EGL_NO_SURFACE) {
    std::cerr << "eglCreatePbufferSurface failed: " << std::hex << eglGetError() << std::endl;
    return false;
  }

  eglMakeCurrent(display, surface, surface, context);

  return true;
}
