#include "common.h"

#define GL_GLEXT_PROTOTYPES

#include "GLES/gl.h"
#include "GLES2/gl2.h"

#include <cstdlib>		// EXIT_SUCCESS, etc
#include <cstdint>		// uint8_t, etc
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "lodepng.h"

#define COMPILE_ERROR_EXIT_CODE (101)
#define LINK_ERROR_EXIT_CODE (102)
#define RENDER_ERROR_EXIT_CODE (103)

#define CHANNELS (4)
#define DELAY (2)

const float vertices[] = {
  -1.0f,  1.0f,
  -1.0f, -1.0f,
   1.0f, -1.0f,
   1.0f,  1.0f
};

const GLubyte indices[] = {
  0, 1, 2,
  2, 3, 0
};

const char* vertex_shader_wo_version =
"attribute vec2 vert2d;\n"
"void main(void) {\n"
"  gl_Position = vec4(vert2d, 0.0, 1.0);\n"
"}";


bool readFile(const std::string& fileName, std::string& contentsOut) {
  std::ifstream ifs(fileName.c_str());
  if(!ifs) {
    std::cerr << "File " << fileName << " not found" << std::endl;
    return false;
  }
  std::stringstream ss;
  ss << ifs.rdbuf();
  contentsOut = ss.str();
  return true;
}

void printProgramError(GLuint program) {
  GLint length = 0;
  glGetShaderiv(program, GL_INFO_LOG_LENGTH, &length);

  // The maxLength includes the NULL character

  std::vector<GLchar> errorLog((size_t) length, 0);

  glGetShaderInfoLog(program, length, &length, &errorLog[0]);
  if(length > 0) {
    std::string s(&errorLog[0]);
    std::cout << s << std::endl;
  }
}

