#include "raylib.h"
#include "raymath.h"
#include <math.h>

#define ABSOLUTE_MAX_ENEMIES 12
#define screenWidth 1280
#define screenHeight 600
#define GROUND_LEVEL 560.0f
#define GRAVITY 900.0f
#define JUMP_FORCE -450.0f
#define PLAYER_SPEED 250.0f
#define INVINCIBILITY_DURATION 1.0f
#define SPRITE_SCALE 3.0f
#define SPRITE_OFFSET_X -30.0f
#define SPRITE_OFFSET_Y 180.0f
#define ENEMY_OFFSET_Y -12.0f
#define FRAME_WIDTH 144.0f
#define FRAME_HEIGHT 144.0f
#define UPDATE_TIME (1.0f / 12.0f)
#define ATTACK_DURATION 0.25f
#define ATTACK_FRAME_WIDTH 144.0f
#define ATTACK_FRAME_HEIGHT 144.0f
#define ENEMY_FRAME_TIME (1.0f / 10.0f) // how fast goblin animation strips advance

typedef enum GameState
{
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_VICTORY
} GameState;

typedef enum EnemyAnimState
{
    ENEMY_ANIM_IDLE,
    ENEMY_ANIM_RUN,
    ENEMY_ANIM_HIT,
    ENEMY_ANIM_ATTACK,
    ENEMY_ANIM_DEATH
} EnemyAnimState;

typedef struct Enemy
{
    Rectangle rec;
    float speed;
    bool active; // still part of the level (updated + drawn)
    bool facingRight;
    float minX;
    float maxX;
    EnemyAnimState animState;
    int currentFrame;
    float frameTimer;
} Enemy;

// holds each goblin animation strip + how many frames it contains, loaded once and shared by every goblin
typedef struct EnemyAnimSet
{
    Texture2D idle;
    int idleFrames;
    Texture2D run;
    int runFrames;
    Texture2D hit;
    int hitFrames;
    Texture2D attack;
    int attackFrames;
    Texture2D death;
    int deathFrames;
} EnemyAnimSet;

typedef struct Player
{
    Rectangle rec;
} Player;

void SpawnLevelEnemies(Enemy enemies[], int level, int *activeCount, float groundLevel, int screenW);

