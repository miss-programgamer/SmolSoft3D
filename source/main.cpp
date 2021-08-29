#include <utility>
#include <array>
#include <cmath>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

void SDL_Blit(SDL_Surface* surface, int x, int y, const SDL_Color& color)
{
    SDL_Rect rect{ x, y, 1, 1 };
    SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a));
}

void SDL_BlitBig(SDL_Surface* surface, int x, int y, const SDL_Color& color)
{
    SDL_Blit(surface, x - 1, y - 1, color);
    SDL_Blit(surface, x + 0, y - 1, color);
    SDL_Blit(surface, x + 1, y - 1, color);
    
    SDL_Blit(surface, x - 1, y + 0, color);
    SDL_Blit(surface, x + 0, y + 0, color);
    SDL_Blit(surface, x + 1, y + 0, color);
    
    SDL_Blit(surface, x - 1, y + 1, color);
    SDL_Blit(surface, x + 0, y + 1, color);
    SDL_Blit(surface, x + 1, y + 1, color);
}

SDL_Color SDL_ReadPixel(SDL_Surface* surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    Uint8* pixel_data = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    
    Uint32 pixel = 0;
    switch (bpp)
    {
        case 1:
            pixel = *pixel_data;
            break;
        
        case 2:
            pixel = *(Uint16*)pixel_data;
            break;
        
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            { pixel = (pixel_data[0] << 16) | (pixel_data[1] << 8) | pixel_data[2]; }
            else
            { pixel = pixel_data[0] | (pixel_data[1] << 8) | (pixel_data[2] << 16); }
            break;
        
        case 4:
            pixel = *(Uint32*)pixel_data;
            break;
    }
    
    SDL_Color color;
    SDL_GetRGBA(pixel, surface->format, &color.r, &color.g, &color.b, &color.a);
    return color;
}

struct Vertex2D
{
    glm::vec2 pos;
    SDL_Color color;
    glm::vec2 uv;
};

struct Triangle2D
{
    std::array<Vertex2D, 3> vertices;
};

struct Vertex3D
{
    glm::vec3 pos;
    SDL_Color color;
    glm::vec2 uv;
};

struct Triangle3D
{
    std::array<Vertex3D, 3> vertices;
};

struct Model3D
{
    std::vector<Triangle3D> triangles;
};

struct Camera3D
{
    glm::vec3 pos;
    float angle;
    float fov;
};

struct Screen
{
    float width;
    float height;
};

inline float Lerp(float a, float b, float p)
{
    return a + p * (b - a);
}

inline float InvLerp(float x, float a, float b)
{
    return (x - a) / (b - a);
}

inline float Remap(float x, float a, float b, float c, float d)
{
    return Lerp(c, d, InvLerp(x, a, b));
}

inline SDL_Color Lerp(const SDL_Color& a, const SDL_Color& b, float p)
{
    return SDL_Color
    {
        (Uint8)Lerp((float)a.r, (float)b.r, p),
        (Uint8)Lerp((float)a.g, (float)b.g, p),
        (Uint8)Lerp((float)a.b, (float)b.b, p),
        (Uint8)Lerp((float)a.a, (float)b.a, p),
    };
}

inline glm::vec2 Lerp(const glm::vec2& a, const glm::vec2& b, float p)
{
    return a + p * (b - a);
}

inline glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float p)
{
    return a + p * (b - a);
}

inline glm::vec4 Lerp(const glm::vec4& a, const glm::vec4& b, float p)
{
    return a + p * (b - a);
}

inline float Clamp(float x, float a, float b)
{
    return std::max(std::min(x, b), a);
}

inline glm::vec4 ToInterpSpace(const SDL_Color& color, float w)
{
    return glm::vec4
    {
        float(color.r) * w,
        float(color.g) * w,
        float(color.b) * w,
        float(color.a) * w,
    };
}

