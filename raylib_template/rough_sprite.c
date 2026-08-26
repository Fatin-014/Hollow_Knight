#include "raylib.h"

int main()
{
    InitWindow(1000,1000,"Hollow Knights");
    Texture2D knight=LoadTexture("assets/Attack 1.png");
    float posx,posy,dt,speed;
    posx=440;
    speed=300;
    posy=480;
    int direction=1;
    int currentFrame=0;
    while (!WindowShouldClose())
    {
        dt=GetFrameTime();       
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Rectangle sprite={0,0,128*direction,128};
        Rectangle ground={0,600,1000,400};
        DrawRectangleRec(ground,RED);
        DrawTexturePro(knight,sprite,(Rectangle){posx,posy,120,120},(Vector2){0,0},0,WHITE);
        
        if(IsKeyDown(KEY_RIGHT))
        {
        posx+=(speed*dt);
        direction=1;
        }
        if(IsKeyDown(KEY_LEFT))
        {
        posx-=(speed*dt);
        direction=-1;
        }
        EndDrawing();
    }
    CloseWindow();
    
    return 0;
}