int main()
{
    InitWindow(screenWidth, screenHeight, "HOLLOW KNIGHT");
    SetTargetFPS(60);
    GameState currentState = STATE_MENU;
    int selectedOption = 0;
    float verticalVelocity = 0.0f;
    bool isGrounded = true;
    Player player = {0};
    player.rec = (Rectangle){100.0f, GROUND_LEVEL - 64.0f, 64.0f, 64.0f};

    int playerHealth = 5;
    const int maxPlayerHealth = 5;
    bool isInvincible = false;
    float invincibilityTimer = 0.0f;
    const float invincibilityDuration = INVINCIBILITY_DURATION;
    const int maxFrames = 8;
    int currentFrame = 0;
    float runningTime = 0.0f;
    float updateTime = UPDATE_TIME;
    bool facingRight = true;

    // attack main
    bool isAttacking = false;
    float attackTimer = 0.0f;
    const float attackDuration = ATTACK_DURATION;
    const int attackMaxFrames = 10;
    const float attackFrameWidth = ATTACK_FRAME_WIDTH;
    const float attackFrameHeight = ATTACK_FRAME_HEIGHT;

    int jumpcount = 0; // fix: was uninitialized

    // level up
    int currentLevel = 1;
    int activeEnemyCount = 0;
    Enemy enemies[ABSOLUTE_MAX_ENEMIES] = {0};

    // pic loading
    // fix: copy the goblin files from the asset pack into assets/enemies/goblin/ in the project folder
    //(relative paths so the build works on any machine, not just one with a D:\ drive laid out this way)
    EnemyAnimSet goblinAnim = {0};
    goblinAnim.idle = LoadTexture("assets/enemies/goblin/goblin_idle_anim_strip_4.png");
    goblinAnim.idleFrames = 4;
    goblinAnim.run = LoadTexture("assets/enemies/goblin/goblin_run_anim_strip_6.png");
    goblinAnim.runFrames = 6;
    goblinAnim.hit = LoadTexture("assets/enemies/goblin/goblin_hit_anim_strip_3.png");
    goblinAnim.hitFrames = 3;
    goblinAnim.attack = LoadTexture("assets/enemies/goblin/goblin_attack_anim_strip_4.png");
    goblinAnim.attackFrames = 4;
    goblinAnim.death = LoadTexture("assets/enemies/goblin/goblin_death_anim_strip_6.png");
    goblinAnim.deathFrames = 6;

    Texture2D knightTextureLvl1 = LoadTexture("assets/Run.png");
    Texture2D knightTextureLvl2 = knightTextureLvl1; // fix: was loading Run.png twice into a second GPU texture; reuse the handle until a real lvl2 sprite sheet exists
    Texture2D attackTexture = LoadTexture("assets/Attack 1.png");

    // Level Backgrounds
    Texture2D bgTextureLvl1 = LoadTexture("assets/bg.png");
    Texture2D bgTextureLvl2 = LoadTexture("assets/bg2.png");
    Texture2D bgTextureLvl3 = LoadTexture("assets/bg3.png");
    Texture2D currentKnightTexture = knightTextureLvl1;
    Texture2D currentBgTexture = bgTextureLvl1;

    // level 1 shuru
    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        // main screen
        if (currentState == STATE_MENU)
        {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
                selectedOption = 1;
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
                selectedOption = 0;
            if (IsKeyPressed(KEY_ENTER))
            {
                if (selectedOption == 0)
                    currentState = STATE_GAMEPLAY;
                else if (selectedOption == 1)
                    break;
            }
        }
        else if (currentState == STATE_GAMEPLAY)
        {
            if (playerHealth <= 0)
            {
                if (IsKeyPressed(KEY_R))
                {
                    playerHealth = maxPlayerHealth;
                    currentLevel = 1;
                    currentKnightTexture = knightTextureLvl1;
                    currentBgTexture = bgTextureLvl1;
                    player.rec.x = 100.0f;
                    player.rec.y = GROUND_LEVEL - 64.0f;
                    verticalVelocity = 0.0f;
                    isAttacking = false;
                    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);
                }
            }
            else
            {
                if (isInvincible)
                {
                    invincibilityTimer -= deltaTime;
                    if (invincibilityTimer <= 0.0f)
                        isInvincible = false;
                }
                bool isMoving = false;
                // fix: lock movement while attacking (Hollow Knight-style rooted swing)
                if (!isAttacking)
                {
                    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
                    {
                        player.rec.x += PLAYER_SPEED * deltaTime;
                        facingRight = true;
                        isMoving = true;
                    }
                    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
                    {
                        player.rec.x -= PLAYER_SPEED * deltaTime;
                        facingRight = false;
                        isMoving = true;
                    }
                }
                if (player.rec.x < 0)
                    player.rec.x = 0;
                if (player.rec.x + player.rec.width > screenWidth)
                    player.rec.x = screenWidth - player.rec.width;
                verticalVelocity += GRAVITY * deltaTime;
                player.rec.y += verticalVelocity * deltaTime;
                if (player.rec.y >= GROUND_LEVEL - player.rec.height)
                {
                    player.rec.y = GROUND_LEVEL - player.rec.height;
                    verticalVelocity = 0.0f;
                    isGrounded = true;
                }
                else
                    isGrounded = false;
                if (isGrounded)
                {
                    jumpcount = 0; // grounded hoile 0
                }
                if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W)) && jumpcount <= 1 && !isAttacking)
                {
                    verticalVelocity = JUMP_FORCE;
                    isGrounded = false;
                    jumpcount += 1;
                }
                if (isMoving && isGrounded && !isAttacking)
                {
                    runningTime += deltaTime;
                    if (runningTime >= updateTime)
                    {
                        runningTime = 0.0f;
                        currentFrame++;
                        if (currentFrame >= maxFrames)
                            currentFrame = 0;
                    }
                }
                else if (isGrounded && !isAttacking)
                {
                    currentFrame = 0;
                    runningTime = 0.0f;
                }

                // enemy auto
                for (int i = 0; i < activeEnemyCount; i++)
                {
                    Enemy *e = &enemies[i];
                    if (!e->active)
                        continue;

                    if (e->animState == ENEMY_ANIM_DEATH)
                    {
                        // play the death strip once, then fully remove the enemy
                        e->frameTimer += deltaTime;
                        if (e->frameTimer >= ENEMY_FRAME_TIME)
                        {
                            e->frameTimer = 0.0f;
                            e->currentFrame++;
                            if (e->currentFrame >= goblinAnim.deathFrames)
                                e->active = false;
                        }
                        continue; // dead goblins don't move, attack, or hurt the player
                    }

                    if (e->animState == ENEMY_ANIM_HIT)
                    {
                        // brief reaction pose, then drop into the death anim
                        e->frameTimer += deltaTime;
                        if (e->frameTimer >= ENEMY_FRAME_TIME)
                        {
                            e->frameTimer = 0.0f;
                            e->currentFrame++;
                            if (e->currentFrame >= goblinAnim.hitFrames)
                            {
                                e->animState = ENEMY_ANIM_DEATH;
                                e->currentFrame = 0;
                                e->frameTimer = 0.0f;
                            }
                        }
                        continue;
                    }

                    if (e->animState == ENEMY_ANIM_ATTACK)
                    {
                        // rooted while its attack swing plays out
                        e->frameTimer += deltaTime;
                        if (e->frameTimer >= ENEMY_FRAME_TIME)
                        {
                            e->frameTimer = 0.0f;
                            e->currentFrame++;
                            if (e->currentFrame >= goblinAnim.attackFrames)
                            {
                                e->animState = ENEMY_ANIM_RUN;
                                e->currentFrame = 0;
                                e->frameTimer = 0.0f;
                            }
                        }
                    }
                    else
                    {
                        // patrol (RUN state)
                        e->animState = ENEMY_ANIM_RUN;
                        if (e->facingRight)
                        {
                            e->rec.x += e->speed * deltaTime;
                            if (e->rec.x >= e->maxX)
                                e->facingRight = false;
                        }
                        else
                        {
                            e->rec.x -= e->speed * deltaTime;
                            if (e->rec.x <= e->minX)
                                e->facingRight = true;
                        }
                        e->frameTimer += deltaTime;
                        if (e->frameTimer >= ENEMY_FRAME_TIME)
                        {
                            e->frameTimer = 0.0f;
                            e->currentFrame++;
                            if (e->currentFrame >= goblinAnim.runFrames)
                                e->currentFrame = 0;
                        }
                    }

                    if (!isInvincible && e->animState != ENEMY_ANIM_ATTACK)
                    {
                        float pCenterX = player.rec.x + (player.rec.width / 2.0f);
                        float pCenterY = player.rec.y + (player.rec.height / 2.0f);
                        float eCenterX = e->rec.x + (e->rec.width / 2.0f);
                        float eCenterY = e->rec.y + (e->rec.height / 2.0f);
                        float deltaX = fabsf(pCenterX - eCenterX);
                        float deltaY = fabsf(pCenterY - eCenterY);
                        if (deltaX < 50.0f && deltaY < 45.0f)
                        {
                            playerHealth--;
                            isInvincible = true;
                            invincibilityTimer = invincibilityDuration;
                            if (pCenterX < eCenterX)
                                player.rec.x -= 40.0f;
                            else
                                player.rec.x += 40.0f;
                            verticalVelocity = -200.0f;
                            // play the goblin's own attack animation for the hit it just landed
                            e->animState = ENEMY_ANIM_ATTACK;
                            e->currentFrame = 0;
                            e->frameTimer = 0.0f;
                        }
                    }
                }

                // attack shuru kora
                if ((IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) && !isAttacking)
                {
                    isAttacking = true;
                    attackTimer = attackDuration;
                }
                // attack and damage
                if (isAttacking)
                {
                    attackTimer -= deltaTime;

                    float attackRange = 150.0f;
                    float playerCenterX = player.rec.x + (player.rec.width / 2.0f);
                    Rectangle attackBox = facingRight ? (Rectangle){playerCenterX, player.rec.y - 20.0f, attackRange, player.rec.height + 40.0f}
                                                      : (Rectangle){playerCenterX - attackRange, player.rec.y - 20.0f, attackRange, player.rec.height + 40.0f};

                    for (int i = 0; i < activeEnemyCount; i++)
                    {
                        bool alreadyDying = (enemies[i].animState == ENEMY_ANIM_HIT || enemies[i].animState == ENEMY_ANIM_DEATH);
                        if (enemies[i].active && !alreadyDying && CheckCollisionRecs(attackBox, enemies[i].rec))
                        {
                            enemies[i].animState = ENEMY_ANIM_HIT;
                            enemies[i].currentFrame = 0;
                            enemies[i].frameTimer = 0.0f;
                        }
                    }
                    if (attackTimer <= 0.0f)
                        isAttacking = false;
                }

                // level par korar part
                bool allEnemiesDefeated = true;
                for (int i = 0; i < activeEnemyCount; i++)
                {
                    if (enemies[i].active)
                    {
                        allEnemiesDefeated = false;
                        break;
                    }
                }
                if (allEnemiesDefeated)
                {
                    if (currentLevel >= 3)
                    {
                        currentState = STATE_VICTORY;
                    }
                    else
                    {
                        currentLevel++;
                        player.rec.x = 50.0f;
                        if (currentLevel == 2 && bgTextureLvl2.id != 0)
                            currentBgTexture = bgTextureLvl2;
                        else if (currentLevel == 3 && bgTextureLvl3.id != 0)
                            currentBgTexture = bgTextureLvl3;
                        if (currentLevel >= 2 && knightTextureLvl2.id != 0)
                            currentKnightTexture = knightTextureLvl2;
                        SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);
                    }
                }
            }
        }
        else if (currentState == STATE_VICTORY)
        {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R))
            {
                currentLevel = 1;
                playerHealth = maxPlayerHealth;
                currentKnightTexture = knightTextureLvl1;
                currentBgTexture = bgTextureLvl1;
                player.rec.x = 100.0f;
                player.rec.y = GROUND_LEVEL - 64.0f;
                verticalVelocity = 0.0f;
                SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);
                currentState = STATE_GAMEPLAY;
            }
        }

        BeginDrawing();
        ClearBackground((Color){20, 20, 30, 255});
        if (currentState == STATE_MENU)
        {
            int title_l = MeasureText("HOLLOW KNIGHT", 55);
            DrawText("HOLLOW KNIGHT", (screenWidth - title_l) / 2, 160, 55, GOLD);
            Color opt1Color = (selectedOption == 0) ? YELLOW : GRAY;
            const char *opt1Text = (selectedOption == 0) ? "> Start New Game <" : "Start New Game";
            DrawText(opt1Text, (screenWidth - MeasureText(opt1Text, 28)) / 2, 270, 28, opt1Color);
            Color opt2Color = (selectedOption == 1) ? YELLOW : GRAY;
            const char *opt2Text = (selectedOption == 1) ? "> Exit <" : "Exit";
            DrawText(opt2Text, (screenWidth - MeasureText(opt2Text, 28)) / 2, 320, 28, opt2Color);
        }
        else if (currentState == STATE_GAMEPLAY)
        {
            // background
            if (currentBgTexture.id != 0)
            {
                Rectangle bgSource = {0.0f, 0.0f, (float)currentBgTexture.width, (float)currentBgTexture.height};
                Rectangle bgDest = {0.0f, 0.0f, (float)screenWidth, (float)screenHeight};
                DrawTexturePro(currentBgTexture, bgSource, bgDest, (Vector2){0, 0}, 0.0f, WHITE);
            }
            Color playerTint = (isInvincible && ((int)(invincibilityTimer * 10) % 2 == 0)) ? RED : WHITE;

            // main character er code
            if (isAttacking && attackTexture.id != 0)
            {
                // fix: cycle through attack frames over the swing instead of freezing on frame 3
                float attackProgress = 1.0f - (attackTimer / attackDuration);
                if (attackProgress < 0.0f)
                    attackProgress = 0.0f;
                if (attackProgress >= 1.0f)
                    attackProgress = 0.999f;
                int attackFrame = (int)(attackProgress * attackMaxFrames);
                if (attackFrame >= attackMaxFrames)
                    attackFrame = attackMaxFrames - 1;

                float sourceWidth = facingRight ? attackFrameWidth : -attackFrameWidth;
                Rectangle sourceRec = {attackFrame * attackFrameWidth, 0.0f, sourceWidth, attackFrameHeight};
                float renderWidth = attackFrameWidth * SPRITE_SCALE;
                float renderHeight = attackFrameHeight * SPRITE_SCALE;
                Rectangle destRec = {player.rec.x + SPRITE_OFFSET_X, player.rec.y + player.rec.height - renderHeight + SPRITE_OFFSET_Y, renderWidth, renderHeight};
                DrawTexturePro(attackTexture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, playerTint);
            }
            else if (currentKnightTexture.id != 0)
            {
                float sourceWidth = facingRight ? FRAME_WIDTH : -FRAME_WIDTH;
                // fix: was multiplying by FRAME_HEIGHT; must step across columns using FRAME_WIDTH
                Rectangle sourceRec = {currentFrame * FRAME_WIDTH, 0.0f, sourceWidth, FRAME_HEIGHT};
                float renderWidth = FRAME_WIDTH * SPRITE_SCALE;
                float renderHeight = FRAME_HEIGHT * SPRITE_SCALE;
                Rectangle destRec = {player.rec.x + SPRITE_OFFSET_X, player.rec.y + player.rec.height - renderHeight + SPRITE_OFFSET_Y, renderWidth, renderHeight};
                DrawTexturePro(currentKnightTexture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, playerTint);
            }

            // enemy akaaki
            for (int i = 0; i < activeEnemyCount; i++)
            {
                if (!enemies[i].active)
                    continue;

                Texture2D tex;
                int frameCount;
                switch (enemies[i].animState)
                {
                case ENEMY_ANIM_IDLE:
                    tex = goblinAnim.idle;
                    frameCount = goblinAnim.idleFrames;
                    break;
                case ENEMY_ANIM_RUN:
                    tex = goblinAnim.run;
                    frameCount = goblinAnim.runFrames;
                    break;
                case ENEMY_ANIM_HIT:
                    tex = goblinAnim.hit;
                    frameCount = goblinAnim.hitFrames;
                    break;
                case ENEMY_ANIM_ATTACK:
                    tex = goblinAnim.attack;
                    frameCount = goblinAnim.attackFrames;
                    break;
                case ENEMY_ANIM_DEATH:
                    tex = goblinAnim.death;
                    frameCount = goblinAnim.deathFrames;
                    break;
                default:
                    tex = goblinAnim.idle;
                    frameCount = goblinAnim.idleFrames;
                    break;
                }

                if (tex.id != 0 && frameCount > 0)
                {
                    float frameW = (float)tex.width / (float)frameCount;
                    float frameH = (float)tex.height;
                    int frame = enemies[i].currentFrame % frameCount; // clamp in case a strip is shorter than expected
                    float srcW = enemies[i].facingRight ? frameW : -frameW;
                    Rectangle src = {frame * frameW, 0.0f, srcW, frameH};
                    Rectangle dest = {enemies[i].rec.x, enemies[i].rec.y + ENEMY_OFFSET_Y, enemies[i].rec.width, enemies[i].rec.height};
                    DrawTexturePro(tex, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
                }
            }

            DrawText(TextFormat("LEVEL %d/3", currentLevel), 10, 10, 22, YELLOW);
            for (int i = 0; i < maxPlayerHealth; i++)
            {
                Color heartColor = (i < playerHealth) ? RED : DARKGRAY;
                DrawRectangle(160 + (i * 25), 10, 20, 20, heartColor);
                DrawRectangleLines(160 + (i * 25), 10, 20, 20, WHITE);
            }
            if (playerHealth <= 0)
            {
                DrawText("GAME OVER!!Press R to Restart", screenWidth / 2 - 200, screenHeight / 2, 28, RED);
            }
        }
        else if (currentState == STATE_VICTORY)
        {
            const char *winText = "VICTORY! YOU CLEARED ALL 3 LEVELS!";
            int winWidth = MeasureText(winText, 32);
            DrawText(winText, (screenWidth - winWidth) / 2, 220, 32, GOLD);
            const char *subText = "Press ENTER or R to Play Again";
            int subWidth = MeasureText(subText, 20);
            DrawText(subText, (screenWidth - subWidth) / 2, 300, 20, RAYWHITE);
        }
        EndDrawing();
    }

    UnloadTexture(bgTextureLvl1);
    UnloadTexture(bgTextureLvl2);
    UnloadTexture(bgTextureLvl3);
    UnloadTexture(knightTextureLvl1);
    // fix: knightTextureLvl2 shares the same GPU handle as knightTextureLvl1 now, so do NOT unload it a second time (double free)
    UnloadTexture(goblinAnim.idle);
    UnloadTexture(goblinAnim.run);
    UnloadTexture(goblinAnim.hit);
    UnloadTexture(goblinAnim.attack);
    UnloadTexture(goblinAnim.death);
    UnloadTexture(attackTexture);
    CloseWindow();
    return 0;
}