inline SDL_Color ColorFromInterpSpace(const glm::vec4& color, float w)
{
    return SDL_Color
    {
        Uint8(color.x / w),
        Uint8(color.y / w),
        Uint8(color.z / w),
        Uint8(color.w / w),
    };
}

SDL_Color Sample(SDL_Surface* surface, float u, float v)
{
    auto x = (int)(u * surface->w);
    auto y = (int)(v * surface->h);
    
    if (x >= 0 && x < surface->w && y >= 0 && y < surface->h)
    { return SDL_ReadPixel(surface, x, y); }
    else
    { return { 0, 0, 0, 255 }; }
}

inline SDL_Surface* sampler = nullptr;

void BlitTriangle(SDL_Surface* surface, const glm::vec2& clip, const Triangle3D& triangle)
{
    auto& verts = triangle.vertices;
    
    if (verts[0].pos.y == verts[1].pos.y && verts[1].pos.y == verts[2].pos.y)
    { return; }
    
    if (verts[0].pos.y != verts[1].pos.y && verts[1].pos.y != verts[2].pos.y && verts[2].pos.y != verts[0].pos.y)
    {
        // split triangle in two and recurse
        const Vertex3D* vert1 = &verts[0];
        const Vertex3D* vert2 = &verts[1];
        const Vertex3D* vert3 = &verts[2];
        
        // sort vertices in descending order
        if (vert2->pos.y < vert1->pos.y)
        { std::swap(vert1, vert2); }
        
        if (vert3->pos.y < vert2->pos.y)
        { std::swap(vert2, vert3); }
        
        if (vert2->pos.y < vert1->pos.y)
        { std::swap(vert1, vert2); }
        
        // prepare to interpolate vertices in world space
        auto w1 = 1 / vert1->pos.z;
        auto w2 = 1 / vert2->pos.z;
        auto w3 = 1 / vert3->pos.z;
        
        auto color1 = ToInterpSpace(vert1->color, w1);
        auto color3 = ToInterpSpace(vert3->color, w3);
        
        auto uv1 = vert1->uv * w1;
        auto uv3 = vert3->uv * w3;
        
        // find extra vertex
        auto p = InvLerp(vert2->pos.y, vert1->pos.y, vert3->pos.y);
        auto wx = Lerp(w1, w3, p);
        
        Vertex3D x_vert
        {
            Lerp(vert1->pos, vert3->pos, p),
            ColorFromInterpSpace(Lerp(color1, color3, p), wx),
            Lerp(uv1, uv3, p) / wx,
        };
        
        x_vert.pos.y = vert2->pos.y;
        x_vert.pos.z = Lerp(vert1->pos.z * w1, vert3->pos.z * w3, p) / wx;
        
        // draw top and bottom triangles
        BlitTriangle(surface, clip, { *vert1, *vert2, x_vert });
        BlitTriangle(surface, clip, { *vert3, *vert2, x_vert });
    }
    else if (verts[0].pos.y != verts[1].pos.y && verts[1].pos.y == verts[2].pos.y)
    {
        auto y1 = verts[0].pos.y;
        auto y2 = verts[1].pos.y;
        auto height = std::abs(y2 - y1);
        auto remain = height - std::floor(height);
        
        const Vertex3D* t_vert = &verts[0];
        const Vertex3D* l_vert;
        const Vertex3D* r_vert;
        
        if (verts[1].pos.x < verts[2].pos.x)
        {
            l_vert = &verts[1];
            r_vert = &verts[2];
        }
        else
        {
            l_vert = &verts[2];
            r_vert = &verts[1];
        }
        
        auto wt = 1 / t_vert->pos.z;
        auto wl = 1 / l_vert->pos.z;
        auto wr = 1 / r_vert->pos.z;
        
        auto t_color = ToInterpSpace(t_vert->color, wt);
        auto l_color = ToInterpSpace(l_vert->color, wl);
        auto r_color = ToInterpSpace(r_vert->color, wr);
        
        auto t_uv = t_vert->uv * wt;
        auto l_uv = l_vert->uv * wl;
        auto r_uv = r_vert->uv * wr;
        
        float ct;
        float cb;
        
        if (y1 < y2)
        {
            ct = Remap(1.0f, y1, y2, 0.0f, height);
            cb = Remap(clip.y - 1.0f, y1, y2, 0.0f, height);
        }
        else
        {
            ct = Remap(clip.y - 1.0f, y1, y2, 0.0f, height);
            cb = Remap(1.0f, y1, y2, 0.0f, height);
        }
        
        for (float y = std::max(0.0f, ct) + remain; y <= std::min(height, cb); y += 1.0f)
        {
            auto x1 = Remap(y, 0.0f, height, t_vert->pos.x, l_vert->pos.x);
            auto x2 = Remap(y, 0.0f, height, t_vert->pos.x, r_vert->pos.x);
            
            for (float x = std::max(1.0f, x1); x <= std::min(clip.x - 1.0f, x2); x += 1.0f)
            {
                auto xp = InvLerp(x, x1, x2);
                auto yp = InvLerp(y, 0.0, height);
                
                auto w = Lerp(wt, Lerp(wl, wr, xp), yp);
                auto lerp_color = Lerp(t_color, Lerp(l_color, r_color, xp), yp);
                auto lerp_uv = Lerp(t_uv, Lerp(l_uv, r_uv, xp), yp);
                
                auto color = ColorFromInterpSpace(lerp_color, w);
                auto uv = lerp_uv / w;
                
                if (sampler != nullptr)
                { color = Sample(sampler, uv.x, uv.y); }
                
                auto yy = (int)(Lerp(y1, y2, yp));
                auto xx = (int)(x);
                
                SDL_Blit(surface, xx, yy, color);
            }
        }
    }
    else
    {
        if (verts[0].pos.y == verts[1].pos.y)
        {
            BlitTriangle(surface, clip, { verts[2], verts[0], verts[1] });
        }
        else // if (verts[0].pos.y == verts[2].pos.y)
        {
            BlitTriangle(surface, clip, { verts[1], verts[2], verts[0] });
        }
    }
}