int checkForGLError(const char loc[]) {
  GLenum res = glGetError();
  if(res != GL_NO_ERROR) {
    std::cerr << loc << ": glGetError: " << std::hex << res << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

#define CHECK_ERROR(loc) \
do { \
  if(checkForGLError(loc) == EXIT_FAILURE) { \
    return EXIT_FAILURE; \
  } \
} while(false)

int render(
    EGLDisplay display,
    EGLSurface surface,
    int width,
    int height,
    bool animate,
    int numFrames,
    bool& saved,
    const std::string& output,
    GLint resolutionLocation,
    GLint timeLocation) {

  glViewport(0, 0, width, height);
  CHECK_ERROR("After glViewport");

  if(resolutionLocation != -1) {
    glUniform2f(resolutionLocation, width, height);
    CHECK_ERROR("After glUniform2f");
  }

  if(animate && timeLocation != -1 && numFrames > DELAY) {
    glUniform1f(timeLocation, numFrames / 10.0f);
    CHECK_ERROR("After glUniform1f");
  }

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  CHECK_ERROR("After glClearColor");
  glClear(GL_COLOR_BUFFER_BIT);
  CHECK_ERROR("After glClear");

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
  CHECK_ERROR("After glDrawElements");

  glFlush();
  CHECK_ERROR("After glFlush");

  eglSwapBuffers(display, surface);
  CHECK_ERROR("After swapBuffers");

  return EXIT_SUCCESS;
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

  bool persist = false;
  bool animate = false;
  bool exit_compile = false;
  bool exit_linking = false;
  std::string output("output.png");
  std::string vertex_shader;
  std::string fragment_shader;

  for(int i = 1; i < argc; i++) {
    std::string curr_arg = std::string(argv[i]);
    if(!curr_arg.compare(0, 2, "--")) {
      if(curr_arg == "--persist") {
        persist = true;
        continue;
      }
      else if(curr_arg == "--animate") {
        animate = true;
        continue;
      }
      else if(curr_arg == "--exit_compile") {
        exit_compile = true;
        continue;
      }
      else if(curr_arg == "--exit_linking") {
        exit_linking = true;
        continue;
      }
      else if(curr_arg == "--output") {
        output = argv[++i];
        continue;
      }
      else if(curr_arg == "--vertex") {
        vertex_shader = argv[++i];
        continue;
      }
      std::cerr << "Unknown argument " << curr_arg << std::endl;
      continue;
    }
    if (fragment_shader.length() == 0) {
      fragment_shader = curr_arg;
    } else {
      std::cerr << "Ignoring extra argument " << curr_arg << std::endl;
    }
  }

  if(fragment_shader.length() == 0) {
    std::cerr << "Requires fragment shader argument!" << std::endl;
    return EXIT_FAILURE;
  }

  GLuint program = glCreateProgram();
  int compileOk = 0;
  const char* temp;

  std::string fragContents;
  if(!readFile(fragment_shader, fragContents)) {
    return EXIT_FAILURE;
  }

  temp = fragContents.c_str();
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &temp, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileOk);
  if (!compileOk) {
    std::cerr << "Error compiling fragment shader." << std::endl;
    printProgramError(program);
    return COMPILE_ERROR_EXIT_CODE;
  }
  std::cerr << "Fragment shader compiled successfully." << std::endl;
  if (exit_compile) {
    std::cout << "Exiting after fragment shader compilation." << std::endl;
    return EXIT_SUCCESS;
  }
  glAttachShader(program, fragmentShader);

  std::string vertexContents;
  if(vertex_shader.length() == 0) {
    // Use embedded vertex shader.
    std::stringstream ss;
    size_t i = fragContents.find('\n');
    if(i != std::string::npos && fragContents[0] == '#') {
      ss << fragContents.substr(0,i);
      ss << "\n";
    } else {
      std::cerr << "Warning: Could not find #version string of fragment shader." << std::endl;
    }
    ss << vertex_shader_wo_version;
    vertexContents = ss.str();
  } else {
    if(!readFile(vertex_shader, vertexContents)) {
      return EXIT_FAILURE;
    }
  }

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  temp = vertexContents.c_str();
  glShaderSource(vertexShader, 1, &temp, NULL);
  glCompileShader(vertexShader);
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileOk);
  if (!compileOk) {
    std::cerr << "Error compiling vertex shader." << std::endl;
    printProgramError(program);
    return EXIT_FAILURE;
  }
  std::cerr << "Vertex shader compiled successfully." << std::endl;

  glAttachShader(program, vertexShader);

  std::cerr << "Linking program." << std::endl;
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &compileOk);
  if (!compileOk) {
    std::cerr << "Error in linking program." << std::endl;
    printProgramError(program);
    return LINK_ERROR_EXIT_CODE;
  }
  std::cerr << "Program linked successfully." << std::endl;
  if (exit_linking) {
    std::cout << "Exiting after program linking." << std::endl;
    return EXIT_SUCCESS;
  }

  GLint posAttribLocationAttempt = glGetAttribLocation(program, "vert2d");
  if(posAttribLocationAttempt == -1) {
    std::cerr << "Error getting vert2d attribute location." << std::endl;
    return EXIT_FAILURE;
  }
  GLuint posAttribLocation = (GLuint) posAttribLocationAttempt;
  glEnableVertexAttribArray(posAttribLocation);

  glUseProgram(program);

  GLuint vertexBuffer;
  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLuint indicesBuffer;
  glGenBuffers(1, &indicesBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  GLint injectionSwitchLocation = glGetUniformLocation(program, "injectionSwitch");
  GLint timeLocation = glGetUniformLocation(program, "time");
  GLint mouseLocation = glGetUniformLocation(program, "mouse");
  GLint resolutionLocation = glGetUniformLocation(program, "resolution");

  if(injectionSwitchLocation != -1) {
    glUniform2f(injectionSwitchLocation, 0.0f, 1.0f);
  }
  if(mouseLocation != -1) {
    glUniform2f(mouseLocation, 0.0f, 0.0f);
  }
  if(timeLocation != -1) {
    glUniform1f(timeLocation, 0.0f);
  }

  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glVertexAttribPointer(posAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);

  int numFrames = 0;
  bool saved = false;


  int result = render(
      display,
      surface,
      width,
      height,
      animate,
      numFrames,
      saved,
      output,
      resolutionLocation,
      timeLocation);

  if(result != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

//  ++numFrames;

//  if(numFrames == DELAY && !saved) {
  std::cerr << "Capturing frame." << std::endl;
  saved = true;
  unsigned uwidth = (unsigned int) width;
  unsigned uheight = (unsigned int) height;
  std::vector<std::uint8_t> data(uwidth * uheight * CHANNELS);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
  CHECK_ERROR("After glReadPixels");
  std::vector<std::uint8_t> flipped_data(uwidth * uheight * CHANNELS);
  for (unsigned int h = 0; h < uheight ; h++)
    for (unsigned int col = 0; col < uwidth * CHANNELS; col++)
      flipped_data[h * uwidth * CHANNELS + col] =
          data[(uheight - h - 1) * uwidth * CHANNELS + col];
  unsigned png_error = lodepng::encode(output, flipped_data, uwidth, uheight);
  if (png_error) {
    std::cerr << "Error producing PNG file: " << lodepng_error_text(png_error) << std::endl;
    return EXIT_FAILURE;
  }
  if (!persist) {
    return EXIT_SUCCESS;
  }
//  }

  return EXIT_SUCCESS;
}

