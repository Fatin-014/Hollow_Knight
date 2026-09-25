#ifndef CONSTANTS_H
#define CONSTANTS_H

// Screen & Layout Macros
#define SC_WIDTH            1500
#define SC_HEIGHT           1000
#define GROUND_Y            800
#define TARGET_FPS          60

// Player Dimensions & Start State
#define PLAYER_WIDTH        128
#define PLAYER_HEIGHT       128
#define PLAYER_START_X      500.0f
#define PLAYER_START_Y      (GROUND_Y - PLAYER_HEIGHT)

#define ENEMY_WIDTH        128
#define ENEMY_HEIGHT       128
#define ENEMY_START_X      800.0f
#define ENEMY_START_Y      (GROUND_Y - ENEMY_HEIGHT)

#define DEATHZONE_START_X (SC_WIDTH/2)
#define DEATHZONE_START_Y GROUND_Y
#define DEATHZONE_WIDTH 512
#define DEATHZONE_HEIGHT 16


#define PLATFORM_X      600
#define PLATFORM_Y      400
#define PLATFORM_WIDTH  200
#define PLATFORM_HEIGHT 20

// Physics & Movement Parameters
#define GRAVITY_ACCEL_Y     1200.0f // Gravity force (pixels/s^2)
#define MOVE_ACCEL          1500.0f // Horizontal acceleration (pixels/s^2)
#define MAX_MOVE_SPEED      500.0f  // Terminal horizontal velocity (pixels/s)
#define FRICTION_COEFF      8.0f    // Drag rate when no movement keys are held
#define JUMP_IMPULSE        -600.0f // Upward velocity applied on jump (pixels/s)

// Visual Macros
#define COLOR_BG            GetColor(0x283D3BFF)  // DARK SLATE GRAY
#define COLOR_GROUND        GetColor(0x197278FF)  // STORMY TEAL
#define COLOR_PLAYER        GetColor(0xEDDDD4FF)  // POWDER PETAL
#define COLOR_ENEMY         GetColor(0xD2F898FF)  // LIME CREAM
#define COLOR_TEXT          GetColor(0xC44536FF)  // TOMATO JAM
#define COLOR_PLATFORM      GetColor(0x6A4C93FF)
#endif // CONFIG_H