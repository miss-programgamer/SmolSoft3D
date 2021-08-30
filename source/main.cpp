#include <utility>
#include <array>
#include <cmath>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "sdl_extra.hpp"


// converts an SDL_Color to a glm::vec4
inline constexpr glm::vec4 ToVec4(const SDL_Color& color)
{
    return glm::vec4
    {
        float(color.r),
        float(color.g),
        float(color.b),
        float(color.a),
    };
}


// converts a glm::vec4 to an SDL_Color
inline constexpr SDL_Color ToColor(const glm::vec4& color)
{
    return SDL_Color
    {
        Uint8(color.x),
        Uint8(color.y),
        Uint8(color.z),
        Uint8(color.w),
    };
}


struct Vertex3D
{
    glm::vec4 pos;
    glm::vec4 color;
    glm::vec2 uv;
    
    inline constexpr Vertex3D(const glm::vec3& pos):
        pos(pos, 1.0f),
        color{ 255, 255, 255, 255 },
        uv(0.0f, 0.0f)
    {}
    
    inline constexpr Vertex3D(const glm::vec3& pos, const SDL_Color& color):
        pos(pos, 1.0f),
        color(ToVec4(color)),
        uv(0.0f, 0.0f)
    {}
    
    inline constexpr Vertex3D(const glm::vec3& pos, const glm::vec2& uv):
        pos(pos, 1.0f),
        color{ 255, 255, 255, 255 },
        uv(uv)
    {}
    
    inline constexpr Vertex3D(const glm::vec3& pos, const SDL_Color& color, const glm::vec2& uv):
        pos(pos, 1.0f),
        color(ToVec4(color)),
        uv(uv)
    {}
    
    inline constexpr Vertex3D(const glm::vec3& pos, const glm::vec4& color, const glm::vec2& uv):
        pos(pos, 1.0f),
        color(color),
        uv(uv)
    {}
    
    inline constexpr Vertex3D(const glm::vec4& pos, const glm::vec4& color, const glm::vec2& uv):
        pos(pos),
        color(color),
        uv(uv)
    {}
    
    inline constexpr Vertex3D Interp() const
    {
        auto w = 1 / pos.z;
        return Vertex3D(glm::vec4(pos.x * w, pos.y * w, pos.z * w, w), color * w, uv * w);
    }
    
    inline constexpr Vertex3D Restore() const
    {
        auto w = pos.w;
        return Vertex3D(glm::vec4(pos.x / w, pos.y / w, pos.z / w, 1.0f), color / w, uv / w);
    }
};


struct Triangle3D
{
    std::array<Vertex3D, 3> vertices;
    
    inline int GetWindingOrder() const
    {
        auto pos0 = glm::vec2(vertices[0].pos);
        auto pos1 = glm::vec2(vertices[1].pos);
        auto pos2 = glm::vec2(vertices[2].pos);
        
        auto side01 = pos1 - pos0;
        auto side02 = pos2 - pos0;
        auto normal = glm::vec2(side01.y, -side01.x);
        
        return (int)glm::sign(glm::dot(normal, side02));
    }
};


struct Model3D
{
    std::vector<Triangle3D> triangles;
};


struct Camera3D
{
    glm::vec3 pos;
    float pitch;
    float yaw;
};


struct Screen
{
    float width;
    float height;
    float fov;
};


struct Target
{
    SDL_Surface* surface;
    std::vector<float> depth_buffer;
    
    inline Target(SDL_Surface* surface):
        surface(surface)
    {
        depth_buffer.resize(surface->w * surface->h);
        std::fill(depth_buffer.begin(), depth_buffer.end(), 1.0f);
    }
    
    void Blit(int x, int y, float depth, const SDL_Color& color)
    {
        if (x >= 0 && x < surface->w && y >= 0 && y < surface->h)
        {
            auto depth_i = y * surface->w + x;
            if (depth < depth_buffer[depth_i])
            {
                depth_buffer[depth_i] = depth;
                SDL_Blit(surface, x, y, color);
            }
        }
    }
    
    SDL_Color Read(int x, int y) const
    {
        return SDL_ReadPixel(surface, x, y);
    }
    
