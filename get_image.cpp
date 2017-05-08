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
#include "json.hpp"
using json = nlohmann::json;

static const int WIDTH = 256;
static const int HEIGHT = 256;

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

void printShaderError(GLuint shader) {
  GLint length = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

  // The maxLength includes the NULL character

  std::vector<GLchar> errorLog((size_t) length, 0);

  glGetShaderInfoLog(shader, length, &length, &errorLog[0]);
  if(length > 0) {
    std::string s(&errorLog[0]);
    std::cout << s << std::endl;
  }
}

void printProgramError(GLuint program) {
  GLint length = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

  // The maxLength includes the NULL character

  std::vector<GLchar> errorLog((size_t) length, 0);

  glGetProgramInfoLog(program, length, &length, &errorLog[0]);
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

/*---------------------------------------------------------------------------*/
// Initialisation of uniforms

template<typename T>
T *getArray(const json& j) {
  T *a = new T[j.size()];
  for (int i = 0; i < j.size(); i++) {
    a[i] = j[i];
  }
  return a;
}

#define GLUNIFORM_ARRAYINIT(funcname, uniformloc, gltype, jsonarray) \
  gltype *a = getArray<gltype>(jsonarray); \
  funcname(uniformloc, jsonarray.size(), a); \
  delete [] a

void setJSONDefaultEntries(json& j) {

  if (j.count("injectionSwitch") == 0) {
    std::cerr << "Warning: uniform injectionSwitch not found in JSON, using default value" << std::endl;
    j["injectionSwitch"] = {
      {"func", "glUniform2f"},
      { "args", { 0.0f, 1.0f }}
    };
  }

  if (j.count("time") == 0) {
    std::cerr << "Warning: uniform time not found in JSON, using default value" << std::endl;
    j["time"] = {
      {"func", "glUniform1f"},
      { "args", { 0.0f }}
    };
  }


  if (j.count("mouse") == 0) {
    std::cerr << "Warning: uniform mouse not found in JSON, using default value" << std::endl;
    j["mouse"] = {
      {"func", "glUniform2f"},
      { "args", { 0.0f, 0.0f }}
    };
  }

  if (j.count("resolution") == 0) {
    std::cerr << "Warning: uniform resolution not found in JSON, using default value" << std::endl;
    j["resolution"] = {
      {"func", "glUniform2f"},
      { "args", { float(WIDTH), float(HEIGHT) }}
    };
  }

}

int setUniforms(const GLuint& program, const std::string& fragment_shader) {
  GLint nbUniforms;
  glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &nbUniforms);
  CHECK_ERROR("glGetProgramiv");
  if (nbUniforms == 0) {
    return EXIT_SUCCESS;
  }

  GLint uniformNameMaxLength = 0;
  glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameMaxLength);
  CHECK_ERROR("glGetProgramiv");
  GLchar *uniformName = new GLchar[uniformNameMaxLength];
  GLint uniformSize;
  GLenum uniformType;

  std::string jsonFilename(fragment_shader);
  jsonFilename.replace(jsonFilename.end()-4, jsonFilename.end(), "json");
  std::string jsonContent;
  if (!readFile(jsonFilename, jsonContent)) {
    return EXIT_FAILURE;
  }
  json j = json::parse(jsonContent);

  setJSONDefaultEntries(j);

  for (int i = 0; i < nbUniforms; i++) {
    glGetActiveUniform(program, i, uniformNameMaxLength, NULL, &uniformSize, &uniformType, uniformName);
    CHECK_ERROR("glGetActiveUniform");
    std::cout << "UNIFORM " << i << ": " << uniformName << " size:" << uniformSize << std::endl;

    if (j.count(uniformName) == 0) {
      std::cerr << "Error: missing JSON entry for uniform: " << uniformName << std::endl;
      return EXIT_FAILURE;
    }
    if (j.count(uniformName) > 1) {
      std::cerr << "Error: more than one JSON entry for uniform: " << uniformName << std::endl;
      return EXIT_FAILURE;
    }
    json uniformInfo = j[uniformName];

    // Check presence of func and args entries
    if (uniformInfo.find("func") == uniformInfo.end()) {
      std::cerr << "Error: malformed JSON: no \"func\" entry for uniform: " << uniformName << std::endl;
      return EXIT_FAILURE;
    }
    if (uniformInfo.find("args") == uniformInfo.end()) {
      std::cerr << "Error: malformed JSON: no \"args\" entry for uniform: " << uniformName << std::endl;
      return EXIT_FAILURE;
    }

    // Get uniform location
    GLint uniformLocation = glGetUniformLocation(program, uniformName);
    CHECK_ERROR("After glGetUniformLocation");
    if (uniformLocation == -1) {
      std::cerr << "Error: Cannot find uniform named: " << uniformName << std::endl;
      return EXIT_FAILURE;
    }

    // Dispatch to matching init function
    std::string uniformFunc = uniformInfo["func"];
    json args = uniformInfo["args"];

    // TODO: check that args has the good number of fields and type

    if (uniformFunc == "glUniform1f") {
      glUniform1f(uniformLocation, args[0]);
    } else if (uniformFunc == "glUniform2f") {
      glUniform2f(uniformLocation, args[0], args[1]);
    } else if (uniformFunc == "glUniform3f") {
      glUniform3f(uniformLocation, args[0], args[1], args[2]);
    } else if (uniformFunc == "glUniform4f") {
      glUniform4f(uniformLocation, args[0], args[1], args[2], args[3]);
    }

    else if (uniformFunc == "glUniform1i") {
      glUniform1i(uniformLocation, args[0]);
    } else if (uniformFunc == "glUniform2i") {
      glUniform2i(uniformLocation, args[0], args[1]);
    } else if (uniformFunc == "glUniform3i") {
      glUniform3i(uniformLocation, args[0], args[1], args[2]);
    } else if (uniformFunc == "glUniform4i") {
      glUniform4i(uniformLocation, args[0], args[1], args[2], args[3]);
    }

    // Note: no "glUniformXui" variant in OpenGL ES

    // else if (uniformFunc == "glUniform1ui") {
    //   glUniform1ui(uniformLocation, args[0]);
    // } else if (uniformFunc == "glUniform2ui") {
    //   glUniform2ui(uniformLocation, args[0], args[1]);
    // } else if (uniformFunc == "glUniform3ui") {
    //   glUniform3ui(uniformLocation, args[0], args[1], args[2]);
    // } else if (uniformFunc == "glUniform4ui") {
    //   glUniform4ui(uniformLocation, args[0], args[1], args[2], args[3]);
    // }

    else if (uniformFunc == "glUniform1fv") {
      GLUNIFORM_ARRAYINIT(glUniform1fv, uniformLocation, GLfloat, args);
    } else if (uniformFunc == "glUniform2fv") {
      GLUNIFORM_ARRAYINIT(glUniform2fv, uniformLocation, GLfloat, args);
    } else if (uniformFunc == "glUniform3fv") {
      GLUNIFORM_ARRAYINIT(glUniform3fv, uniformLocation, GLfloat, args);
    } else if (uniformFunc == "glUniform4fv") {
      GLUNIFORM_ARRAYINIT(glUniform4fv, uniformLocation, GLfloat, args);
    }

    else if (uniformFunc == "glUniform1iv") {
      GLUNIFORM_ARRAYINIT(glUniform1iv, uniformLocation, GLint, args);
    } else if (uniformFunc == "glUniform2iv") {
      GLUNIFORM_ARRAYINIT(glUniform2iv, uniformLocation, GLint, args);
    } else if (uniformFunc == "glUniform3iv") {
      GLUNIFORM_ARRAYINIT(glUniform3iv, uniformLocation, GLint, args);
    } else if (uniformFunc == "glUniform4iv") {
      GLUNIFORM_ARRAYINIT(glUniform4iv, uniformLocation, GLint, args);
    }

    else {
      std::cerr << "Error: unknown/unsupported uniform init func: " << uniformFunc << std::endl;
      return EXIT_FAILURE;
    }
    CHECK_ERROR("After uniform initialisation");
  }

  delete [] uniformName;

  return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char* argv[]) {

  EGLDisplay display = 0;
  EGLConfig config = 0;
  EGLContext context = 0;
  EGLSurface surface = 0;

  bool res = init_gl(
      WIDTH,
      HEIGHT,
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
    printShaderError(fragmentShader);
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
    printShaderError(vertexShader);
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

  int result = setUniforms(program, fragment_shader);
  if(result != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  std::cerr << "Uniforms set successfully." << std::endl;

  int numFrames = 0;
  bool saved = false;

  result = render(
      display,
      surface,
      WIDTH,
      HEIGHT,
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
  unsigned uwidth = (unsigned int) WIDTH;
  unsigned uheight = (unsigned int) HEIGHT;
  std::vector<std::uint8_t> data(uwidth * uheight * CHANNELS);
  glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
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