glm::vec3 Scale3D(const glm::vec3& pos, const Screen& screen)
{
    auto diff = screen.width - screen.height;
    
    return glm::vec3
    {
        Remap(pos.x / pos.z, -1.0, 1.0, diff / 2.0f, screen.height + diff / 2.0f),
        Remap(pos.y / pos.z, -1.0, 1.0, 0.0f, screen.height),
        pos.z
    };
}

void BlitTriangle3D(SDL_Surface* surface, const Screen& screen, const Triangle3D& triangle)
{
    auto& verts = triangle.vertices;
    
    // can't draw triangles that are partially beind us yet
    if (verts[0].pos.z <= 0.0f || verts[1].pos.z <= 0.0f || verts[2].pos.z <= 0.0f)
    { return; }
    
    BlitTriangle(surface, { screen.width, screen.height }, Triangle3D{
        Vertex3D{ Scale3D(verts[0].pos, screen), verts[0].color, verts[0].uv },
        Vertex3D{ Scale3D(verts[1].pos, screen), verts[1].color, verts[1].uv },
        Vertex3D{ Scale3D(verts[2].pos, screen), verts[2].color, verts[2].uv },
    });
}

void BlitTriangle3D(SDL_Surface* surface, const Camera3D& camera, const Screen& screen, const Triangle3D& triangle)
{
    auto& verts = triangle.vertices;
    
    BlitTriangle3D(surface, screen, Triangle3D{
        Vertex3D{ glm::rotateY(verts[0].pos - camera.pos, glm::radians(camera.angle)), verts[0].color, verts[0].uv },
        Vertex3D{ glm::rotateY(verts[1].pos - camera.pos, glm::radians(camera.angle)), verts[1].color, verts[1].uv },
        Vertex3D{ glm::rotateY(verts[2].pos - camera.pos, glm::radians(camera.angle)), verts[2].color, verts[2].uv },
    });
}

