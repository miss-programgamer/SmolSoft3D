#pragma once
#include <filesystem>
namespace fs = std::filesystem;

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include "math.hpp"


// a vertex in 3D space with w scaling, color, and uv information
struct Vertex3D
{
    glm::vec4 pos;
    glm::vec4 color;
    glm::vec2 uv;
    
    // constructs a default vertex
    inline constexpr Vertex3D():
        pos(0.0f, 0.0f, 0.0f, 1.0f),
        color(255, 255, 255, 255),
        uv(0.0f, 0.0f)
    {}
    
    // constructs a vertex from a 3D point
    inline constexpr Vertex3D(const glm::vec3& pos):
        pos(pos, 1.0f),
        color{ 255, 255, 255, 255 },
        uv(0.0f, 0.0f)
    {}
    
    // constructs a vertex from a 3D point and color
    inline constexpr Vertex3D(const glm::vec3& pos, const SDL_Color& color):
        pos(pos, 1.0f),
        color(ToVec4(color)),
        uv(0.0f, 0.0f)
    {}
    
    // constructs a vertex from a 3D point and texture coordinate
    inline constexpr Vertex3D(const glm::vec3& pos, const glm::vec2& uv):
        pos(pos, 1.0f),
        color{ 255, 255, 255, 255 },
        uv(uv)
    {}
    
    // constructs a vertex from a 3D point, color, and texture coordinate
    inline constexpr Vertex3D(const glm::vec3& pos, const SDL_Color& color, const glm::vec2& uv):
        pos(pos, 1.0f),
        color(ToVec4(color)),
        uv(uv)
    {}
    
    // constructs a vertex from a 3D point, glm::vec4 color value, and texture coordinate
    inline constexpr Vertex3D(const glm::vec3& pos, const glm::vec4& color, const glm::vec2& uv):
        pos(pos, 1.0f),
        color(color),
        uv(uv)
    {}
    
    // constructs a vertex from a 3D point, w scalar, glm::vec4 color value, and texture coordinate
    inline constexpr Vertex3D(const glm::vec4& pos, const glm::vec4& color, const glm::vec2& uv):
        pos(pos),
        color(color),
        uv(uv)
    {}
    
    // prepares a vertex to be interpolated by storing 1/pos.z in pos.w and multiplying every other value by pos.w
    inline constexpr Vertex3D Interp() const
    {
        auto w = 1 / pos.z;
        return Vertex3D(glm::vec4(pos.x * w, pos.y * w, pos.z * w, w), color * w, uv * w);
    }
    
    // restores a value after it has been interpolated by dividing every value by pos.w and storing 1 in pos.w
    inline constexpr Vertex3D Restore() const
    {
        auto w = pos.w;
        return Vertex3D(glm::vec4(pos.x / w, pos.y / w, pos.z / w, 1.0f), color / w, uv / w);
    }
};


// contains the 3 vertices of a single 3D triangle
struct Triangle3D
{
    std::array<Vertex3D, 3> vertices;
    
    // calculates the winding order of this triangle's vertices
    // (-1 = counter-clockwise, 1 = clockwise, 0 = completely flat)
    inline int GetWindingOrder() const
    {
        auto pos0 = glm::vec2(vertices[0].pos);
        auto pos1 = glm::vec2(vertices[1].pos);
        auto pos2 = glm::vec2(vertices[2].pos);
        
        auto span01 = pos1 - pos0;
        auto span02 = pos2 - pos0;
        auto normal = glm::vec2(span01.y, -span01.x);
        
        return (int)glm::sign(glm::dot(normal, span02));
    }
};


// contains all the triangles of a 3D model
struct Model3D
{
    std::vector<Triangle3D> triangles;
};


// loads a 3D model from a text file
inline std::optional<Model3D> LoadModel(const fs::path& filepath)
{
    if (std::ifstream file(filepath); file)
    {
        Model3D model;
        
        // read metadata
        size_t triangle_count;
        size_t format_count;
        std::vector<std::string> format;
        
        file >> triangle_count;
        file >> format_count;
        
        // read format
        format.resize(format_count);
        
        for (size_t f = 0; f < format_count; ++f)
        {
            file >> format[f];
        }
        
        // reserve the number of triangles used in advance
        model.triangles.reserve(triangle_count);
        
        // read each triangle's data
        for (size_t t = 0; t < triangle_count; ++t)
        {
            Triangle3D triangle;
            
            for (size_t v = 0; v < 3; ++v)
            {
                for (size_t f = 0; f < format_count; ++f)
                {
                    if (format[f] == "pos")
                    {
                        file >> triangle.vertices[v].pos.x;
                        file >> triangle.vertices[v].pos.y;
                        file >> triangle.vertices[v].pos.z;
                    }
                    else if (format[f] == "color")
                    {
                        file >> triangle.vertices[v].color.x;
                        file >> triangle.vertices[v].color.y;
                        file >> triangle.vertices[v].color.z;
                        file >> triangle.vertices[v].color.w;
                    }
                    else if (format[f] == "uv")
                    {
                        file >> triangle.vertices[v].uv.x;
                        file >> triangle.vertices[v].uv.y;
                    }
                }
            }
            
            model.triangles.push_back(triangle);
        }
        
        // return our result
        return model;
    }
    else
    {
        return std::nullopt;
    }
}


// shorthand for attempting to load a model from a file and into a reference
inline void TryLoadModel(const fs::path& filepath, Model3D& out_model)
{
    if (auto model = LoadModel(filepath); model)
    {
        out_model = std::move(model.value());
    }
}


// contains the position and rotation of a camera in 3D space
struct Camera3D
{
    glm::vec3 pos;
    float pitch;
    float yaw;
    
    // turns the camera around two axes by the given amounts
    inline void Turn(float pitch_delta, float yaw_delta)
    {
        pitch += pitch_delta;
        yaw += yaw_delta;
        yaw = Clamp(yaw, -89.9f, 89.9f);
    }
    
    // moves the camera along three axes aligned with the camera's pitch
    inline void Move(float advance, float strafe, float ascend)
    {
        pos += glm::rotateY(glm::vec3(strafe, ascend, advance), -glm::radians(pitch));
    }
};


// contains the information necessary to transform vertices from view space to screen space
struct Screen
{
    float width;
    float height;
    float fov;
};


// a rendering target with a frame buffer
struct Target
{
    SDL_Surface* surface;
    std::vector<float> depth_buffer;
    
    // constructs a target from a surface and resizes the depth buffer accordingly
    inline Target(SDL_Surface* surface):
        surface(surface)
    {
        depth_buffer.resize(surface->w * surface->h);
        std::fill(depth_buffer.begin(), depth_buffer.end(), 1.0f);
    }
    
    // blits a single pixel onto the render target if the given depth permits it
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
    
    // reads a single pixel color from the surface
    SDL_Color Read(int x, int y) const
    {
        return SDL_ReadPixel(surface, x, y);
    }
    
    // clears the depth buffer by filling it with ones
    void ClearDepth()
    {
        std::fill(depth_buffer.begin(), depth_buffer.end(), 1.0f);
    }
    
    // clears the surface with the given SDL_Color value
    void ClearSurface(const SDL_Color& color)
    {
        Uint32 pixel = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
        SDL_FillRect(surface, nullptr, pixel);
    }
};


// linearly interpolates two Vertex3D values
inline Vertex3D Lerp(const Vertex3D& a, const Vertex3D& b, float p)
{
    return Vertex3D
    {
        Lerp(a.pos, b.pos, p),
        Lerp(a.color, b.color, p),
        Lerp(a.uv, b.uv, p),
    };
}


// translates the given point from world space to view space
inline glm::vec3 TranslateToView(const glm::vec3& pos, const Camera3D& camera)
{
    auto pitch = glm::radians(camera.pitch);
    auto yaw = glm::radians(camera.yaw);
    return glm::rotateX(glm::rotateY(pos - camera.pos, pitch), yaw);
}


// scales a 3D point from view space to screen space
inline glm::vec3 ScaleToScreen(const glm::vec3& pos, const Screen& screen)
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


// scales a 3D triangle from view space to screen space
inline Triangle3D ScaleToScreen(const Triangle3D& triangle, const Screen& screen)
{
    auto& verts = triangle.vertices;
    
    return Triangle3D
    {
        Vertex3D{ ScaleToScreen(verts[0].pos, screen), verts[0].color, verts[0].uv },
        Vertex3D{ ScaleToScreen(verts[1].pos, screen), verts[1].color, verts[1].uv },
        Vertex3D{ ScaleToScreen(verts[2].pos, screen), verts[2].color, verts[2].uv },
    };
}


// software renderer for 3D polygons
struct Renderer3D
{
    SDL_Surface* sampler = nullptr;
    
    // changes which SDL_Surface the renderer samples textures from, if any
    inline void SetSampler(SDL_Surface* sampler)
    {
        this->sampler = sampler;
    }
    
    // blits a single screen space triangle to the given target
    inline void BlitTriangle(Target& target, const glm::vec2& clip, const Triangle3D& triangle)
    {
        auto& verts = triangle.vertices;
        
        // completely flat triangles don't get drawn
        if (verts[0].pos.y == verts[1].pos.y && verts[1].pos.y == verts[2].pos.y)
        { return; }
        
        // ignore triangles with a clockwise winding order
        if (triangle.GetWindingOrder() != 1)
        { return; }
        
        // can't draw triangles that are partially beind us (should never happen in practice)
        if (verts[0].pos.z <= 0.0f || verts[1].pos.z <= 0.0f || verts[2].pos.z <= 0.0f)
        { return; }
        
        // in this case, the given triangle doesn't have a flat top/bottom, so we split it in two
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
            
            // draw top and bottom triangles (and do a bit of work to preserve winding order)
            if (auto order = Triangle3D{ *vert1, *vert2, *vert3 }.GetWindingOrder(); order == 1)
            {
                BlitTriangle(target, clip, { *vert1, *vert2, vert4 });
                BlitTriangle(target, clip, { *vert2, *vert3, vert4 });
            }
            else if (order == -1)
            {
                BlitTriangle(target, clip, { *vert2, *vert1, vert4 });
                BlitTriangle(target, clip, { *vert3, *vert2, vert4 });
            }
        }
        // this is where drawing triangles actually happens! finally!
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
                    { color = Blend(color, SDL_Sample(sampler, vertex.uv.x, vertex.uv.y)); }
                    
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
            // in these two cases, we rotate the triangle so the first vertex is the one pointing away from the flat top/bottom
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
    
