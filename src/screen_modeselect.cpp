/**********************************************************************************************
*
*   Hexodus - Mode Select Screen
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
#include "utils.hpp"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static float screenTime = 0.0f;
static int finishScreen = 0;

static const Rectangle campaignButtonRect  = { 160.0f, 260.0f, 400.0f, 80.0f };
static const Rectangle unlimitedButtonRect = { 160.0f, 380.0f, 400.0f, 80.0f };

//----------------------------------------------------------------------------------
// Mode Select Screen Functions Definition
//----------------------------------------------------------------------------------

// Mode Select Screen Initialization logic
void InitModeSelectScreen(void)
{
    screenTime = 0.0f;
    finishScreen = 0;
}

// Mode Select Screen Update logic
void UpdateModeSelectScreen(void)
{
    screenTime += GetFrameTime();

    if (screenTime < 0.5f)
    {
        return;   // grace period so the title tap doesnt click a mode
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (CheckCollisionPointRec(GetMousePosition(), campaignButtonRect))
        {
            unlimitedMode = false;
            finishScreen = 1;
            SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
            PlaySound(fxSelection);
        }

        if (CheckCollisionPointRec(GetMousePosition(), unlimitedButtonRect))
        {
            unlimitedMode = true;
            finishScreen = 1;
            SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
            PlaySound(fxSelection);
        }
    }
}

// Mode Select Screen Draw logic
void DrawModeSelectScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), backgroundColor);

    const char *heading = "SELECT MISSION TYPE";
    DrawText(heading, (screenWidth - MeasureText(heading, 30)) / 2, 160, 30, LIGHTGRAY);

    bool overCampaign  = CheckCollisionPointRec(GetMousePosition(), campaignButtonRect);
    bool overUnlimited = CheckCollisionPointRec(GetMousePosition(), unlimitedButtonRect);

    DrawRectangleRec(campaignButtonRect, overCampaign ? DARKBLUE : hudBackgroundColor);
    DrawRectangleLinesEx(campaignButtonRect, 2.0f, overCampaign ? SKYBLUE : screenBorderColor);
    DrawText("CAMPAIGN", (int)(campaignButtonRect.x + 20), (int)(campaignButtonRect.y + 15), 30, overCampaign ? SKYBLUE : LIGHTGRAY);
    DrawText("Clean all the junk and get back home!", (int)(campaignButtonRect.x + 20), (int)(campaignButtonRect.y + 50), 18, GRAY);

    DrawRectangleRec(unlimitedButtonRect, overUnlimited ? DARKBLUE : hudBackgroundColor);
    DrawRectangleLinesEx(unlimitedButtonRect, 2.0f, overUnlimited ? SKYBLUE : screenBorderColor);
    DrawText("ENDLESS", (int)(unlimitedButtonRect.x + 20), (int)(unlimitedButtonRect.y + 15), 30, overUnlimited ? SKYBLUE : LIGHTGRAY);
    DrawText("It...never...stops! ahh!", (int)(unlimitedButtonRect.x + 20), (int)(unlimitedButtonRect.y + 50), 18, GRAY);
}

// Mode Select Screen Unload logic
void UnloadModeSelectScreen(void)
{
}

// Mode Select Screen should finish?
int FinishModeSelectScreen(void)
{
    return finishScreen;
}