void SpawnLevelEnemies(Enemy enemies[], int level, int *activeCount, float groundLevel, int screenW)
{
    *activeCount = 6 + (level - 1) * 2;
    if (*activeCount > ABSOLUTE_MAX_ENEMIES)
        *activeCount = ABSOLUTE_MAX_ENEMIES;
    float speedBoost = (level - 1) * 30.0f;
    float zoneMinX[3] = {300.0f, 600.0f, 900.0f}; // enemy er norar jayga
    float zoneMaxX[3] = {500.0f, 800.0f, 1150.0f};
    for (int i = 0; i < *activeCount; i++)
    {
        int groupIndex = i % 3;
        float minX = zoneMinX[groupIndex];
        float maxX = zoneMaxX[groupIndex];
        float spawnOffset = (i / 3) * 60.0f;
        float startX = minX + 20.0f + spawnOffset;
        if (startX > maxX - 64.0f)
            startX = maxX - 64.0f;
        float enemySpeed = (100.0f + speedBoost) + ((i % 2) * 20.0f);
        bool startFacingRight = (i % 2 == 0);
        enemies[i] = (Enemy){(Rectangle){startX, groundLevel - 64.0f, 64.0f, 64.0f},
                             enemySpeed,
                             true,
                             startFacingRight,
                             minX,
                             maxX,
                             ENEMY_ANIM_RUN,
                             0,
                             0.0f};
    }
    (void)screenW; // currently unused, kept for future spawn logic that scales with screen width
}