    void ClearDepth()
    {
        std::fill(depth_buffer.begin(), depth_buffer.end(), 1.0f);
    }
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

inline Vertex3D Lerp(const Vertex3D& a, const Vertex3D& b, float p)
{
    return Vertex3D
    {
        Lerp(a.pos, b.pos, p),
        Lerp(a.color, b.color, p),
        Lerp(a.uv, b.uv, p),
    };
}

inline float Clamp(float x, float a, float b)
{
    return std::max(std::min(x, b), a);
}

inline constexpr SDL_Color Blend(const SDL_Color& a, const SDL_Color& b)
{
    return ToColor(((ToVec4(a) / 255.0f) * (ToVec4(b) / 255.0f)) * 255.0f);
}

inline SDL_Color Sample(SDL_Surface* surface, float u, float v)
{
    auto x = (int)(u * surface->w);
    auto y = (int)(v * surface->h);
    
    if (x >= 0 && x < surface->w && y >= 0 && y < surface->h)
    { return SDL_ReadPixel(surface, x, y); }
    else
    { return { 0, 0, 0, 255 }; }
}

inline SDL_Surface* sampler = nullptr;

inline void BlitTriangle(Target& target, const glm::vec2& clip, const Triangle3D& triangle)
{
    auto& verts = triangle.vertices;
    
    // completely flat triangles don't get drawn
    if (verts[0].pos.y == verts[1].pos.y && verts[1].pos.y == verts[2].pos.y)
    { return; }
    
    // ignore triangles with a clockwise winding order
    if (triangle.GetWindingOrder() != -1)
    { return; }
    
    // can't draw triangles that are partially beind us (should never happen in practice)
    if (verts[0].pos.z <= 0.0f || verts[1].pos.z <= 0.0f || verts[2].pos.z <= 0.0f)
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
        
        // find extra vertex
        auto p = InvLerp(vert2->pos.y, vert1->pos.y, vert3->pos.y);
        auto vert4 = Lerp(vert1->Interp(), vert3->Interp(), p).Restore();
        
        // adjust vertex x and y to be in screen space again
        vert4.pos.x = Lerp(vert1->pos.x, vert3->pos.x, p);
        vert4.pos.y = vert2->pos.y;
        
        // draw top and bottom triangles
        if (auto order = Triangle3D{ *vert1, *vert2, *vert3 }.GetWindingOrder(); order == -1)
        {
            BlitTriangle(target, clip, { *vert1, *vert2, vert4 });
            BlitTriangle(target, clip, { *vert2, *vert3, vert4 });
        }
        else if (order == 1)
        {
            BlitTriangle(target, clip, { *vert2, *vert1, vert4 });
            BlitTriangle(target, clip, { *vert3, *vert2, vert4 });
        }
    }
    else if (verts[0].pos.y != verts[1].pos.y && verts[1].pos.y == verts[2].pos.y)
    {
        // find height
        auto y1 = verts[0].pos.y;
        auto y2 = verts[1].pos.y;
        auto height = std::abs(std::round(y2) - std::round(y1));
        
        // get top vertex (slight misnomer; can also be single bottom vertex)
        const Vertex3D* t_vert = &verts[0];
        
        // find leftmost and rightmost vertices
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
        
        // get interpolation-ready vertices
        auto t_vert_i = t_vert->Interp();
        auto l_vert_i = l_vert->Interp();
        auto r_vert_i = r_vert->Interp();
        
        // determine vertical clipping
        float t_clip;
        float b_clip;
        
        if (y1 < y2)
        {
            t_clip = std::round(Remap(0.0f,   y1, y2, 0.0f, height)) + 0.5f;
            b_clip = std::round(Remap(clip.y, y1, y2, 0.0f, height)) - 0.5f;
        }
        else
        {
            t_clip = std::round(Remap(clip.y, y1, y2, 0.0f, height)) + 0.5f;
            b_clip = std::round(Remap(0.0f,   y1, y2, 0.0f, height)) - 0.5f;
        }
        
        // draw pixels
        for (float y = std::max(0.5f, t_clip); y <= std::min(height, b_clip); y += 1.0f)
        {
            // find edges of current row
            auto x1 = std::round(Remap(y, 0.0f, height, t_vert->pos.x, l_vert->pos.x));
            auto x2 = std::round(Remap(y, 0.0f, height, t_vert->pos.x, r_vert->pos.x));
            
            // draw current row
            for (float x = std::max(x1, 0.5f); x <= std::min(x2, clip.x - 0.5f); x += 1.0f)
            {
                // find progress across x and y axes
                auto xp = InvLerp(x, x1, x2);
                auto yp = InvLerp(y, 0.0, height);
                
                // interpolate vertices in 2D
                auto vertex = Lerp(t_vert_i, Lerp(l_vert_i, r_vert_i, xp), yp).Restore();
                
                // determine color
                SDL_Color color = ToColor(vertex.color);
                
                // blend sample color
                if (sampler != nullptr)
                { color = Blend(color, Sample(sampler, vertex.uv.x, vertex.uv.y)); }
                
                // determine position at which to draw our pixel
                auto yy = (int)(Lerp(y1, y2, yp));
                auto xx = (int)(x);
                
                // blit the pixel
                target.Blit(xx, yy, vertex.pos.z / 10000.0f, color);
            }
        }
    }
    else
    {
        if (verts[0].pos.y == verts[1].pos.y)
        {
            BlitTriangle(target, clip, { verts[2], verts[0], verts[1] });
        }
        else // if (verts[0].pos.y == verts[2].pos.y)
        {
            BlitTriangle(target, clip, { verts[1], verts[2], verts[0] });
        }
    }
}