void BlitModel3D(SDL_Surface* surface, const Camera3D& camera, const Screen& screen, const Model3D& model)
{
    for (auto& triangle: model.triangles)
    { BlitTriangle3D(surface, camera, screen, triangle); }
}

int main(int, char**)
{
    // init sdl
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    
    // create window and renderer
    SDL_Window* window = SDL_CreateWindow("SmolSoft3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    
    // prevent window from appearing white for a split second at startup
    SDL_RenderClear(renderer);
    SDL_ShowWindow(window);
    
    // create surface and its texture
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 600, 400, 32, SDL_PIXELFORMAT_BGRA32);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Surface* goober = IMG_Load("./assets/goober.png");
    // goober = SDL_ConvertSurface(surface, surface->format, 0);
    
    // camera
    Screen screen{ 600.0f, 400.0f };
    Camera3D camera{ glm::vec3(0, -0.5, 0), 0.0f, 45.0f };
    
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
            }
        }
        
        // get key states
        auto keys = SDL_GetKeyboardState(nullptr);
        bool up     = keys[SDL_GetScancodeFromKey(SDLK_w)];
        bool down   = keys[SDL_GetScancodeFromKey(SDLK_s)];
        bool left   = keys[SDL_GetScancodeFromKey(SDLK_a)];
        bool right  = keys[SDL_GetScancodeFromKey(SDLK_d)];
        bool tleft  = keys[SDL_GetScancodeFromKey(SDLK_LEFT)];
        bool tright = keys[SDL_GetScancodeFromKey(SDLK_RIGHT)];
        
        camera.angle += 1.5f * ((tleft ? 1.0f : 0.0f) - (tright ? 1.0f : 0.0f));
        
        auto movement = glm::vec3((right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f), 0.0f, (up ? 1.0f : 0.0f) - (down ? 1.0f : 0.0f));
        camera.pos += glm::rotateY(0.02f * movement, -glm::radians(camera.angle));
        
        // do rendering stuff
        SDL_RenderClear(renderer);
        SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 255));
        
        Model3D floor_model
        {
            {
                Triangle3D
                {
                    Vertex3D{ glm::vec3( 1.0f, 1.0f, 2.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(1, 1) },
                    Vertex3D{ glm::vec3(-1.0f, 1.0f, 2.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(0, 1) },
                    Vertex3D{ glm::vec3(-1.0f, 1.0f, 4.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(0, 0) },
                },
                Triangle3D
                {
                    Vertex3D{ glm::vec3(-1.0f, 1.0f, 4.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(0, 0) },
                    Vertex3D{ glm::vec3( 1.0f, 1.0f, 4.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(1, 0) },
                    Vertex3D{ glm::vec3( 1.0f, 1.0f, 2.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(1, 1) },
                }
            }
        };
        
        Model3D triangle_model
        {
            {
                Triangle3D
                {
                    Vertex3D{ glm::vec3( 0.0f, -1.0f, 6.0f), SDL_Color{ 255, 0, 0, 255 } },
                    Vertex3D{ glm::vec3(-1.0f,  1.0f, 5.0f), SDL_Color{ 0, 255, 0, 255 } },
                    Vertex3D{ glm::vec3( 1.0f,  1.0f, 7.0f), SDL_Color{ 0, 0, 255, 255 } },
                }
            }
        };
        
        sampler = goober;
        BlitModel3D(surface, camera, screen, floor_model);
        
        sampler = nullptr;
        BlitModel3D(surface, camera, screen, triangle_model);
        
        SDL_UpdateTexture(texture, nullptr, surface->pixels, surface->pitch);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
    
    // quickly hide window to be more responsive
    SDL_HideWindow(window);
    
    IMG_Quit();
    SDL_Quit();
    return 0;
}
