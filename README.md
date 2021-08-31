# SmolSoft3D

![screenshot](./screenshot.png "Screenshot of SmolSoft3D in action")

This is the repository of SmolSoft3D, a small software 3D rendering project initially created in about a weekend by [Mireille (@ProgramGamer)](https://twitter.com/ProgramGamer). It is not meant to be a useful renderer for making commercial games and the likes, and was more or less intended as a fun distraction, and also to prove to myself that I could figure out how to do it in two days!

## Dependencies & Building

In order to compile, this project needs to link to three libraries:

- SDL2
- SDL2 image
- GLM

Additionally, this project uses CMake as a build system by default, though with the small code size it *should* be relatively simple to adapt it to your preferred build system.

## Code Structure

This codebase is organised into four source files contained in the [source](./source) folder. [math.hpp](./source/math.hpp) contains a few utility functions for linear interpolation, color blending, and the like. [sdl_extra.hpp](./source/sdl_extra.hpp) has a few functions for reading/writing pixel date to and from an `SDL_Surface`. Finally, the crux of this repository, [renderer.hpp](./source/renderer.hpp) contains everything directly related to rendering 3D polygons, such as structs for vertices, triangles, and models, and a big `Renderer3D` class that contains the bulk of the rendering logic. Also, there is a [main.cpp](./source/main.cpp), but you can probably guess what that is for if you've programmed in C/C++ before :P.

Additionally, the [assets](./assets) folder contains a few textures and models that the engine loads and renders by default.

## 3D Model Format

*Example of a colored, textured triangle in the format used by the engine:*

``` txt
1 3 pos color uv

 0.0  1.0 0.0   255 0 0 255   0 1
-1.0 -1.0 0.0   0 255 0 255   0 1
 1.0 -1.0 0.0   0 0 255 255   0 1
```

The text files inside the [assets](./assets) folder contain data for 3D models. In case you would want to modify these files and/or add new ones, the format is as follows:

The first number read from the file is the number of **triangles** (not the number of vertices) and is used to reserve memory in advance. The engine will also not read more triangles than this, so be careful to increase this number if you manually add more polygons to a model!

The second number is the number of attributes per vertex. Based on this number, the engine will then read that many strings and use them as the vertex format. Possible values for vertex attributes are `pos` for the 3D position of the vertex, `color` for the RGBA vertex color, and `uv` for texture coordinates. This is useful for models that do not use a certain attribute, such as an "OpenGL triangle" that doesn't need to be textured, in which case the format should be `2 pos color`, or a model that only uses textures with a format like `2 pos uv`. Examples of all useful formats are present in the [assets](./assets) folder.

Finally, based on the format provided previously, the engine will proceed to read groups of three vertices.

## Renderer3D API

### Rendering Setup

Before rendering anything, some setup is required:

1. First, a `Renderer3D` instance should be created. Its constructor does not require any arguments.
2. Then, a `Target` instance should be created with a previously created `SDL_Surface`. This is necessary so that the renderer has access to a depth buffer.
3. Then, a `Camera3D` instance will represent the position/rotation of a camera in 3D space. It contains a `glm::vec3` for its **position**, and two floats for its **pitch** and **yaw**. This could be achieved with a transformation matrix, but I think this data structure is more intuitive.
4. Finally, a `Screen` instance should be initialized with the size of our `Target`'s surface, as well as the desired FOV in degrees.

*Here is an example of what this setup would look like:*

``` cpp
auto format = SDL_PIXELFORMAT_BGRA32;
SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 400, 240, 32, format);
// ...
Renderer3D renderer3d;
Target target = surface;
Camera3D camera{ glm::vec3(0.0f, 1.5f, 0.0f), 45.0f, -20.0f };
Screen screen{ (float)surface->w, (float)surface->h, 60.0f };
```

### Loading Models

Then, before we can render anything, we need to load some models. To do so, we can use the `LoadModel` function.

`LoadModel`, takes a `std::string` and a `Model3D` reference. If the function succeeds, the given model reference will contain the loaded model and return true, otherwise it returns false.

``` cpp
Model3D floor_model;
LoadModel("./assets/floor.txt", floor_model);
```

### Drawing, aka Blitting

Finally, once we've setup our rendering classes and loaded our models, we can start drawing stuff!

The most important method you should be aware of is `Renderer3D::Blit3DModel`, which takes a `Target`, a `Camera3D`, a `Screen`, a `Model3D`, and an optional `glm::mat4`. This will blit the given 3D model to the surface contained in the given target, using the camera to translate its vertices, the screen to project it into screen space, and the transform to draw it at a specific position/rotation/scale.

You should also be aware of `Renderer3D::SetSampler`, which takes an `SDL_Surface*` which will be used to sample texture data. This value can be `nullptr`, at which point the renderer will simply draw untextured polygons.

``` cpp
// goober is a previously loaded SDL_Surface
renderer3d.SetSampler(goober);
renderer3d.Blit3DModel(target, camera, screen, floor_model);
```

``` cpp
// draws the given model, translated by the given offset
renderer3d.SetSampler(nullptr);
auto transform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 2.0f));
renderer3d.Blit3DModel(target, camera, screen, spike_model, transform);
```

### Now What?

Now that we've rendered our scene, we still need to present it to the screen. However, since SmolSoft3D is a software renderer, it isn't really within the scope of this README to explain how to do this. However, the main function provided in this repo does contain code that does this, so reading it will give you an idea of how you can achieve it yourself.

Also, note that saving screenshots is trivial since the result of rendering a scene is an `SDL_Surface`! However, it is left as an exercise to the reader to implement this functionality. You know, to leave some of the fun to you :P

## Shortcomings

Unlike many other open source projects, I've opted to also talk about the various shortcomings of this project for the sake of transparency, namely:

### There's gaps between polygons sometimes!

Yep. The drawing algorythm I'm using can, in theory, draw polygons that don't do this, but my implementation probably rounds some numbers incorrectly, and so sometimes there are black lines between/inside polygons. I tried to mitigate the issue, but I'll have to keep poking at it to truly fix it.

### Textures look kinda jagged?

Again, probably a rounding error. Fixing the issue with gaps between polygons would probably also fix this, so again, I just have to keep poking at the problem.

### If I increase the resolution, it slows down a lot...

Yep! this isn't a very fast renderer, and I prioritized readability over performance. But hey, keeping the resolution low does give it a pretty retro feel, don't you think?

Also, running this engine in debug mode is probably not a good idea >_>

### Why is the clipping phase only for the near plane?

So, if you're familiar with 3D rendering, you'll know that there are normally four phases of spatial transformations in the rendering pipeline. Namely, going from **local space** to **world space**, from **world space** to **view space**, from **view space** to **clip space**, and finally rasterizing that onto the screen. [This section](https://learnopengl.com/Getting-started/Coordinate-Systems) of the Learn OpenGL tutorials explains this in more detail, and is encouraged reading material for any 3D rendering newbie!

In any case, this engine kind of fenagles its way around the transition to clip space by only clipping triangles that intersect with the near plane, and opts to do clipping on the x and y axis during rasterization. This happens to be more convenient in this implementation, but it does mean that the clipping phase happens in two places, sorry ^^'

## In Closing

Thank you for reading this to the end! Hopefully this repository will provide you with something fun to tinker with, or an opportunity to learn the logic of 3D rendering in more detail! And if you have questions, feell free to open a GitHub issue and I'll answer to the best of my ability!

Peace :v: