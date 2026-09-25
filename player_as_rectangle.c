#include "raylib.h"
#include "raymath.h"
#include "constants.h"
#include "player.h"
#include "platform.h"

#define SPIKE_FRAME_COUNT 7
#define SPIKE_FRAME_TIME 0.2f

float spikeAnimTimer = 0.0f;
int spikeFrame = 0;

int main(void)
{
    InitWindow(SC_WIDTH, SC_HEIGHT, "PLAYER AS RECTANGLE - MACRO CONSTANTS");
    SetTargetFPS(TARGET_FPS);

    Player player = InitPlayer();
    Platform platform = InitPlatform();
    Texture2D spikes = LoadTexture("assets/platformer_metroidvania asset pack v1.01/miscellaneous sprites/trap_spikes_anim_strip_7.png");

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        float frameWidth = (float)spikes.width / SPIKE_FRAME_COUNT;
        float spikeTileWidth = (float)DEATHZONE_WIDTH / 16.0f;

        spikeAnimTimer += dt;
        if (spikeAnimTimer >= SPIKE_FRAME_TIME)
        {
            spikeAnimTimer -= SPIKE_FRAME_TIME;
            spikeFrame = (spikeFrame + 1) % SPIKE_FRAME_COUNT;
        }

        Rectangle spikeSource = {frameWidth * spikeFrame, 0, frameWidth, -(float)spikes.height};
        Rectangle playerRect = {player.pos.x, player.pos.y, PLAYER_WIDTH, PLAYER_HEIGHT};

        if (player.obostha == Alive)
        {
            UpdatePlayer(&player, dt, platform.rect);
        }
        else
        {
            if (IsKeyPressed(KEY_R))
            {
                player = InitPlayer();
            }
        }

        BeginDrawing();
        ClearBackground(COLOR_BG);

        DrawRectangle(0, GROUND_Y, SC_WIDTH, SC_HEIGHT, GetColor(0x353D2FFF));
        DrawLineEx((Vector2){0, GROUND_Y}, (Vector2){SC_WIDTH, GROUND_Y}, 5.0f, BLACK);

        DrawRectanglePro(playerRect, (Vector2){0.0f, 0.0f}, 0.0f, COLOR_PLAYER);
        DrawRectanglePro(platform.rect, (Vector2){0.0f, 0.0f}, 0.0f, COLOR_PLATFORM);

        for (int i = 0; i < 16; i++)
        {
            Rectangle spikeDest = {
                DEATHZONE_START_X + i * spikeTileWidth,
                DEATHZONE_START_Y - DEATHZONE_HEIGHT,
                spikeTileWidth,
                DEATHZONE_HEIGHT};
            DrawTexturePro(spikes, spikeSource, spikeDest, (Vector2){0, 0}, 0, WHITE);
        }

        DrawText(TextFormat("Vel X: %.1f", player.vel.x), 20, 20, 20, COLOR_TEXT);
        DrawText(TextFormat("Accel X: %.1f", player.accel.x), 20, 50, 20, COLOR_TEXT);
        DrawText(TextFormat("Vel Y: %.1f", player.vel.y), 20, 80, 20, COLOR_TEXT);
        DrawText(TextFormat("Accel Y: %.1f", player.accel.y), 20, 110, 20, COLOR_TEXT);
        DrawText(TextFormat("State: %d", player.obostha), 20, 130, 20, COLOR_TEXT);
        DrawText(TextFormat("JumpState: %d", player.jumpState), 20, 160, 20, COLOR_TEXT);

        if (player.obostha == Dead)
        {
            const char *msg = "Nice try. Press R to start again";
            int fontSize = 40;
            int textWidth = MeasureText(msg, fontSize);
            DrawText(msg, (SC_WIDTH - textWidth) / 2, SC_HEIGHT / 2 - fontSize / 2, fontSize, RED);
        }

        EndDrawing();
    }

    UnloadTexture(spikes);
    CloseWindow();
    return 0;
}