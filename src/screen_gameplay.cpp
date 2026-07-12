/**********************************************************************************************
*
*   Hexodus - Gameplay Screen
*
*   Built by Kyle aka Klutch @ quicks games
*
*   Based on the raylib Advance Game template screens
*   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.hpp"
#include "ecs.hpp"
#include "particles.hpp"
#include "utils.hpp"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int finishScreen = 0;

static bool paused = false;   // ESC menu open - game frozen while true

static const Rectangle quitToMenuButtonRect = { 210.0f, 300.0f, 300.0f, 60.0f };
static const Rectangle musicToggleButtonRect = { 210.0f, 480.0f, 300.0f, 60.0f };


#if !defined(PLATFORM_WEB)
    static const Rectangle exitGameButtonRect   = { 210.0f, 390.0f, 300.0f, 60.0f };
#endif

static bool musicEnabled = true;   // pause menu toggle - pauses/resumes the music stream

// Campaign ending cinematic: 0 = normal play, 1 = wait for the dust to settle,
// 2 = fleet flies home, 3 = slow fade to black, then the game over screen
static int endingPhase   = 0;
static float endingFade  = 0.0f;

int hexPoints            = 0;
int redPoints            = 0;
int bluePoints           = 0;
int greenPoints          = 0;
int technologyLevel      = 0;
int weaponsLevel         = 0;
int mobilityLevel        = 0;
int automationLevel      = 0;
bool showUpgradePanel    = false;
bool autoNavEnabled      = false;
int nukesOwned           = 0;
float nukeWaveRadius     = -1.0f;   // -1 = no wave running, otherwise current blast ring radius
bool campaignComplete    = false;   // the Mega Laser finished in campaign mode



//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    finishScreen = 0;
    paused       = false;
    endingPhase  = 0;
    endingFade   = 0.0f;

    // Every play through is fresh: wipe entities + progress, then build the world
    ECS::resetRun();
    ECS::spawnShip();
    ECS::spawnBackground();
    ECS::spawnInitialAsteroids();
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        paused = !paused;
    }

    // Paused = the world is frozen; only the menu buttons listen
    if (paused)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (CheckCollisionPointRec(GetMousePosition(), quitToMenuButtonRect))
            {
                paused = false;
                finishScreen = 1;   // TITLE
                SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
                PlaySound(fxSelection);
            }

#if !defined(PLATFORM_WEB)
            if (CheckCollisionPointRec(GetMousePosition(), exitGameButtonRect))
            {
                exitGameRequested = true;
            }
#endif

            if (CheckCollisionPointRec(GetMousePosition(), musicToggleButtonRect))
            {
                musicEnabled = !musicEnabled;

                if (musicEnabled)
                {
                    ResumeMusicStream(music);
                }
                else
                {
                    PauseMusicStream(music);
                }
            }
        }

        return;
    }

    if (endingPhase == 0)    // Normal play
    {
        ECS::inputSystem();
        ECS::moveToTargetSystem();
        ECS::idleDriftSystem();
        ECS::facingSystem();
        ECS::spinSystem();
        ECS::farPlanetOrbitSystem();
        ECS::movementSystem();
        ECS::turretFollowSystem();
        ECS::shipAutoMovementSystem();
        ECS::asteroidSpawnSystem();
        ECS::asteroidDespawnSystem();
        ECS::miningLaserSystem();
        ECS::nukeWaveSystem();
        ECS::deathTimerSystem();
        ECS::catcherCollisionSystem();
        PARTICLES::thrusterEmissionSystem();
        PARTICLES::particleUpdateSystem();
        ECS::upgradePanelInputSystem();
        ECS::laserBeamExpirySystem();

        // Debug/prototype controls (remember to disable for release!) ==========//
        // if (IsKeyPressed(KEY_P))
        // {
        //     // Give yourself MONEYYYYYY
        //     // 500 of each point
        //     ECS::addPointsDebug(hexPoints, redPoints, bluePoints, greenPoints);
        // }

        // The Mega Laser finished its sweep - roll the ending cinematic
        if (campaignComplete)
        {
            endingPhase    = 1;
            autoNavEnabled = false;    // nobody re-targets the fleet during the movie
        }
    }
    else    // Ending cinematic: no input, no spawns, no lasers - just the world winding down
    {
        ECS::moveToTargetSystem();
        ECS::idleDriftSystem();
        ECS::facingSystem();
        ECS::spinSystem();
        ECS::farPlanetOrbitSystem();
        ECS::movementSystem();
        ECS::turretFollowSystem();
        ECS::deathTimerSystem();
        PARTICLES::particleUpdateSystem();
        ECS::laserBeamExpirySystem();

        if (endingPhase == 1)    // Wait for every last burst pixel to fade
        {
            if (ECS::componentsPool<ECS::Particle>().empty())
            {
                endingPhase = 2;
                ECS::sendFleetHome();
            }
        }
        else if (endingPhase == 2)    // Fleet flies toward the home planet
        {
            PARTICLES::thrusterEmissionSystem();    // exhaust back on for the departure shot

            if (ECS::allShipsOffScreen())
            {
                endingPhase = 3;
            }
        }
        else if (endingPhase == 3)    // Slow fade to black, then game over
        {
            endingFade += GetFrameTime() / 3.0f;

            if (endingFade >= 1.0f)
            {
                finishScreen = 2;    // GAMEOVER
            }
        }
    }

    // Garbage collection, always run last!
    ECS::flushRemovalList();
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), backgroundColor);

    ECS::drawBackground();
    ECS::drawAsteroids();
    PARTICLES::drawParticles();
    ECS::drawShips();
    ECS::drawTurrets();
    ECS::drawLaserBeams();
    ECS::drawCatchers();
    ECS::drawNukeWave();
    PARTICLES::drawPopups();

    // The ending cinematic hides all the UI - just the world, the fleet, and the fade
    if (endingPhase > 0)
    {
        DrawRectangleLinesEx({ 0, 0, screenWidth, screenHeight }, screenBorderWidth, screenBorderColor);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, endingFade));
        ShowCursor();
        return;
    }

    ECS::drawData(hexPoints, redPoints, bluePoints, greenPoints);
    ECS::drawUpgradeMenu();
    DrawRectangleLinesEx({ 0, 0, screenWidth, screenHeight }, screenBorderWidth, screenBorderColor);
    PARTICLES::drawUiParticles(); // buy-feedback particle bursts
    ECS::drawFinalSweepPrompt();

    if (paused)
    {
        // Dim the frozen world, then the menu
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

        const char *heading = "PAUSED";
        DrawText(heading, (int)(screenWidth - MeasureText(heading, 40)) / 2, 220, 40, LIGHTGRAY);

        bool overQuit = CheckCollisionPointRec(GetMousePosition(), quitToMenuButtonRect);

        DrawRectangleRec(quitToMenuButtonRect, overQuit ? DARKBLUE : hudBackgroundColor);
        DrawRectangleLinesEx(quitToMenuButtonRect, 2.0f, overQuit ? SKYBLUE : screenBorderColor);
        DrawText("QUIT TO MAIN MENU", (int)(quitToMenuButtonRect.x + 40), (int)(quitToMenuButtonRect.y + 20), 20, overQuit ? SKYBLUE : LIGHTGRAY);

#if !defined(PLATFORM_WEB)   // browsers cant exit the app, just hide the button
        bool overExit = CheckCollisionPointRec(GetMousePosition(), exitGameButtonRect);

        DrawRectangleRec(exitGameButtonRect, overExit ? DARKBLUE : hudBackgroundColor);
        DrawRectangleLinesEx(exitGameButtonRect, 2.0f, overExit ? SKYBLUE : screenBorderColor);
        DrawText("EXIT GAME", (int)(exitGameButtonRect.x + 85), (int)(exitGameButtonRect.y + 20), 20, overExit ? SKYBLUE : LIGHTGRAY);
#endif

        bool overMusic = CheckCollisionPointRec(GetMousePosition(), musicToggleButtonRect);

        DrawRectangleRec(musicToggleButtonRect, overMusic ? DARKBLUE : hudBackgroundColor);
        DrawRectangleLinesEx(musicToggleButtonRect, 2.0f, overMusic ? SKYBLUE : screenBorderColor);
        DrawText(musicEnabled ? "MUSIC: ON" : "MUSIC: OFF", (int)(musicToggleButtonRect.x + 85), (int)(musicToggleButtonRect.y + 20), 20, overMusic ? SKYBLUE : LIGHTGRAY);

        ShowCursor();   // normal cursor for menu clicks
        return;         // skip the target reticle
    }

    ECS::drawTargetCursor(); // keep last so the reticle draws on top of everything
    
    // DrawFPS(50, 50);
}



// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    //TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}