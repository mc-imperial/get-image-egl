#include "common.h"

#define GL_GLEXT_PROTOTYPES

#include "GLES/gl.h"
#include "GLES2/gl2.h"
#include "GL/glcorearb.h"

#include <iostream>

GLint getInt(GLenum name) {
  GLint temp = 0;
  glGetIntegerv(name, &temp);
  return temp;
}

template<class KT, class VT> void output(KT key, VT val) {
  std::cout << "    \"" << key << "\": \"" << val << "\",\n";
}

void go() {

  std::cout << "{\n";

  output("GL_VERSION", glGetString(GL_VERSION));
  output("GL_MAJOR_VERSION", getInt(GL_MAJOR_VERSION));
  output("GL_MINOR_VERSION", getInt(GL_MINOR_VERSION));
  output("GL_SHADING_LANGUAGE_VERSION", glGetString(GL_SHADING_LANGUAGE_VERSION));
  output("GL_VENDOR", glGetString(GL_VENDOR));
  output("GL_RENDERER", glGetString(GL_RENDERER));

  GLuint glslNumVersions = (GLuint) getInt(GL_NUM_SHADING_LANGUAGE_VERSIONS);

  std::cout << "    \"Supported_GLSL_versions\": [";
  if(glslNumVersions > 0) {
    std::cout << "\"" << glGetStringi(GL_SHADING_LANGUAGE_VERSION, 0) << "\"";
    for(GLuint i=1; i < glslNumVersions; ++i) {
      std::cout << ", \"" << glGetStringi(GL_SHADING_LANGUAGE_VERSION, i) << "\"";
    }
  }
  std::cout << "]\n";

  std::cout << "}" << std::endl;
}

int main(int argc, char* argv[]) {

  EGLDisplay display = 0;
  EGLConfig config = 0;
  EGLContext context = 0;
  EGLSurface surface = 0;

  const int width = 640;
  const int height = 480;

  bool res = init_gl(
      width,
      height,
      display,
      config,
      context,
      surface
  );

  if(!res) {
    return EXIT_FAILURE;
  }

  go();

  return EXIT_SUCCESS;
}
