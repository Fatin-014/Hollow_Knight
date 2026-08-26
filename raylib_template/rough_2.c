#include "raylib.h"

int main(void)
{
    InitWindow(1000, 1000, "Beast Slayer");
    SetTargetFPS(60);
    float posx,posy,jump,velocity,gravity,dis;
    posx=800;
    posy=670;
    jump=-500,dis=0,velocity=0;
    gravity=600;
    bool IsOnGround=true;
    while (!WindowShouldClose())
    {
        float dt=GetFrameTime();
        float previousy=posy;
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Rectangle ground={0,750,1000,250};//here the pos can be float
        Rectangle platform = {400,600,200,30};
        DrawRectangleRec(platform,GREEN);
        DrawRectangleRec(ground,BLUE);
        if(IsKeyPressed(KEY_SPACE)&&IsOnGround)
        {
        velocity=jump;
        IsOnGround=false;
        }
        if(!IsOnGround)
        {
            dis=velocity*dt+0.5*gravity*dt*dt;
            velocity+=gravity*dt;
        }
        posy+=dis;
        Rectangle player = {posx,posy,80,80};
        if((previousy+player.height<=platform.y)&&(posy+player.height>=platform.y)&&velocity>0&&(player.x+player.width>platform.x)&&(platform.x+platform.width>player.x))
        {
            posy=platform.y-player.height;
            velocity=0;
            dis=0;
            IsOnGround = true;
        }
        if(CheckCollisionRecs(player,ground)&&velocity>0)
        {
            posy=ground.y-player.height;
            IsOnGround=true;
            dis=0;
            velocity=0;
        }
        DrawRectangleRec(player,RED);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}