inline glm::vec3 Scale3D(const glm::vec3& pos, const Screen& screen)
{
    auto diff = screen.width - screen.height;
    auto fov_factor = screen.fov / 90.0f;
    
    return glm::vec3
    {
        Remap(pos.x / (pos.z * fov_factor), -1.0, 1.0, diff / 2.0f, screen.height + diff / 2.0f),
        Remap(pos.y / (pos.z * fov_factor), -1.0, 1.0, screen.height, 0.0f),
        pos.z
    };
}

inline void BlitTriangleScaled(Target& target, const Screen& screen, const Triangle3D& triangle)
{
    auto& verts = triangle.vertices;
    
    BlitTriangle(target, { screen.width, screen.height }, Triangle3D{
        Vertex3D{ Scale3D(verts[0].pos, screen), verts[0].color, verts[0].uv },
        Vertex3D{ Scale3D(verts[1].pos, screen), verts[1].color, verts[1].uv },
        Vertex3D{ Scale3D(verts[2].pos, screen), verts[2].color, verts[2].uv },
    });
}

inline void BlitTriangleClip(Target& target, const Screen& screen, const Triangle3D& triangle)
{
    auto& verts = triangle.vertices;
    
    float clip_plane = 0.5f;
    bool vert0_clip = verts[0].pos.z < clip_plane;
    bool vert1_clip = verts[1].pos.z < clip_plane;
    bool vert2_clip = verts[2].pos.z < clip_plane;
    
    if (vert0_clip && vert1_clip && vert2_clip)
    { return; }
    
    if (vert0_clip && !vert1_clip && !vert2_clip)
    {
        auto to_vert1 = Lerp(verts[0], verts[1], InvLerp(clip_plane, verts[0].pos.z, verts[1].pos.z));
        auto to_vert2 = Lerp(verts[0], verts[2], InvLerp(clip_plane, verts[0].pos.z, verts[2].pos.z));
        auto mid_vert = Lerp(verts[1], verts[2], 0.5f);
        
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert1, verts[1], mid_vert });
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert2, to_vert1, mid_vert });
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert2, mid_vert, verts[2] });
    }
    else if (!vert0_clip && vert1_clip && !vert2_clip)
    {
        auto to_vert0 = Lerp(verts[1], verts[0], InvLerp(clip_plane, verts[1].pos.z, verts[0].pos.z));
        auto to_vert2 = Lerp(verts[1], verts[2], InvLerp(clip_plane, verts[1].pos.z, verts[2].pos.z));
        auto mid_vert = Lerp(verts[0], verts[2], 0.5f);
        
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert0, mid_vert, verts[0] });
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert2, mid_vert, to_vert0 });
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert2, verts[2], mid_vert });
    }
    else if (!vert0_clip && !vert1_clip && vert2_clip)
    {
        auto to_vert0 = Lerp(verts[2], verts[0], InvLerp(clip_plane, verts[2].pos.z, verts[0].pos.z));
        auto to_vert1 = Lerp(verts[2], verts[1], InvLerp(clip_plane, verts[2].pos.z, verts[1].pos.z));
        auto mid_vert = Lerp(verts[0], verts[1], 0.5f);
        
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert0, verts[0], mid_vert });
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert1, to_vert0, mid_vert });
        BlitTriangleScaled(target, screen, Triangle3D{ to_vert1, mid_vert, verts[1] });
    }
    else if (!vert0_clip && vert1_clip && vert2_clip)
    {
        auto to_vert1 = Lerp(verts[0], verts[1], InvLerp(clip_plane, verts[0].pos.z, verts[1].pos.z));
        auto to_vert2 = Lerp(verts[0], verts[2], InvLerp(clip_plane, verts[0].pos.z, verts[2].pos.z));
        BlitTriangleScaled(target, screen, Triangle3D{ verts[0], to_vert1, to_vert2 });
    }
    else if (vert0_clip && !vert1_clip && vert2_clip)
    {
        auto to_vert0 = Lerp(verts[1], verts[0], InvLerp(clip_plane, verts[1].pos.z, verts[0].pos.z));
        auto to_vert2 = Lerp(verts[1], verts[2], InvLerp(clip_plane, verts[1].pos.z, verts[2].pos.z));
        BlitTriangleScaled(target, screen, Triangle3D{ verts[1], to_vert2, to_vert0 });
    }
    else if (vert0_clip && vert1_clip && !vert2_clip)
    {
        auto to_vert0 = Lerp(verts[2], verts[0], InvLerp(clip_plane, verts[2].pos.z, verts[0].pos.z));
        auto to_vert1 = Lerp(verts[2], verts[1], InvLerp(clip_plane, verts[2].pos.z, verts[1].pos.z));
        BlitTriangleScaled(target, screen, Triangle3D{ verts[2], to_vert0, to_vert1 });
    }
    else
    {
        BlitTriangleScaled(target, screen, triangle);
    }
}

