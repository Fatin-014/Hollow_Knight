#include "raylib.h"
#include <math.h>
#include <stdio.h>

// Screen Dimensions
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// Game Parameters
#define GRAVITY 1200.0f
#define JUMP_FORCE -550.0f
#define MOVE_SPEED 300.0f
#define DASH_SPEED 800.0f
#define DASH_DURATION 0.15f
#define DASH_COOLDOWN 0.8f
#define GROUND_LEVEL 600.0f

// Collision Box Sizes
#define PLAYER_COLLISION_WIDTH 40.0f
#define PLAYER_COLLISION_HEIGHT 80.0f
#define ENEMY_COLLISION_WIDTH 45.0f
#define ENEMY_COLLISION_HEIGHT 75.0f

// Combat Hitbox Parameters
#define ATTACK_RANGE 60.0f
#define ATTACK_HEIGHT 50.0f

// Game States
typedef enum GameState {
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_VICTORY
} GameState;

// Animation States
typedef enum PlayerAnimState {
    PLAYER_ANIM_IDLE,
    PLAYER_ANIM_RUN,
    PLAYER_ANIM_ATTACK,
    PLAYER_ANIM_DEATH
} PlayerAnimState;

typedef enum EnemyAnimState {
    ENEMY_ANIM_IDLE,
    ENEMY_ANIM_RUN,
    ENEMY_ANIM_ATTACK,
    ENEMY_ANIM_DEATH
} EnemyAnimState;

// Animation Sets
typedef struct PlayerAnimSet {
    Texture2D idle;
    Texture2D run;
    Texture2D attack;
    Texture2D death;
    int idleFrames;
    int runFrames;
    int attackFrames;
    int deathFrames;
} PlayerAnimSet;

typedef struct EnemyAnimSet {
    Texture2D idle;
    Texture2D run;
    Texture2D attack;
    Texture2D death;
    int idleFrames;
    int runFrames;
    int attackFrames;
    int deathFrames;
} EnemyAnimSet;

// Entities
typedef struct Player {
    Rectangle rec;
    Vector2 speed;
    bool isGrounded;
    bool isFacingLeft;
    int health;
    int maxHealth;
} Player;

typedef struct Enemy {
    Rectangle rec;
    Vector2 speed;
    bool active;
    bool isFacingLeft;
    int health;
    int maxHealth;
    EnemyAnimState animState;
    float animTimer;
    int currentFrame;
} Enemy;

// Global State
static GameState currentState = STATE_MENU;
static int currentLevel = 1;
static bool debugMode = false;

// Function to calculate aligned attack hitbox based on facing direction
Rectangle GetPlayerAttackHitbox(Rectangle playerRec, bool isFacingLeft) {
    Rectangle attackBox;
    attackBox.width = ATTACK_RANGE;
    attackBox.height = ATTACK_HEIGHT;
    
    // Vertically center the attack box relative to the player
    attackBox.y = playerRec.y + (playerRec.height / 2.0f) - (ATTACK_HEIGHT / 2.0f);

    if (isFacingLeft) {
        // Project to the LEFT of the player's left boundary
        attackBox.x = playerRec.x - ATTACK_RANGE;
    } else {
        // Project to the RIGHT of the player's right boundary
        attackBox.x = playerRec.x + playerRec.width;
    }

    return attackBox;
}

