/**********************************************************************************************
*
*   Hexodus Constants - every tunable in one place
*
*   Balance the game from this file: costs, speeds, spawn rates, colors, layout rects
*
*   Built by Kyle aka Klutch @ quicks games - special thanks to wubs @ wubs games
*
**********************************************************************************************/
#pragma once
#include "raylib.h"
#include <vector>

// Window ================//
inline const char *gameTitle    = "Hexodus";
inline const int screenWidth    = 720;
inline const int screenHeight   = 720;
inline const int targetFPS      = 60;

// Styling ================//
inline const Color     backgroundColor         = BLACK;
inline const Color     upgradeButtonBgColor    = DARKPURPLE;
inline const Color     upgradeButtonTextColor  = PINK;
inline const Color     upgradePanelLockedColor = { 24, 24, 24, 255 };
inline const Color     laserPink  = { 255, 80, 180, 255 };    // the beams - hot pink, obviously
inline const Color     trashRed   = { 255, 105, 105, 255 };
inline const Color     trashGreen = { 105, 255, 140, 255 };
inline const Color     trashBlue  = { 105, 170, 255, 255 };

inline const std::vector<Color> thrusterColors = 
{
    { 13,  13,  13,  255 },   // ash
    { 19,  19,  19,  255 },   // ash
    { 255, 230, 120, 255 },   // yellow
    { 255, 180,  60, 255 },   // orange
    { 255, 120,  50, 255 },   // deep orange
    { 230,  70,  40, 255 },   // ember red
};

inline const float hudBarHeight        = 50.0f;                  // top strip holding currency slots and buttons
inline const Color hudBackgroundColor  = { 13, 13, 24, 255 };    // slightly blue-dark, sits on the black playfield
inline const Color screenBorderColor   = { 60, 60, 80, 255 };
inline const float screenBorderWidth   = 4.0f;

inline const int       hudButtonFontSize       = 20;
inline const Rectangle upgradeButtonRect       = { 600.0f, 10.0f, 110.0f, 32.0f };
inline const Rectangle autoNavButtonRect       = { 495.0f, 10.0f, 95.0f, 32.0f };
inline const Rectangle nukeButtonRect          = { 445.0f, 10.0f, 40.0f, 32.0f };
inline const Rectangle sweepButtonWideRect     = { 395.0f, 10.0f, 85.0f, 32.0f };
inline const int       nukeCost                = 1000;                             // hex points per mega laser
inline const float     nukeWaveSpeed           = 400.0f;                           // how fast the blast ring expands, px/s
inline const float     nukeWaveMaxRadius       = 520.0f;                           // past the screen corners

inline const Rectangle upgradePanelRect    = { 70.0f, 50.0f, screenHeight - 120.0f, screenWidth - 100.0f };
inline const Color     upgradePanelBgColor = { 9, 9, 9, 255 };

inline const Rectangle colorsRowRect       = {  90.0f,  85.0f, 520.0f, 115.0f };
inline const Rectangle firepowerRowRect    = {  90.0f, 215.0f, 520.0f, 115.0f };
inline const Rectangle automationRowRect   = {  90.0f, 345.0f, 520.0f, 115.0f };
inline const Rectangle shipRowRect         = {  90.0f, 475.0f, 520.0f, 115.0f };


// Labels
inline const char * upgradeButtonLabel = "Upgrade";
 
// Tech tree node names for Next
inline const char *weaponNodeNames[6] = 
{ 
  "Faster cooldown I",
  "Faster cooldown II",
  "Laser power I",
  "Laser power II",
  "Chain laser I",
  "Chain laser II" 
};

inline const char *mobilityNodeNames[6] = 
{ 
  "Auto navigation",
  "Faster ship I",
  "Faster ship II",
  "Mining radius I",
  "Mining radius II",
  "Mining radius III",
};

inline const char *automationNodeNames[6] = 
{ 
  "Auto catcher I",
  "Auto catcher II",
  "Auto catcher +",
  "Auto catcher ++",
  "Extra ship I",
  "Extra ship II",
};

// Per-node hex prices
inline const int technologyNodeCosts[5] = { 5, 10, 15, 75, 150 };

inline const char *technologyNodeNames[6] =
{
  "Red hex",
  "Green hex",
  "Blue hex",
  "Turret I",
  "Turret II",
  "Mega Laser",
};

// Game params
inline const float asteroidSpawnInterval =   1.0f;
inline const float blockSize             =   4.0f;
inline const float shipHeight            =  16.0f;
inline const float shipWidth             =  64.0f;
inline const float shipSpeed             = 100.0f;
inline const float shipAccelerationRate  =  0.01f;
inline const float shipLaserRadius       = 150.0f;
inline const float shipLaserDamage       =   5.0f;
inline const float shipLaserBeamDuration =   1.0f;
inline const float shipMoveIntervalMin   =   5.0f;
inline const float shipMoveIntervalMax   =  20.0f;
inline const float laserCooldown         =   3.0f;
inline const int   maxAsteroids          =    400;
inline const float asteroidBaseHp        =   5.0f;
inline const float asteroidMinDriftSpeed =   0.5f;
inline const float asteroidMaxDriftSpeed =   8.0f;
