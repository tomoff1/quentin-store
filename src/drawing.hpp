#pragma once
#include <SDL2_gfxPrimitives.h>
// Couleurs lisibles RRGGBBAA, converties explicitement sans dependance endian.
inline int qsBox(SDL_Renderer* r,Sint16 x,Sint16 y,Sint16 xx,Sint16 yy,Uint32 c){return boxRGBA(r,x,y,xx,yy,c>>24,(c>>16)&255,(c>>8)&255,c&255);}
inline int qsRounded(SDL_Renderer* r,Sint16 x,Sint16 y,Sint16 xx,Sint16 yy,Sint16 radius,Uint32 c){return roundedBoxRGBA(r,x,y,xx,yy,radius,c>>24,(c>>16)&255,(c>>8)&255,c&255);}
inline int qsCircle(SDL_Renderer* r,Sint16 x,Sint16 y,Sint16 radius,Uint32 c){return circleRGBA(r,x,y,radius,c>>24,(c>>16)&255,(c>>8)&255,c&255);}
inline int qsFilled(SDL_Renderer* r,Sint16 x,Sint16 y,Sint16 radius,Uint32 c){return filledCircleRGBA(r,x,y,radius,c>>24,(c>>16)&255,(c>>8)&255,c&255);}
inline int qsString(SDL_Renderer* r,Sint16 x,Sint16 y,const char* s,Uint32 c){return stringRGBA(r,x,y,s,c>>24,(c>>16)&255,(c>>8)&255,c&255);}
#define boxColor qsBox
#define roundedBoxColor qsRounded
#define circleColor qsCircle
#define filledCircleColor qsFilled
#define stringColor qsString
