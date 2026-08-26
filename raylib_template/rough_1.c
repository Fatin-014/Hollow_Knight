#include "raylib.h"

int main(void)
{
    InitWindow(800, 800, "Beast Slayer");
    SetTargetFPS(60);
    float posx,posy,speed;
    posx=400-80;
    posy=800-330;
    speed=300;
    while (!WindowShouldClose())
    {
        float dt=GetFrameTime();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        int size = MeasureText("Welcome Bakc Fatin!",20);
        DrawText("Welcome Back Fatin!",(400-size/2), 150, 20, RED);//height=font_size
        DrawRectangle(0,(800-250),800,250,BLUE);
        if(IsKeyDown(KEY_RIGHT))
        posx+=(speed*dt);
        if(IsKeyDown(KEY_LEFT))
        posx-=(speed*dt);
        if(posx<=0)
        posx=0;
        if(posx>=720)
        posx=720;
        DrawRectangle((int)posx,(int)posy,80,80,RED);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}