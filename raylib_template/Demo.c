#include "raylib.h"
#include <stdio.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

typedef struct
{
    Vector2 position;
    float width;
    float height;
    float speed;
    int health;
    bool attacking;
} Knight;

typedef struct
{
    Vector2 position;
    float width;
    float height;
    int health;
    bool alive;
} Demon;

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Knight vs Demons");
    SetTargetFPS(60);

    Knight knight = {
        {200, 400},
        60,
        80,
        250,
        100,
        false
    };

    Demon demon = {
        {700, 400},
        60,
        80,
        100,
        true
    };

    Camera2D camera = {0};
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.zoom = 1.0f;

    int demonsKilled = 0;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // --------------------------------------------------
        // KNIGHT MOVEMENT
        // --------------------------------------------------

        knight.position.x += knight.speed * dt;

        // Small vertical movement
        if (IsKeyDown(KEY_W))
            knight.position.y -= knight.speed * dt;

        if (IsKeyDown(KEY_S))
            knight.position.y += knight.speed * dt;

        // --------------------------------------------------
        // ATTACK
        // --------------------------------------------------

        knight.attacking = IsKeyDown(KEY_SPACE);

        // Attack rectangle in front of knight
        Rectangle attackBox = {
            knight.position.x + knight.width,
            knight.position.y + 20,
            60,
            40
        };

        Rectangle demonBox = {
            demon.position.x,
            demon.position.y,
            demon.width,
            demon.height
        };

        // --------------------------------------------------
        // DAMAGE DEMON
        // --------------------------------------------------

        if (knight.attacking && demon.alive)
        {
            if (CheckCollisionRecs(attackBox, demonBox))
            {
                demon.health -= 100;
                knight.attacking = false;

                if (demon.health <= 0)
                {
                    demon.alive = false;
                    demonsKilled++;
                }
            }
        }

        // --------------------------------------------------
        // SPAWN NEXT DEMON
        // --------------------------------------------------

        if (!demon.alive)
        {
            demon.position.x = knight.position.x + 600;
            demon.position.y = 400;
            demon.health = 100;
            demon.alive = true;
        }

        // --------------------------------------------------
        // CAMERA
        // --------------------------------------------------

        camera.target = (Vector2){
            knight.position.x,
            SCREEN_HEIGHT / 2.0f
        };

        // --------------------------------------------------
        // DRAW
        // --------------------------------------------------

        BeginDrawing();

        ClearBackground(SKYBLUE);

        BeginMode2D(camera);

        // Ground
        DrawRectangle(
            (int)(knight.position.x - 1000),
            480,
            5000,
            120,
            DARKGREEN
        );

        // Some background objects
        for (int i = 0; i < 30; i++)
        {
            float x = i * 300;

            DrawRectangle(
                x,
                380,
                30,
                100,
                BROWN
            );

            DrawCircle(
                x + 15,
                350,
                60,
                GREEN
            );
        }

        // --------------------------------------------------
        // KNIGHT
        // --------------------------------------------------

        DrawRectangle(
            (int)knight.position.x,
            (int)knight.position.y,
            (int)knight.width,
            (int)knight.height,
            BLUE
        );

        // Knight head
        DrawCircle(
            (int)(knight.position.x + 30),
            (int)(knight.position.y - 10),
            25,
            LIGHTGRAY
        );

        // Sword
        if (knight.attacking)
        {
            DrawRectangle(
                (int)(knight.position.x + knight.width),
                (int)(knight.position.y + 25),
                70,
                10,
                GRAY
            );

            DrawRectangle(
                (int)(knight.position.x + knight.width + 60),
                (int)(knight.position.y + 15),
                10,
                30,
                DARKGRAY
            );

            // Attack hitbox visualization
            // Remove this later
            DrawRectangleLinesEx(
                attackBox,
                2,
                RED
            );
        }

        // --------------------------------------------------
        // DEMON
        // --------------------------------------------------

        if (demon.alive)
        {
            DrawRectangle(
                (int)demon.position.x,
                (int)demon.position.y,
                (int)demon.width,
                (int)demon.height,
                RED
            );

            // Demon horns
            DrawTriangle(
                (Vector2){
                    demon.position.x + 10,
                    demon.position.y
                },
                (Vector2){
                    demon.position.x + 20,
                    demon.position.y - 25
                },
                (Vector2){
                    demon.position.x + 30,
                    demon.position.y
                },
                DARKPURPLE
            );

            DrawTriangle(
                (Vector2){
                    demon.position.x + 30,
                    demon.position.y
                },
                (Vector2){
                    demon.position.x + 40,
                    demon.position.y - 25
                },
                (Vector2){
                    demon.position.x + 50,
                    demon.position.y
                },
                DARKPURPLE
            );

            // Health bar
            DrawRectangle(
                (int)demon.position.x,
                (int)demon.position.y - 40,
                60,
                8,
                DARKGRAY
            );

            DrawRectangle(
                (int)demon.position.x,
                (int)demon.position.y - 40,
                (int)(60 * demon.health / 100.0f),
                8,
                RED
            );
        }

        EndMode2D();

        // --------------------------------------------------
        // UI
        // --------------------------------------------------

        DrawText(
            "SPACE = ATTACK",
            20,
            20,
            25,
            BLACK
        );

        DrawText(
            TextFormat("Demons killed: %d", demonsKilled),
            20,
            55,
            25,
            BLACK
        );

        DrawText(
            "Knight is moving forward...",
            20,
            90,
            20,
            DARKGRAY
        );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}