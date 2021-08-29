#pragma once
#include <SDL2/SDL.h>


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