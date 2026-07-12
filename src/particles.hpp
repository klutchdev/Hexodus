/**********************************************************************************************
*
*   Hexodus Particles - Declarations
*
*   Built by Kyle aka Klutch @ quicks games - special thanks to wubs @ wubs games
*
**********************************************************************************************/
#pragma once
#include "raylib.h"
#include "raymath.h"
#include "ecs.hpp"

namespace PARTICLES
{
    void spawnBurst(float x, float y, int count, Color color, bool onUiLayer = false);    // outward burst; UI layer = drawn over the menu
    void spawnPopup(float x, float y, int amount, Color color);                           // floating "+n" payout text

    void particleUpdateSystem();      // Tick every Lifetime, remove the dead
    void thrusterEmissionSystem();    // Exhaust for anything with a Thruster tag

    void drawParticles();             // World-layer particles
    void drawUiParticles();           // UI-layer particles, call after the menu
    void drawPopups();
}
