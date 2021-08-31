#pragma once
#include <cmath>
#include <algorithm>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>


// linearly interpolates between value a and b over the progress p
inline constexpr float Lerp(float a, float b, float p)
{
    return a + p * (b - a);
}


// returns the progress of x between a and b
inline constexpr float InvLerp(float x, float a, float b)
{
    return (x - a) / (b - a);
}


// combines Lerp and InvLerp to convert x from range [a, b] to [c, d]
inline constexpr float Remap(float x, float a, float b, float c, float d)
{
    return Lerp(c, d, InvLerp(x, a, b));
}


// linearly interpolates two SDL_Color values
inline constexpr SDL_Color Lerp(const SDL_Color& a, const SDL_Color& b, float p)
{
    return SDL_Color
    {
        (Uint8)Lerp((float)a.r, (float)b.r, p),
        (Uint8)Lerp((float)a.g, (float)b.g, p),
        (Uint8)Lerp((float)a.b, (float)b.b, p),
        (Uint8)Lerp((float)a.a, (float)b.a, p),
    };
}


// linearly interpolates two glm::vec2 values
inline constexpr glm::vec2 Lerp(const glm::vec2& a, const glm::vec2& b, float p)
{
    return a + p * (b - a);
}


// linearly interpolates two glm::vec3 values
inline constexpr glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, float p)
{
    return a + p * (b - a);
}


// linearly interpolates two glm::vec4 values
inline constexpr glm::vec4 Lerp(const glm::vec4& a, const glm::vec4& b, float p)
{
    return a + p * (b - a);
}


// if x is smaller than a or greater than b, it is clamped back to that range
inline constexpr float Clamp(float x, float a, float b)
{
    return std::max(std::min(x, b), a);
}


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
        Uint8(Clamp(color.x, 0.0f, 255.0f)),
        Uint8(Clamp(color.y, 0.0f, 255.0f)),
        Uint8(Clamp(color.z, 0.0f, 255.0f)),
        Uint8(Clamp(color.w, 0.0f, 255.0f)),
    };
}


// blends two SDL_Color values together
inline constexpr SDL_Color Blend(const SDL_Color& a, const SDL_Color& b)
{
    return ToColor(((ToVec4(a) / 255.0f) * (ToVec4(b) / 255.0f)) * 255.0f);
}