// Spawn Enemies for Current Level
void SpawnLevelEnemies(Enemy enemies[], int *count, int level) {
    *count = level * 2;
    for (int i = 0; i < *count; i++) {
        enemies[i].rec = (Rectangle){
            400.0f + (i * 250.0f), 
            GROUND_LEVEL - ENEMY_COLLISION_HEIGHT, 
            ENEMY_COLLISION_WIDTH, 
            ENEMY_COLLISION_HEIGHT
        };
        enemies[i].speed = (Vector2){80.0f + (level * 10.0f), 0.0f};
        enemies[i].active = true;
        enemies[i].isFacingLeft = true;
        enemies[i].health = 2;
        enemies[i].maxHealth = 2;
        enemies[i].animState = ENEMY_ANIM_RUN;
        enemies[i].animTimer = 0.0f;
        enemies[i].currentFrame = 0;
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "2D Platformer - Fixed Collision Alignment");
    SetTargetFPS(60);

    // Initialize Player
    Player player = {0};
    player.rec = (Rectangle){100.0f, GROUND_LEVEL - PLAYER_COLLISION_HEIGHT, PLAYER_COLLISION_WIDTH, PLAYER_COLLISION_HEIGHT};
    player.speed = (Vector2){0.0f, 0.0f};
    player.isGrounded = true;
    player.isFacingLeft = false;
    player.health = 5;
    player.maxHealth = 5;

    // Movement & Combat Mechanics Timers
    bool isAttacking = false;
    float attackTimer = 0.0f;
    const float attackDuration = 0.3f;
    bool attackHasHit[10] = { false }; // Prevents multi-hits per swing

    bool isDashing = false;
    float dashTimer = 0.0f;
    float dashCooldownTimer = 0.0f;

    bool isInvincible = false;
    float invincibilityTimer = 0.0f;

    // Initialize Enemies
    Enemy enemies[10] = {0};
    int enemyCount = 0;
    SpawnLevelEnemies(enemies, &enemyCount, currentLevel);

    // Animations Placeholder Setup
    PlayerAnimSet playerAnim = {0};
    PlayerAnimState playerAnimState = PLAYER_ANIM_IDLE;
    float playerAnimTimer = 0.0f;
    int playerCurrentFrame = 0;

    EnemyAnimSet enemyAnim = {0};

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Toggle Debug Mode
        if (IsKeyPressed(KEY_F3)) {
            debugMode = !debugMode;
        }

        // ==================== UPDATE LOGIC ====================
        switch (currentState) {
            case STATE_MENU:
                if (IsKeyPressed(KEY_ENTER)) {
                    currentState = STATE_GAMEPLAY;
                }
                break;

            case STATE_GAMEPLAY:
                // --- Dash Cooldown Update ---
                if (dashCooldownTimer > 0.0f) dashCooldownTimer -= deltaTime;
                if (invincibilityTimer > 0.0f) {
                    invincibilityTimer -= deltaTime;
                    if (invincibilityTimer <= 0.0f) isInvincible = false;
                }

                // --- Player Inputs & Movement ---
                if (!isDashing) {
                    player.speed.x = 0.0f;
                    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                        player.speed.x = -MOVE_SPEED;
                        player.isFacingLeft = true;
                    }
                    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                        player.speed.x = MOVE_SPEED;
                        player.isFacingLeft = false;
                    }

                    // Jump
                    if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && player.isGrounded) {
                        player.speed.y = JUMP_FORCE;
                        player.isGrounded = false;
                    }

                    // Trigger Dash
                    if (IsKeyPressed(KEY_LEFT_SHIFT) && dashCooldownTimer <= 0.0f) {
                        isDashing = true;
                        dashTimer = DASH_DURATION;
                        dashCooldownTimer = DASH_COOLDOWN;
                    }

                    // Trigger Attack
                    if (IsKeyPressed(KEY_J) && !isAttacking) {
                        isAttacking = true;
                        attackTimer = attackDuration;
                        for (int i = 0; i < enemyCount; i++) attackHasHit[i] = false;
                    }
                } else {
                    // Dash Execution
                    player.speed.x = player.isFacingLeft ? -DASH_SPEED : DASH_SPEED;
                    player.speed.y = 0.0f; // Freeze gravity during dash
                    dashTimer -= deltaTime;
                    if (dashTimer <= 0.0f) isDashing = false;
                }

                // Gravity & Movement Integration
                if (!isDashing) {
                    player.speed.y += GRAVITY * deltaTime;
                }

                player.rec.x += player.speed.x * deltaTime;
                player.rec.y += player.speed.y * deltaTime;

                // Ground Collision
                if (player.rec.y + player.rec.height >= GROUND_LEVEL) {
                    player.rec.y = GROUND_LEVEL - player.rec.height;
                    player.speed.y = 0.0f;
                    player.isGrounded = true;
                }

                // Screen Boundaries
                if (player.rec.x < 0) player.rec.x = 0;
                if (player.rec.x + player.rec.width > SCREEN_WIDTH) player.rec.x = SCREEN_WIDTH - player.rec.width;

                // --- Attack Logic & Hitbox Registration ---
                if (isAttacking) {
                    attackTimer -= deltaTime;
                    Rectangle attackHitbox = GetPlayerAttackHitbox(player.rec, player.isFacingLeft);

                    for (int i = 0; i < enemyCount; i++) {
                        if (enemies[i].active && !attackHasHit[i]) {
                            if (CheckCollisionRecs(attackHitbox, enemies[i].rec)) {
                                enemies[i].health--;
                                attackHasHit[i] = true;

                                // Apply Knockback
                                enemies[i].rec.x += player.isFacingLeft ? -30.0f : 30.0f;

                                if (enemies[i].health <= 0) {
                                    enemies[i].active = false;
                                }
                            }
                        }
                    }

                    if (attackTimer <= 0.0f) {
                        isAttacking = false;
                    }
                }

                // --- Enemy AI & Collision Updates ---
                bool allDefeated = true;
                for (int i = 0; i < enemyCount; i++) {
                    if (!enemies[i].active) continue;
                    allDefeated = false;

                    // Simple Patrol AI
                    enemies[i].rec.x += (enemies[i].isFacingLeft ? -enemies[i].speed.x : enemies[i].speed.x) * deltaTime;
                    if (enemies[i].rec.x <= 200.0f) enemies[i].isFacingLeft = false;
                    if (enemies[i].rec.x >= SCREEN_WIDTH - 200.0f) enemies[i].isFacingLeft = true;

                    // Enemy vs Player Body Collision
                    if (!isInvincible && CheckCollisionRecs(player.rec, enemies[i].rec)) {
                        player.health--;
                        isInvincible = true;
                        invincibilityTimer = 1.0f; // 1s iframe

                        if (player.health <= 0) {
                            // Restart Level on death
                            player.health = player.maxHealth;
                            player.rec.x = 100.0f;
                            SpawnLevelEnemies(enemies, &enemyCount, currentLevel);
                        }
                    }
                }

                // Level Clear / Transition
                if (allDefeated) {
                    currentLevel++;
                    if (currentLevel > 3) {
                        currentState = STATE_VICTORY;
                    } else {
                        SpawnLevelEnemies(enemies, &enemyCount, currentLevel);
                    }
                }
                break;

            case STATE_VICTORY:
                if (IsKeyPressed(KEY_ENTER)) {
                    currentLevel = 1;
                    player.health = player.maxHealth;
                    SpawnLevelEnemies(enemies, &enemyCount, currentLevel);
                    currentState = STATE_MENU;
                }
                break;
        }

        // ==================== DRAW LOGIC ====================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentState) {
            case STATE_MENU:
                DrawText("2D ACTION PLATFORMER", SCREEN_WIDTH / 2 - 180, 250, 30, DARKGRAY);
                DrawText("PRESS ENTER TO START", SCREEN_WIDTH / 2 - 140, 350, 20, GRAY);
                break;

            case STATE_GAMEPLAY:
                // Draw Ground
                DrawRectangle(0, (int)GROUND_LEVEL, SCREEN_WIDTH, SCREEN_HEIGHT - (int)GROUND_LEVEL, DARKGRAY);

                // Draw Enemies
                for (int i = 0; i < enemyCount; i++) {
                    if (enemies[i].active) {
                        DrawRectangleRec(enemies[i].rec, MAROON);
                        
                        // Enemy Health Bar
                        DrawRectangle((int)enemies[i].rec.x, (int)enemies[i].rec.y - 10, (int)ENEMY_COLLISION_WIDTH, 5, RED);
                        DrawRectangle((int)enemies[i].rec.x, (int)enemies[i].rec.y - 10, (int)(ENEMY_COLLISION_WIDTH * ((float)enemies[i].health / enemies[i].maxHealth)), 5, GREEN);
                    }
                }

                // Draw Player (Flicker if invincible)
                if (!isInvincible || ((int)(invincibilityTimer * 10) % 2 == 0)) {
                    Color playerColor = isDashing ? SKYBLUE : BLUE;
                    DrawRectangleRec(player.rec, playerColor);
                }

                // Draw Player Attack Visualizer
                if (isAttacking) {
                    Rectangle attackHitbox = GetPlayerAttackHitbox(player.rec, player.isFacingLeft);
                    DrawRectangleRec(attackHitbox, Fade(RED, 0.5f));
                }

                // --- Debug Hitbox Visualizer Overlay ---
                if (debugMode) {
                    // Player Hitbox (Green)
                    DrawRectangleLinesEx(player.rec, 2.0f, GREEN);
                    
                    // Attack Hitbox (Red Outline)
                    if (isAttacking) {
                        Rectangle attackHitbox = GetPlayerAttackHitbox(player.rec, player.isFacingLeft);
                        DrawRectangleLinesEx(attackHitbox, 2.0f, RED);
                    }

                    // Enemy Hitboxes (Yellow)
                    for (int i = 0; i < enemyCount; i++) {
                        if (enemies[i].active) {
                            DrawRectangleLinesEx(enemies[i].rec, 2.0f, YELLOW);
                        }
                    }

                    DrawText("DEBUG MODE ACTIVE (F3 to toggle)", 10, 40, 16, RED);
                }

                // Draw HUD
                DrawText(TextFormat("HP: %d/%d", player.health, player.maxHealth), 10, 10, 20, BLACK);
                DrawText(TextFormat("LEVEL: %d", currentLevel), SCREEN_WIDTH - 120, 10, 20, BLACK);
                break;

            case STATE_VICTORY:
                DrawText("VICTORY! ALL LEVELS CLEARED!", SCREEN_WIDTH / 2 - 220, 250, 30, GOLD);
                DrawText("PRESS ENTER TO RETURN TO MENU", SCREEN_WIDTH / 2 - 180, 350, 20, GRAY);
                break;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}