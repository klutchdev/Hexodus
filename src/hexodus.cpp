/*******************************************************************************************
*
*   HEXODUS - an idle game for cleaning space trash
*
*   Built by Kyle aka Klutch @ quicks games for the raylib 6 game jam (7/6/26 - 7/12/26)
*   Special thanks to wubs games for the planet sprites and soundtrack
*
*   Based on the raylib game template:
*   Code licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*   Copyright (c) 2021-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "screens.hpp"    // NOTE: Declares global (extern) variables and screens functions
#include "constants.hpp"
#include "ecs.hpp"
#include "particles.hpp"
#include <ctime>    // Required for: time() (seeding rand)

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
// NOTE: Those variables are shared between modules through screens.hpp
//----------------------------------------------------------------------------------
GameScreen currentScreen = LOGO;
bool unlimitedMode       = false;    // set by the mode select screen
bool exitGameRequested   = false;    // set by the pause menu's Exit Game button

Font font              = { 0 };
Music music            = { 0 };
Music bgAmbience       = { 0 };
Sound fxCoin           = { 0 };
Sound fxCatcherDeploy  = { 0 };
Sound fxLaser          = { 0 };
Sound fxSelection      = { 0 };
Sound fxUpgrade        = { 0 };
Sound fxPop            = { 0 };

Shader shader          = { 0 };

RenderTexture2D gameCanvas              = { 0 };
Texture2D shipTexture             = { 0 };
Texture2D turretTexture           = { 0 };
Texture2D catcherTexture          = { 0 };
Texture2D planetTexture           = { 0 };
Texture2D planetFarTexture        = { 0 };
Texture2D spaceTrashSprites[3][4] = { 0 };    // [tier - 1][variant]

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
// static Music musicTracks[2] = { 0 };    // the two-song playlist; `music` is whichever is playing
// static int currentTrack     = 0;

// Required variables to manage screen transitions (fade-in, fade-out)
static float transAlpha         = 0.0f;
static bool onTransition        = false;
static bool transFadeOut        = false;
static int transFromScreen      = -1;
static GameScreen transToScreen = UNKNOWN;

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void TransitionToScreen(GameScreen screen);    // Request transition to next screen
static void UpdateTransition(void);                   // Update transition effect
static void DrawTransition(void);                     // Draw transition effect (full-screen rectangle)

static void UpdateFrame(void);                        // Update game logic for one frame
static void DrawFrame(void);                          // Draw one frame

#if defined(PLATFORM_WEB)
static void RunFrame(void);                           // Update + draw (single callback for web main loop)
#endif

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, gameTitle);
    SetExitKey(KEY_NULL);    // ESC opens the pause menu instead of quitting raylib
    srand(time(NULL));

    InitAudioDevice();

    shader = LoadShader(0, "resources/bloom.fs");

    // Load global data (assets that must be available in all screens!)
    font             = LoadFont("resources/mecha.png");
    fxCoin           = LoadSound("resources/coin.wav");
    fxCatcherDeploy  = LoadSound("resources/catcher.wav");
    fxLaser          = LoadSound("resources/laser.wav");
    fxSelection      = LoadSound("resources/selection.wav");
    fxUpgrade        = LoadSound("resources/upgrade.wav");
    fxPop            = LoadSound("resources/pop.wav");
    
    music = LoadMusicStream("resources/mainTrack.mp3");
    // bgAmbience     = LoadMusicStream("resources/bgAmbient.wav");
    // musicTracks[0] = LoadMusicStream("resources/The_Commodore_I.mp3");
    // musicTracks[1] = LoadMusicStream("resources/The_Commodore_V.mp3");

    // Looping off: each track runs out naturally, then UpdateFrame flips to the other one
    // musicTracks[0].looping = false;
    // musicTracks[1].looping = false;
    // bgAmbience.looping     = true;

    // music = musicTracks[0];

    SetSoundVolume(fxCoin, 0.05f);
    SetSoundVolume(fxCatcherDeploy, 0.1f);
    SetSoundVolume(fxLaser, 0.075f);
    SetSoundVolume(fxSelection, 0.1f);
    SetSoundVolume(fxUpgrade, 0.075f);
    SetSoundVolume(fxPop, 0.15f);

    // SetMusicVolume(musicTracks[0], 0.01f);
    // SetMusicVolume(musicTracks[1], 0.01f);
    SetMusicVolume(music, 0.04f);
    // SetMusicVolume(bgAmbience, 0.05f);

    PlayMusicStream(music);
    // PlayMusicStream(bgAmbience);

    shipTexture      = LoadTexture("resources/ship.png");
    turretTexture    = LoadTexture("resources/turret.png");
    catcherTexture   = LoadTexture("resources/catcher-64x16.png");
    planetTexture    = LoadTexture("resources/planet.png");
    planetFarTexture = LoadTexture("resources/planetFar.png");

    gameCanvas       = LoadRenderTexture(screenWidth, screenHeight);

    SetTextureFilter(shipTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(turretTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(catcherTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(planetTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(planetFarTexture, TEXTURE_FILTER_POINT);

    int spaceTrashPixelSizes[3] = { 4, 8, 12 };

    for (int tier = 0; tier < 3; tier++)
    {
        for (int variant = 0; variant < 4; variant++)
        {
            spaceTrashSprites[tier][variant] = LoadTexture(TextFormat("resources/spacetrash-%ix%i-%02i.png", spaceTrashPixelSizes[tier], spaceTrashPixelSizes[tier], variant + 1));
            SetTextureFilter(spaceTrashSprites[tier][variant], TEXTURE_FILTER_POINT);
        }
    }

    // Setup and init first screen
    currentScreen = LOGO;
    InitLogoScreen();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(RunFrame, 60, 1);
#else
    SetTargetFPS(targetFPS);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose() && !exitGameRequested)    // window close button, or Exit Game from the pause menu
    {
        UpdateFrame();
        DrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload current screen data before closing
    switch (currentScreen)
    {
        case LOGO: UnloadLogoScreen(); break;
        case TITLE: UnloadTitleScreen(); break;
        case MODESELECT: UnloadModeSelectScreen(); break;
        case DIALOG: UnloadDialogScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case GAMEOVER: UnloadGameOverScreen(); break;
        default: break;
    }

    // Unload global data loaded
    UnloadFont(font);
    // UnloadMusicStream(musicTracks[0]);
    // UnloadMusicStream(musicTracks[1]);
    // UnloadMusicStream(bgAmbience);
    UnloadMusicStream(music);
    UnloadSound(fxCoin);
    UnloadSound(fxCatcherDeploy);
    UnloadSound(fxLaser);
    UnloadSound(fxSelection);
    UnloadSound(fxUpgrade);
    UnloadSound(fxPop);

    // Unload ship, turret, catcher, planet, space trash, and planet sprites
    UnloadTexture(shipTexture);
    UnloadTexture(turretTexture);
    UnloadTexture(catcherTexture);
    UnloadTexture(planetTexture);
    UnloadTexture(planetFarTexture);
    UnloadRenderTexture(gameCanvas);

    for (int tier = 0; tier < 3; tier++)
    {
        for (int variant = 0; variant < 4; variant++)
        {
            UnloadTexture(spaceTrashSprites[tier][variant]);
        }
    }

    CloseAudioDevice();

    CloseWindow();
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Request transition to next screen
static void TransitionToScreen(GameScreen screen)
{
    onTransition    = true;
    transFadeOut    = false;
    transFromScreen = currentScreen;
    transToScreen   = screen;
    transAlpha      = 0.0f;
}

// Update transition effect (fade-in, fade-out)
static void UpdateTransition(void)
{
    if (!transFadeOut)
    {
        transAlpha += 0.05f;

        // NOTE: Due to float internal representation, condition jumps on 1.0f instead of 1.05f
        // For that reason we compare against 1.01f, to avoid last frame loading stop
        if (transAlpha > 1.01f)
        {
            transAlpha = 1.0f;

            // Unload current screen
            switch (transFromScreen)
            {
                case LOGO: UnloadLogoScreen(); break;
                case TITLE: UnloadTitleScreen(); break;
                case MODESELECT: UnloadModeSelectScreen(); break;
                case DIALOG: UnloadDialogScreen(); break;
                case GAMEPLAY: UnloadGameplayScreen(); break;
                case GAMEOVER: UnloadGameOverScreen(); break;
                default: break;
            }

            // Load next screen
            switch (transToScreen)
            {
                case LOGO: InitLogoScreen(); break;
                case TITLE: InitTitleScreen(); break;
                case MODESELECT: InitModeSelectScreen(); break;
                case DIALOG: InitDialogScreen(); break;
                case GAMEPLAY: InitGameplayScreen(); break;
                case GAMEOVER: InitGameOverScreen(); break;
                default: break;
            }

            currentScreen = transToScreen;

            // Activate fade out effect to next loaded screen
            transFadeOut = true;
        }
    }
    else    // Transition fade out logic
    {
        transAlpha -= 0.02f;

        if (transAlpha < -0.01f)
        {
            transAlpha      = 0.0f;
            transFadeOut    = false;
            onTransition    = false;
            transFromScreen = -1;
            transToScreen   = UNKNOWN;
        }
    }
}

// Draw transition effect (full-screen rectangle)
static void DrawTransition(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transAlpha));
}

// Update game logic for one frame
static void UpdateFrame(void)
{
    UpdateMusicStream(music);    // NOTE: Music keeps playing between screens
    // UpdateMusicStream(bgAmbience);

    // Track ran out? Flip to the other one (0.2s early - the stream can stall just short of the end)
    // if (GetMusicTimePlayed(music) >= GetMusicTimeLength(music) - 0.2f)
    // {
    //     StopMusicStream(music);

    //     currentTrack = (currentTrack + 1) % 2;
    //     music = musicTracks[currentTrack];

    //     PlayMusicStream(music);
    // }

    if (!onTransition)
    {
        // Screen flow: LOGO -> TITLE -> MODESELECT -> DIALOG -> GAMEPLAY (-> GAMEOVER in campaign)
        switch (currentScreen)
        {
            case LOGO:
            {
                UpdateLogoScreen();

                if (FinishLogoScreen())
                {
                    TransitionToScreen(TITLE);
                }
            } break;
            case TITLE:
            {
                UpdateTitleScreen();

                if (FinishTitleScreen())
                {
                    TransitionToScreen(MODESELECT);
                }
            } break;
            case MODESELECT:
            {
                UpdateModeSelectScreen();

                if (FinishModeSelectScreen())
                {
                    TransitionToScreen(DIALOG);
                }
            } break;
            case DIALOG:
            {
                UpdateDialogScreen();

                if (FinishDialogScreen())
                {
                    TransitionToScreen(GAMEPLAY);
                }
            } break;
            case GAMEPLAY:
            {
                UpdateGameplayScreen();

                if (FinishGameplayScreen() == 1)
                {
                    TransitionToScreen(TITLE);       // pause menu quit
                }
                else if (FinishGameplayScreen() == 2)
                {
                    TransitionToScreen(GAMEOVER);    // the Mega Laser finished its sweep
                }
            } break;
            case GAMEOVER:
            {
                UpdateGameOverScreen();

                if (FinishGameOverScreen())
                {
                    TransitionToScreen(TITLE);
                }
            } break;
            default: break;
        }
    }
    else
    {
        UpdateTransition();
    }
}

// Draw one frame
static void DrawFrame(void)
{
    BeginDrawing();

        BeginTextureMode(gameCanvas);

            ClearBackground(WHITE);

            switch (currentScreen)
            {
                case LOGO: DrawLogoScreen(); break;
                case TITLE: DrawTitleScreen(); break;
                case MODESELECT: DrawModeSelectScreen(); break;
                case DIALOG: DrawDialogScreen(); break;
                case GAMEPLAY: DrawGameplayScreen(); break;
                case GAMEOVER: DrawGameOverScreen(); break;
                default: break;
            }

            // Draw full screen rectangle in front of everything
            if (onTransition)
            {
                DrawTransition();
            }

        EndTextureMode();

        BeginShaderMode(shader);

        DrawTexturePro(gameCanvas.texture,{ 0.0f, 0.0f, (float)gameCanvas.texture.width, -(float)gameCanvas.texture.height },{ 0.0f, 0.0f, (float)screenWidth, (float)screenHeight },{ 0.0f, 0.0f }, 0.0f, WHITE);

        // DrawFPS(100, 100);

        EndShaderMode();
        

    EndDrawing();
}

#if defined(PLATFORM_WEB)
// Update + draw, kept as one function for the web main loop callback
static void RunFrame(void)
{
    UpdateFrame();
    DrawFrame();
}
#endif
