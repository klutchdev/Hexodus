/**********************************************************************************************
*
*   Hexodus - Title Screen
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
#include "ecs.hpp"    // for the shared texture handles
#include <cmath>
#include "utils.hpp"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int finishScreen = 0;

//----------------------------------------------------------------------------------
// Title Screen Functions Definition
//----------------------------------------------------------------------------------

// Title Screen Initialization logic
void InitTitleScreen(void)
{
    finishScreen = 0;

    for (int i = 0; i < 70; i++)
    {
        ECS::Entity star = ECS::createEntity();

        ECS::componentsPool<ECS::Star>()    [star] = { UTILS::getRandomFloat(0.0f, 2.0f * PI), UTILS::getRandomFloat(1.0f, 2.0f) };
        ECS::componentsPool<ECS::Position>()[star] = { UTILS::getRandomFloat(0.0f, screenWidth), UTILS::getRandomFloat(0.0f, screenHeight) };
    }
}

// Title Screen Update logic
void UpdateTitleScreen(void)
{
    // Press enter or tap to go to mode select
    if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
    {
        finishScreen = 1;
        SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
        PlaySound(fxSelection);
    }
}

// Title Screen Draw logic
void DrawTitleScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), backgroundColor);

    // Big home planet rising from the bottom-right, spinning very slowly
    float planetSize     = 560.0f;
    float planetRotation = (float)GetTime() * 0.6f;    // degrees per second - framesCounter is useless with uncapped FPS. whoops haha

     for (auto& [entity, star] : ECS::componentsPool<ECS::Star>())
    {
        ECS::Position &position = ECS::getComponent<ECS::Position>(entity);

        // Each star breathes around its own base brightness
        float twinkle = 0.15f + 0.08f * sinf((float)GetTime() * 0.8f + star.twinklePhase);

        DrawRectangleRec({ position.x, position.y, star.size, star.size }, Fade(WHITE, twinkle));
    }

    DrawTexturePro(planetTexture, { 0.0f, 0.0f, (float)planetTexture.width, (float)planetTexture.height }, { screenWidth - 120.0f, screenHeight - 60.0f, planetSize, planetSize }, { planetSize / 2, planetSize / 2 }, planetRotation, Fade(WHITE, 0.9f));

    // A little scattered trash so the title matches the job description
    float trashSpin = (float)GetTime() * 9.0f;

    DrawTexturePro(spaceTrashSprites[2][0], { 0, 0, 12, 12 }, {  90.0f, 420.0f, 36.0f, 36.0f }, { 18, 18 },  trashSpin,        LIGHTGRAY);
    DrawTexturePro(spaceTrashSprites[1][2], { 0, 0,  8,  8 }, { 250.0f, 530.0f, 24.0f, 24.0f }, { 12, 12 }, -trashSpin * 0.7f, trashRed);
    DrawTexturePro(spaceTrashSprites[2][3], { 0, 0, 12, 12 }, { 470.0f, 380.0f, 36.0f, 36.0f }, { 18, 18 },  trashSpin * 0.5f, GRAY);
    DrawTexturePro(spaceTrashSprites[0][1], { 0, 0,  4,  4 }, { 160.0f, 250.0f, 16.0f, 16.0f }, {  8,  8 }, -trashSpin,        trashGreen);
    DrawTexturePro(spaceTrashSprites[1][0], { 0, 0,  8,  8 }, { 600.0f, 180.0f, 24.0f, 24.0f }, { 12, 12 },  trashSpin * 0.8f, trashBlue);

    Vector2 pos = { 20, 10 };
    DrawTextEx(font, gameTitle, pos, font.baseSize*5.0f, 4, LIGHTGRAY);

    const char *tagline = "an idle game for cleaning space trash";
    DrawText(tagline, 24, (int)(10 + font.baseSize*5.0f + 10), 20, GRAY);

    DrawText("PRESS ENTER or TAP SCREEN", 120, 300, 20, SKYBLUE);

    // Credits
    DrawText("built by Kyle @ quicks games  @2026", 20, (int)screenHeight - 55, 16, GRAY);
    DrawText("special thanks to wubs games", 20, (int)screenHeight - 32, 16, GRAY);
}

// Title Screen Unload logic
void UnloadTitleScreen(void)
{
}

// Title Screen should finish?
int FinishTitleScreen(void)
{
    return finishScreen;
}
