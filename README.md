# Get image

A tool for rendering a fragment shader to a PNG file.

This version of get image currently uses EGL and OpenGL ES 3.
Thus, `libEGL.so` and `libGLESv2.so` (yes, v2)
will be loaded at runtime on Linux (and similarly for other OS's).
You can use the `swiftshader` version of these libraries to
perform software rendering,
or the `ANGLE` version of these libraries to
perform rendering using ANGLE (using Direct3D on Windows and OpenGL on Linux).

Run `get_image` on the provided `simple.frag` shader to test that it works. E.g.

```bash
./get_image --persist simple.frag
```

Note:

You should also have a file with the same name as the shader but with a .json extension.
This file should only contain an "{}"

## Usage

`./get_image <PATH_TO_FRAGMENT_SHADER>`

Useful flags:
* `--persist` - causes the shader to be rendered until the window is closed
* `--output <OUTPUT_FILE>` - a png file will be produced at the given location with the contents of the rendered shader (default is `output.png`)
* `--vertex <PATH_TO_VERTEX_SHADER>` - provide a custom vertex shader file rather than using the default (provided in `get_image.cpp`).


## Building

Building the project uses CMake.

First you must ensure you have the depdencies. There are scripts in buildscripts
intended to run on the CI that can also be used to build locally. You can run
these as follows: 

On Windows:

```bash
bash buildscripts\1-install-deps-appveyor.sh
```

On Linux or OSX:

```bash
bash buildscripts/1-install-deps-travis.sh
```

Alternativly on Ubuntu you can:
```bash
sudo apt-get install libgles2-mesa-dev
```


(This will probably fail with an error at the last step, but all that matters is that the deps are created)

Once you have the deps downloaded, you can build with CMake as follows:

```bash
mkdir build
cmake ..
cmake --build .
```

Note that if you are building on Windows you must have Visual Studio installed and you must specify the
build generator explicitly:

```bash
cmake -G "Visual Studio 15 2017 Win64"  -T v140 ..
```

The reason for this is that if you do not specify Win64 it will default to 32-bit (which is incompatible
with the libraries in deps). The "-T v140" flag specifies that it should build with a VS2015 C++ compiler
(which is the same one the libraries are built with).

If this complains about being unable to install a compiler and you do have Visual Studio installed, then
you will need to install Visual C++ support for VS2015. You can obtain the build tools from
https://www.visualstudio.com/ by signing up for the Microsoft Developer Essentials (it's free).
Note that you don't have to install the entirety of VS2015 (which is much larger).

It *may* work correctly if you simply omit the "-T v140" flag, but we think that might be a bad idea
but are not entirely sure.

Note that you will see an awful lot of warnings. This is fine. As long as the build exits successfully,
try running the generated executable (in either Debug or Release in your build directory) on sample.frag
in the root of this project. If you get a pretty image called output.png then it has worked.
