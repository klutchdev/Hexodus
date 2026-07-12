/**********************************************************************************************
*
*   Hexodus - Dialog Screen
*
*   Built by Kyle aka Klutch @ quicks games
*
*   Based on the raylib Advance Game template screens
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
**********************************************************************************************/
#include "raylib.h"
#include "screens.hpp"
#include "constants.hpp"
#include <cmath>
#include "utils.hpp"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static float screenTime = 0.0f;   // seconds on this screen - frame counts are useless with uncapped FPS
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Dialog Screen Functions Definition
//----------------------------------------------------------------------------------

// Dialog Screen Initialization logic
void InitDialogScreen(void)
{
    screenTime = 0.0f;
    finishScreen = 0;
}

// Dialog Screen Update logic
void UpdateDialogScreen(void)
{
    screenTime += GetFrameTime();

    // Small grace period so a leftover tap from the title screen doesnt skip the briefing
    if (screenTime > 0.5f && (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)))
    {
        finishScreen = 1;   // GAMEPLAY
        SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
        PlaySound(fxSelection);
    }
}

// Dialog Screen Draw logic
void DrawDialogScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), backgroundColor);

    float inset  = 30.0f;
    float corner = 40.0f;   // bracket arm length

    DrawRectangleLinesEx({ inset, inset, screenWidth - inset*2, screenHeight - inset*2 }, 1.0f, screenBorderColor);

    float left   = inset - 4;
    float right  = screenWidth - inset + 4;
    float top    = inset - 4;
    float bottom = screenHeight - inset + 4;

    DrawLineEx({ left, top },  { left + corner, top },  3.0f, SKYBLUE);   // top-left
    DrawLineEx({ left, top },  { left, top + corner },  3.0f, SKYBLUE);
    DrawLineEx({ right, top }, { right - corner, top }, 3.0f, SKYBLUE);   // top-right
    DrawLineEx({ right, top }, { right, top + corner }, 3.0f, SKYBLUE);
    DrawLineEx({ left, bottom },  { left + corner, bottom },  3.0f, SKYBLUE);   // bottom-left
    DrawLineEx({ left, bottom },  { left, bottom - corner },  3.0f, SKYBLUE);
    DrawLineEx({ right, bottom }, { right - corner, bottom }, 3.0f, SKYBLUE);   // bottom-right
    DrawLineEx({ right, bottom }, { right, bottom - corner }, 3.0f, SKYBLUE);

    const char *dialogLineOne   = "Welcome aboard the Hexodus I.";
    const char *dialogLineTwo   = "Some jerks left all this space trash here,";
    const char *dialogLineThree = "we need you to clean it up.";
    const char *dialogLineFour  = "Click/tap to move your ship, use hex points";
    const char *dialogLineFive  = "to upgrade your ship and fleet.";

    if (unlimitedMode)
    {
        dialogLineTwo   = "We dont know when we can get someone up there.";
        dialogLineThree = "Clear as much trash as you can!";
    }

    DrawText(dialogLineOne,   (screenWidth - MeasureText(dialogLineOne,   30)) / 2, 200, 30, LIGHTGRAY);
    DrawText(dialogLineTwo,   (screenWidth - MeasureText(dialogLineTwo,   20)) / 2, 280, 20, LIGHTGRAY);
    DrawText(dialogLineThree, (screenWidth - MeasureText(dialogLineThree, 20)) / 2, 310, 20, LIGHTGRAY);
    DrawText(dialogLineFour,  (screenWidth - MeasureText(dialogLineFour,  20)) / 2, 370, 20, LIGHTGRAY);
    DrawText(dialogLineFive,  (screenWidth - MeasureText(dialogLineFive,  20)) / 2, 400, 20, LIGHTGRAY);

    // Prompt pulses smoothly
    float pulse = 0.65f + 0.35f * sinf(screenTime * 3.0f);

    DrawText("PRESS ENTER or TAP SCREEN", (screenWidth - MeasureText("PRESS ENTER or TAP SCREEN", 20)) / 2, 520, 20, Fade(SKYBLUE, pulse));
}

// Dialog Screen Unload logic
void UnloadDialogScreen(void)
{
}

// Dialog Screen should finish?
int FinishDialogScreen(void)
{
    return finishScreen;
}
