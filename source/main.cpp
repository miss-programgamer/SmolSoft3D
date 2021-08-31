#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include "sdl_extra.hpp"
#include "math.hpp"
#include "renderer.hpp"


int main(int, char**)
{
    // init sdl
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    
    // create window and renderer
    int window_scale = 3;
    SDL_Window* window = SDL_CreateWindow("SmolSoft3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_scale * 400, window_scale * 240, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    
    // prevent window from appearing white for a split second at startup
    SDL_RenderClear(renderer);
    SDL_ShowWindow(window);
    
    // create surface and its texture
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 400, 240, 32, SDL_PIXELFORMAT_BGRA32);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    // load images to sample
    SDL_Surface* goober = IMG_Load("./assets/goober.png");
    SDL_Surface* crate = IMG_Load("./assets/crate.png");
    
    // rendering structs
    Renderer3D renderer3d;
    Target target = surface;
    Camera3D camera{ glm::vec3(3.5f, 1.5f, -2.0f), 45.0f, -20.0f };
    Screen screen{ (float)surface->w, (float)surface->h, 60.0f };
    
    // for calculating delta time
    Uint64 time_now = SDL_GetPerformanceCounter();
    Uint64 time_prev = time_now;
    
    // global variables
    float sensitivity = 0.2f;
    
    // game state
    float spike_x = 0.0f;
    
    // load models
    Model3D floor_model;
    LoadModel("./assets/floor.txt", floor_model);
    
    Model3D triangle_model;
    LoadModel("./assets/triangle.txt", triangle_model);
    
    Model3D spike_model;
    LoadModel("./assets/spike.txt", spike_model);
    
    Model3D crate_model;
    LoadModel("./assets/crate.txt", crate_model);
    
    // main loop
    for (bool running = true; running;)
    {
        // process events
        for (SDL_Event event{}; running && SDL_PollEvent(&event);)
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    running = false;
                    break;
                
                case SDL_MOUSEMOTION:
                    if (SDL_GetRelativeMouseMode())
                    {
                        camera.Turn(
                            -sensitivity * (float)event.motion.xrel,
                            -sensitivity * (float)event.motion.yrel
                        );
                    }
                    break;
                
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        SDL_ShowCursor(SDL_FALSE);
                    }
                    break;
                
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        SDL_ShowCursor(SDL_TRUE);
                    }
                    break;
            }
        }
        
        // calculate delta time
        time_prev = time_now;
        time_now = SDL_GetPerformanceCounter();
        float time_delta = float((time_now - time_prev) / double(SDL_GetPerformanceFrequency()));
        
        // get key states
        auto keys = SDL_GetKeyboardState(nullptr);
        bool up     = keys[SDL_GetScancodeFromKey(SDLK_w)];
        bool down   = keys[SDL_GetScancodeFromKey(SDLK_s)];
        bool left   = keys[SDL_GetScancodeFromKey(SDLK_a)];
        bool right  = keys[SDL_GetScancodeFromKey(SDLK_d)];
        
        // move camera
        float advance = (up ? 1.0f : 0.0f) - (down ? 1.0f : 0.0f);
        float strafe = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
        
        float move_speed = 2.2f;
        float move_factor = time_delta * move_speed;
        
        camera.Move(move_factor * advance, move_factor * strafe, 0.0f);
        
        // clear target
        target.ClearSurface({ 0, 0, 0, 255 });
        target.ClearDepth();
        
        // draw floor with a texture
        renderer3d.SetSampler(goober);
        renderer3d.Blit3DModel(target, camera, screen, floor_model);
        
        // draw crate
        renderer3d.SetSampler(crate);
        renderer3d.Blit3DModel(target, camera, screen, crate_model);
        
        // draw spike and colored triangle
        renderer3d.SetSampler(nullptr);
        renderer3d.Blit3DModel(target, camera, screen, triangle_model);
        
        auto transform = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 2.0f));
        renderer3d.Blit3DModel(target, camera, screen, spike_model, transform);
        
        // present our finished drawing to the window
        SDL_UpdateTexture(texture, nullptr, surface->pixels, surface->pitch);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
    
    // quickly hide window to be more responsive
    SDL_HideWindow(window);
    
    // quit sdl
    IMG_Quit();
    SDL_Quit();
    
    // nothing ever goes wrong :D
    return 0;
}
