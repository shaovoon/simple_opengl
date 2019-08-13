# Simple Opengl
Simple OpenGL Project for Emscripten bug report

## The bug

The OpenGL 2.0 project showing a rotating quad is made to reproduce the problem. It works perfectly on VC++ and Windows. To run the exe on windows, copy the SDL2.dll to exe folder.

There is a problem with the latest Emscripten calling the render function. The 1st OpenGL call in the render function has this "GLctx is undefined" error in javascript on Chrome and Firefox. I am not multi-threading.

```Cpp
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(render, 0, 0);
#else
...

void render()
{
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT); <--- exception on this function.
...
}
```

To build this project in Emscripten, use the Makefile. The include folder location of glm has to be amended in the Makefile to your location.

```
make all
```