    // clips the given view space triangle to the near plane, scales it to screen space, and blits the result
    inline void BlitClippedTriangle(Target& target, const Screen& screen, const Triangle3D& triangle)
    {
        auto& verts = triangle.vertices;
        
        // distance from the camera at which triangles get clipped
        float clip_plane = 0.1f;
        
        // which vertices are behind the clip plane
        bool vert0_clip = verts[0].pos.z < clip_plane;
        bool vert1_clip = verts[1].pos.z < clip_plane;
        bool vert2_clip = verts[2].pos.z < clip_plane;
        
        // screen space rectangle around which triangles are clipped (this happens inside of BlitTriangle)
        auto clip_vec = glm::vec2(screen.width, screen.height);
        
        // if every vertex is behind the clip plane, do nothing
        if (vert0_clip && vert1_clip && vert2_clip)
        { return; }
        
        // first three cases have one point behind the clip plane and create three new triangles (and preserves their winding order)
        if (vert0_clip && !vert1_clip && !vert2_clip)
        {
            auto to_vert1 = Lerp(verts[0], verts[1], InvLerp(clip_plane, verts[0].pos.z, verts[1].pos.z));
            auto to_vert2 = Lerp(verts[0], verts[2], InvLerp(clip_plane, verts[0].pos.z, verts[2].pos.z));
            auto mid_vert = Lerp(verts[1], verts[2], 0.5f);
            
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert1, verts[1], mid_vert }, screen));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert2, to_vert1, mid_vert }, screen));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert2, mid_vert, verts[2] }, screen));
        }
        else if (!vert0_clip && vert1_clip && !vert2_clip)
        {
            auto to_vert0 = Lerp(verts[1], verts[0], InvLerp(clip_plane, verts[1].pos.z, verts[0].pos.z));
            auto to_vert2 = Lerp(verts[1], verts[2], InvLerp(clip_plane, verts[1].pos.z, verts[2].pos.z));
            auto mid_vert = Lerp(verts[0], verts[2], 0.5f);
            
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert0, mid_vert, verts[0] }, screen));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert2, mid_vert, to_vert0 }, screen));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert2, verts[2], mid_vert }, screen));
        }
        else if (!vert0_clip && !vert1_clip && vert2_clip)
        {
            auto to_vert0 = Lerp(verts[2], verts[0], InvLerp(clip_plane, verts[2].pos.z, verts[0].pos.z));
            auto to_vert1 = Lerp(verts[2], verts[1], InvLerp(clip_plane, verts[2].pos.z, verts[1].pos.z));
            auto mid_vert = Lerp(verts[0], verts[1], 0.5f);
            
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert0, verts[0], mid_vert }, screen));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert1, to_vert0, mid_vert }, screen));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ to_vert1, mid_vert, verts[1] }, screen));
        }
        // next three cases have two points behind the clip plane and creating a single new triangle (and preserves its winding order)
        else if (!vert0_clip && vert1_clip && vert2_clip)
        {
            auto to_vert1 = Lerp(verts[0], verts[1], InvLerp(clip_plane, verts[0].pos.z, verts[1].pos.z));
            auto to_vert2 = Lerp(verts[0], verts[2], InvLerp(clip_plane, verts[0].pos.z, verts[2].pos.z));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ verts[0], to_vert1, to_vert2 }, screen));
        }
        else if (vert0_clip && !vert1_clip && vert2_clip)
        {
            auto to_vert0 = Lerp(verts[1], verts[0], InvLerp(clip_plane, verts[1].pos.z, verts[0].pos.z));
            auto to_vert2 = Lerp(verts[1], verts[2], InvLerp(clip_plane, verts[1].pos.z, verts[2].pos.z));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ verts[1], to_vert2, to_vert0 }, screen));
        }
        else if (vert0_clip && vert1_clip && !vert2_clip)
        {
            auto to_vert0 = Lerp(verts[2], verts[0], InvLerp(clip_plane, verts[2].pos.z, verts[0].pos.z));
            auto to_vert1 = Lerp(verts[2], verts[1], InvLerp(clip_plane, verts[2].pos.z, verts[1].pos.z));
            BlitTriangle(target, clip_vec, ScaleToScreen(Triangle3D{ verts[2], to_vert0, to_vert1 }, screen));
        }
        // final case simply draws the entire triangle unchanged because it's in front of us (and, obviously, preserves its winding order)
        else
        {
            BlitTriangle(target, clip_vec, ScaleToScreen(triangle, screen));
        }
    }
    
    // blits the given world space triangle to the given target
    inline void BlitWorldTriangle(Target& target, const Camera3D& camera, const Screen& screen, const Triangle3D& triangle, const glm::mat4& transform)
    {
        auto& verts = triangle.vertices;
        
        BlitClippedTriangle(target, screen, Triangle3D {
            Vertex3D{ TranslateToView(transform * verts[0].pos, camera), verts[0].color, verts[0].uv },
            Vertex3D{ TranslateToView(transform * verts[1].pos, camera), verts[1].color, verts[1].uv },
            Vertex3D{ TranslateToView(transform * verts[2].pos, camera), verts[2].color, verts[2].uv },
        });
    }
    
    // blits the given 3D model's triangles to the given target
    inline void Blit3DModel(Target& target, const Camera3D& camera, const Screen& screen, const Model3D& model, const glm::mat4& transform = glm::mat4(1.0f))
    {
        for (auto& triangle: model.triangles)
        {
            BlitWorldTriangle(target, camera, screen, triangle, transform);
        }
    }
};