inline void BlitTriangleTransform(Target& target, const Camera3D& camera, const Screen& screen, const Triangle3D& triangle)
{
    auto& verts = triangle.vertices;
    
    BlitTriangleClip(target, screen, Triangle3D{
        Vertex3D{ glm::rotateX(glm::rotateY(glm::vec3(verts[0].pos) - camera.pos, glm::radians(camera.pitch)), glm::radians(camera.yaw)), verts[0].color, verts[0].uv },
        Vertex3D{ glm::rotateX(glm::rotateY(glm::vec3(verts[1].pos) - camera.pos, glm::radians(camera.pitch)), glm::radians(camera.yaw)), verts[1].color, verts[1].uv },
        Vertex3D{ glm::rotateX(glm::rotateY(glm::vec3(verts[2].pos) - camera.pos, glm::radians(camera.pitch)), glm::radians(camera.yaw)), verts[2].color, verts[2].uv },
    });
}

inline void BlitModel(Target& target, const Camera3D& camera, const Screen& screen, const Model3D& model)
{
    for (auto& triangle: model.triangles)
    { BlitTriangleTransform(target, camera, screen, triangle); }
}

int main(int, char**)
{
    // init sdl
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    
    // create window and renderer
    SDL_Window* window = SDL_CreateWindow("SmolSoft3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600 * 2, 400 * 2, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    
    // prevent window from appearing white for a split second at startup
    SDL_RenderClear(renderer);
    SDL_ShowWindow(window);
    
    // create surface and its texture
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 600, 400, 32, SDL_PIXELFORMAT_BGRA32);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    Target target = surface;
    
    // load image to sample
    SDL_Surface* goober = IMG_Load("./assets/goober.png");
    
    // camera
    Screen screen{ 600.0f, 400.0f, 60.0f };
    Camera3D camera{ glm::vec3(0, 0.5, 0), 0.0f, 0.0f };
    float sensitivity = 0.2f;
    
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
                        camera.pitch -= sensitivity * (float)event.motion.xrel;
                        camera.yaw   -= sensitivity * (float)event.motion.yrel;
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
        
        // get key states
        auto keys = SDL_GetKeyboardState(nullptr);
        bool up     = keys[SDL_GetScancodeFromKey(SDLK_w)];
        bool down   = keys[SDL_GetScancodeFromKey(SDLK_s)];
        bool left   = keys[SDL_GetScancodeFromKey(SDLK_a)];
        bool right  = keys[SDL_GetScancodeFromKey(SDLK_d)];
        bool tup    = keys[SDL_GetScancodeFromKey(SDLK_UP)];
        bool tdown  = keys[SDL_GetScancodeFromKey(SDLK_DOWN)];
        bool tleft  = keys[SDL_GetScancodeFromKey(SDLK_LEFT)];
        bool tright = keys[SDL_GetScancodeFromKey(SDLK_RIGHT)];
        
        camera.pitch += 1.5f * ((tleft ? 1.0f : 0.0f) - (tright ? 1.0f : 0.0f));
        camera.yaw += 1.5f * ((tup ? 1.0f : 0.0f) - (tdown ? 1.0f : 0.0f));
        
        camera.yaw = Clamp(camera.yaw, -89.9f, 89.9f);
        
        auto movement = glm::vec3((right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f), 0.0f, (up ? 1.0f : 0.0f) - (down ? 1.0f : 0.0f));
        camera.pos += glm::rotateY(0.02f * movement, -glm::radians(camera.pitch));
        
        // do rendering stuff
        SDL_RenderClear(renderer);
        SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 255));
        target.ClearDepth();
        
        Model3D floor_model
        {
            {
                Triangle3D
                {
                    Vertex3D{ glm::vec3( 1.0f, -1.0f, 2.0f), SDL_Color{ 255, 255, 255, 255 }, glm::vec2(1, 1) },
                    Vertex3D{ glm::vec3(-1.0f, -1.0f, 2.0f), SDL_Color{ 255, 255, 255, 255 }, glm::vec2(0, 1) },
                    Vertex3D{ glm::vec3(-1.0f, -1.0f, 4.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(0, 0) },
                },
                Triangle3D
                {
                    Vertex3D{ glm::vec3(-1.0f, -1.0f, 4.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(0, 0) },
                    Vertex3D{ glm::vec3( 1.0f, -1.0f, 4.0f), SDL_Color{ 64, 64, 64, 255 }, glm::vec2(1, 0) },
                    Vertex3D{ glm::vec3( 1.0f, -1.0f, 2.0f), SDL_Color{ 255, 255, 255, 255 }, glm::vec2(1, 1) },
                }
            }
        };
        
        Model3D triangle_model
        {
            {
                Triangle3D
                {
                    Vertex3D{ glm::vec3( 1.0f, -1.0f, 7.0f), SDL_Color{ 0, 0, 255, 255 } },
                    Vertex3D{ glm::vec3(-1.0f, -1.0f, 5.0f), SDL_Color{ 0, 255, 0, 255 } },
                    Vertex3D{ glm::vec3( 0.0f,  1.0f, 6.0f), SDL_Color{ 255, 0, 0, 255 } },
                }
            }
        };
        
        Model3D spike_model
        {
            {
                Triangle3D
                {
                    Vertex3D{ glm::vec3( 0.0f,  2.0f, 3.0f), SDL_Color{ 128, 128, 128, 255 } },
                    Vertex3D{ glm::vec3( 0.5f, -1.0f, 3.0f), SDL_Color{ 128, 128, 128, 255 } },
                    Vertex3D{ glm::vec3(-0.5f, -1.0f, 3.0f), SDL_Color{ 128, 128, 128, 255 } },
                }
            }
        };
        
        sampler = goober;
        BlitModel(target, camera, screen, floor_model);
        
        sampler = nullptr;
        BlitModel(target, camera, screen, triangle_model);
        BlitModel(target, camera, screen, spike_model);
        
        // for (int i = 0; i < 100; ++i)
        //     BlitModel(target, camera, screen, triangle_model);
        
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
