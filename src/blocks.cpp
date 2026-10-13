#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include "drawing.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
// Quentin Blocks, jeu original GPL-3.0-or-later. Aucun asset externe.
int main(int,char**) {
    SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK))return 1;
    auto* w=SDL_CreateWindow("Quentin Blocks",0,0,1280,720,SDL_WINDOW_SHOWN);auto* r=w?SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE):nullptr;if(!r){SDL_Quit();return 2;}
    SDL_RenderSetLogicalSize(r,1280,720);auto* pad=SDL_NumJoysticks()?SDL_JoystickOpen(0):nullptr;
    float x=640,y=520,vx=250,vy=-280,paddle=640;int score=0;bool run=true,paused=true;std::vector<bool> bricks(50,true);Uint32 before=SDL_GetTicks();
    while(run){Uint32 now=SDL_GetTicks();float dt=std::min(0.03f,float(now-before)/1000);before=now;SDL_Event e;
        while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)run=false;if(e.type==SDL_KEYDOWN){if(e.key.keysym.sym==SDLK_ESCAPE)run=false;if(e.key.keysym.sym==SDLK_SPACE)paused=!paused;}if(e.type==SDL_JOYBUTTONDOWN){if(e.jbutton.button==6)run=false;if(e.jbutton.button==0)paused=!paused;}}
        const Uint8* keys=SDL_GetKeyboardState(nullptr);float axis=pad?SDL_JoystickGetAxis(pad,0)/32768.f:0;
        if(keys[SDL_SCANCODE_LEFT])axis=-1;if(keys[SDL_SCANCODE_RIGHT])axis=1;if(std::abs(axis)>.15f)paddle+=axis*650*dt;paddle=std::max(100.f,std::min(1180.f,paddle));
        if(!paused){x+=vx*dt;y+=vy*dt;if((x<20&&vx<0)||(x>1260&&vx>0))vx=-vx;if(y<75&&vy<0)vy=-vy;
            if(y>635&&y<663&&std::abs(x-paddle)<110&&vy>0){vy=-std::abs(vy);vx=(x-paddle)*4;}
            for(int k=0;k<50;k++){int bx=70+k%10*115,by=110+k/10*55;if(bricks[k]&&x>bx-8&&x<bx+108&&y>by-8&&y<by+43){bricks[k]=false;vy=-vy;score++;break;}}
            if(y>740 || score==50){paused=true;x=640;y=520;vx=250;vy=-280;if(score==50){score=0;std::fill(bricks.begin(),bricks.end(),true);}}}
        SDL_SetRenderDrawColor(r,10,15,27,255);SDL_RenderClear(r);stringColor(r,40,30,("QUENTIN BLOCKS | score: "+std::to_string(score)+" | X/Space: pause | Options/Escape: quitter").c_str(),0xc4f4eaff);
        for(int k=0;k<50;k++)if(bricks[k]){int bx=70+k%10*115,by=110+k/10*55;roundedBoxColor(r,bx,by,bx+100,by+35,6,0x5ce3caff-Uint32(k/10)*0x09090000);}
        roundedBoxColor(r,int(paddle)-100,650,int(paddle)+100,668,8,0x99baffff);filledCircleColor(r,int(x),int(y),10,0xffffffff);if(paused)stringColor(r,500,420,"X pour jouer / reprendre",0xffffffff);
        SDL_RenderPresent(r);SDL_Delay(16);
    }if(pad)SDL_JoystickClose(pad);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
