#include "raylib.h"

int main(void)
{
    InitWindow(800, 800, "Beast Slayer");
    SetTargetFPS(60);
    int posx,posy,speed;
    posx=400-80;
    posy=800-330;
    float jump=-10,velocity=0;
    float gravity=0.5f;
    bool IsOnGround=true;
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        int size = MeasureText("Welcome Bakc Fatin!",20);
        DrawText("Welcome Back Fatin!",(400-size/2), 150, 20, RED);//height=font_size
        DrawRectangle(0,(800-250),800,250,BLUE);
        if(IsKeyPressed(KEY_SPACE)&&IsOnGround)
        {
        velocity=jump;
        IsOnGround=false;
        }
        if(!IsOnGround)
        {
            velocity+=gravity;
        }
        posy+=(int)velocity;
        if(posy>=470)
        {
            posy=470;
            IsOnGround=true;
        }
        DrawRectangle(posx,posy,80,80,RED);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
