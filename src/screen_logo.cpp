/**********************************************************************************************
*
*   Hexodus - Logo Screen (the raylib logo, but hexagonal - it is called HEXodus after all bahahahaha)
*
*   Altered version of the raylib Advance Game template logo screen
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.hpp"
#include "constants.hpp"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static float stateTime  = 0.0f;    // seconds in the current state - frame counts are useless with uncapped FPS
static int finishScreen = 0;

static int lettersCount = 0;

static float hexRadius  = 16.0f;    // grows from a blinking seed to the full ring
static int state        = 0;        // Logo animation states
static float alpha      = 1.0f;     // Useful for fading

static const float hexFullRadius = 140.0f;
static const float hexThickness  = 14.0f;

//----------------------------------------------------------------------------------
// Logo Screen Functions Definition
//----------------------------------------------------------------------------------

// Logo Screen Initialization logic
void InitLogoScreen(void)
{
    finishScreen = 0;
    stateTime    = 0.0f;
    lettersCount = 0;

    hexRadius = 16.0f;
    state     = 0;
    alpha     = 1.0f;
}

// Logo Screen Update logic
void UpdateLogoScreen(void)
{
    stateTime += GetFrameTime();

    if (state == 0)         // State 0: small hexagon seed blink logic
    {
        if (stateTime >= 1.3f)
        {
            state = 1;
            stateTime = 0.0f;    // Reset timer... will be used later...
        }
    }
    else if (state == 1)    // State 1: the hexagon ring grows to full size
    {
        hexRadius += 120.0f * GetFrameTime();

        if (hexRadius >= hexFullRadius)
        {
            hexRadius = hexFullRadius;
            state = 2;
            stateTime = 0.0f;
        }
    }
    else if (state == 2)    // State 2: "raylib" text-write animation logic
    {
        if (lettersCount < 10)
        {
            if (stateTime >= 0.2f)    // Every 0.2 seconds, one more letter!
            {
                lettersCount++;
                stateTime = 0.0f;
            }
        }
        else    // When all letters have appeared, just fade out everything
        {
            if (stateTime > 3.0f)
            {
                alpha -= 1.2f * GetFrameTime();

                if (alpha <= 0.0f)
                {
                    alpha = 0.0f;
                    finishScreen = 1;    // Jump to next screen
                }
            }
        }
    }
}

// Logo Screen Draw logic
void DrawLogoScreen(void)
{
    Vector2 center = { GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), backgroundColor);

    if (state == 0)         // Draw the blinking hexagon seed
    {
        if (((int)(stateTime / 0.17f)) % 2)
        {
            DrawPoly(center, 6, hexRadius, 0.0f, WHITE);
        }
    }
    else if (state == 1)    // Draw the growing hexagon ring
    {
        DrawPolyLinesEx(center, 6, hexRadius, 0.0f, hexThickness, WHITE);
    }
    else if (state == 2)    // Draw the full ring + "raylib" text-write animation + "powered by"
    {
        DrawPolyLinesEx(center, 6, hexRadius, 0.0f, hexThickness, Fade(WHITE, alpha));

        DrawText(TextSubtext("raylib", 0, lettersCount), (int)center.x - 44, (int)center.y - 15, 50, Fade(WHITE, alpha));

        if (lettersCount > 1)
        {
            DrawText("powered by", (int)(center.x - hexFullRadius), (int)(center.y - hexFullRadius - 27), 20, Fade(DARKGRAY, alpha));
        }
    }
}

// Logo Screen Unload logic
void UnloadLogoScreen(void)
{
}

// Logo Screen should finish?
int FinishLogoScreen(void)
{
    return finishScreen;
}
