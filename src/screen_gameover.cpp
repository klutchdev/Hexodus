/**********************************************************************************************
*
*   Hexodus - Game Over Screen
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
static float screenTime = 0.0f;
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Game Over Screen Functions Definition
//----------------------------------------------------------------------------------

// Game Over Screen Initialization logic
void InitGameOverScreen(void)
{
    screenTime = 0.0f;
    finishScreen = 0;
}

// Game Over Screen Update logic
void UpdateGameOverScreen(void)
{
    screenTime += GetFrameTime();

    // Let the moment land before allowing the skip back to title
    if (screenTime > 2.0f && (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP)))
    {
        finishScreen = 1;   // back to TITLE
        SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
        PlaySound(fxSelection);
    }
}

// Game Over Screen Draw logic
void DrawGameOverScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

    // Text fades in over the first couple of seconds
    float fadeIn = fminf(screenTime / 2.0f, 1.0f);

    const char *lineOne = "MISSION COMPLETE!";
    const char *lineTwo = "Now you can get home in time for the Raylib game jam!";

    DrawText(lineOne, (screenWidth - MeasureText(lineOne, 60)) / 2, 280, 60, Fade(LIGHTGRAY, fadeIn));
    DrawText(lineTwo, (screenWidth - MeasureText(lineTwo, 20)) / 2, 370, 20, Fade(GRAY, fadeIn));

    if (screenTime > 2.0f)
    {
        float pulse = 0.65f + 0.35f * sinf(screenTime * 3.0f);
        const char *prompt = "PRESS ENTER or TAP SCREEN";
        DrawText(prompt, (screenWidth - MeasureText(prompt, 20)) / 2, 520, 20, Fade(SKYBLUE, pulse));
    }
}

// Game Over Screen Unload logic
void UnloadGameOverScreen(void)
{
}

// Game Over Screen should finish?
int FinishGameOverScreen(void)
{
    return finishScreen;
}
