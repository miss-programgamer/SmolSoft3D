#pragma once
#include <algorithm>

#include <SDL2/SDL.h>

#include "math.hpp"


// blits a single colored pixel onto the given surface at the given point
inline void SDL_Blit(SDL_Surface* surface, int x, int y, const SDL_Color& color)
{
    SDL_Rect rect{ x, y, 1, 1 };
    SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a));
}


// blits a 3x3 rectangle around the given pixel
inline void SDL_BlitBig(SDL_Surface* surface, int x, int y, const SDL_Color& color)
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


// reads the color of a single pixel in the given surface
inline SDL_Color SDL_ReadPixel(SDL_Surface* surface, int x, int y)
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


// samples a pixel in the given surface using normalized coordinates
inline SDL_Color SDL_Sample(SDL_Surface* surface, float u, float v)
{
    auto x = (int)Lerp(0.0f, float(surface->w), u);
    auto y = (int)Lerp(float(surface->h), 0.0f, v);
    
    if (x >= 0 && x <= surface->w && y >= 0 && y <= surface->h)
    { return SDL_ReadPixel(surface, std::min(x, surface->w - 1), std::min(y, surface->h - 1)); }
    else
    { return { 0, 0, 0, 255 }; }
}