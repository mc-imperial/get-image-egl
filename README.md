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

## Usage

`./get_image <PATH_TO_FRAGMENT_SHADER>`

Useful flags:
* `--persist` - causes the shader to be rendered until the window is closed
* `--output <OUTPUT_FILE>` - a png file will be produced at the given location with the contents of the rendered shader (default is `output.png`)
* `--vertex <PATH_TO_VERTEX_SHADER>` - provide a custom vertex shader file rather than using the default (provided in `get_image.cpp`).

