/**********************************************************************************************
*
*   Hexodus Particles - bursts, thruster exhaust, and payout popups
*
*   Particles are just ECS entities (Particle tag + Position + Velocity + Lifetime + Color, etc.)
*   so the existing movementSystem flies them - this module only spawns,
*   expires, and draws them.
*
*   Built by Kyle aka Klutch @ quicks games - special thanks to wubs @ wubs games
*
**********************************************************************************************/
#include "particles.hpp"
#include "utils.hpp"

namespace PARTICLES
{
    //----------------------------------------------------------------------------------
    // Spawn Functions Definition
    //----------------------------------------------------------------------------------

    // Spawn particles flying "outward" from a point in random directions
    // onUiLayer = true draws them in the second pass, on top of the menu/HUD
    void spawnBurst(float x, float y, int count, Color color, bool onUiLayer)
    {
        for (int i = 0; i < count; i++)
        {
            ECS::Entity particle = ECS::createEntity();

            float angle = UTILS::getRandomFloat(0.0f, 3.0f * PI);
            float speed = UTILS::getRandomFloat(0.25f, 25.0f);
            float life  = UTILS::getRandomFloat(0.8f, 8.0f);

            ECS::componentsPool<ECS::Particle>()[particle] = {};
            ECS::componentsPool<ECS::Position>()[particle] = { x, y };
            ECS::componentsPool<ECS::Velocity>()[particle] = { cosf(angle) * speed, sinf(angle) * speed };
            ECS::componentsPool<ECS::Lifetime>()[particle] = { life, life };
            ECS::componentsPool<Color>()        [particle] = color;

            if (onUiLayer)
            {
                ECS::componentsPool<ECS::UiLayer>()[particle] = {};
            }
        }
    }

    // Floating "+n" payout text that rises and fades
    void spawnPopup(float x, float y, int amount, Color color)
    {
        ECS::Entity popup = ECS::createEntity();

        ECS::componentsPool<ECS::Popup>()   [popup] = { amount };
        ECS::componentsPool<ECS::Position>()[popup] = { x, y };
        ECS::componentsPool<ECS::Velocity>()[popup] = { 0.0f, -30.0f };    // rises
        ECS::componentsPool<ECS::Lifetime>()[popup] = { 0.8f, 0.8f };
        ECS::componentsPool<Color>()        [popup] = color;
    }

    //----------------------------------------------------------------------------------
    // Update Systems Definition
    //----------------------------------------------------------------------------------

    // Tick EVERYTHING with a Lifetime (particles, popups, whatever comes next) and
    // remove the dead ones
    void particleUpdateSystem()
    {
        for (auto& [entity, lifetime] : ECS::componentsPool<ECS::Lifetime>())
        {
            lifetime.timeLeft -= GetFrameTime();

            if (lifetime.timeLeft <= 0.0f)
            {
                ECS::markForRemoval(entity);
            }
        }
    }

    // Everything with a Thruster tag gets a flight trail; parked SHIPS also get
    // the occasional trail
    void thrusterEmissionSystem()
    {
        for (auto& [shipEntity, thruster] : ECS::componentsPool<ECS::Thruster>())
        {
            ECS::Position &position = ECS::getComponent<ECS::Position>(shipEntity);
            ECS::Velocity &velocity = ECS::getComponent<ECS::Velocity>(shipEntity);

            float speed = sqrtf(velocity.xSpeed * velocity.xSpeed + velocity.ySpeed * velocity.ySpeed);

            if (speed < 10.0f)
            {
                if (ECS::componentsPool<ECS::Ship>().count(shipEntity) == 0)
                {
                    continue;    // only ships idle-puff
                }

                if (GetRandomValue(0, 4) != 0)
                {
                    continue;    // ~12 puffs/sec instead of 60
                }

                ECS::Entity particle = ECS::createEntity();

                float life    = UTILS::getRandomFloat(0.15f, 1.0f);
                float jitterX = UTILS::getRandomFloat(-25.0f, 25.0f);

                ECS::componentsPool<ECS::Particle>()[particle] = {};
                ECS::componentsPool<ECS::Position>()[particle] = { position.x, position.y + 10.0f };    // just past the back edge
                ECS::componentsPool<ECS::Velocity>()[particle] = { jitterX, UTILS::getRandomFloat(0.2f, 10.0f) };
                ECS::componentsPool<ECS::Lifetime>()[particle] = { life, life };
                ECS::componentsPool<Color>()        [particle] = UTILS::getRandColor(thrusterColors);

                continue;    // idle handled, skip the flight-trail spawn below
            }

            // Flight trail: exhaust flies backward at half the ship's speed, jittered into a plume
            // (several per frame - at 60fps a single one is weak as heck)
            for (int i = 0; i < 8; i++)
            {
                ECS::Entity particle = ECS::createEntity();

                float life    = UTILS::getRandomFloat(0.05f, 0.75f);
                float jitterX = UTILS::getRandomFloat(-20.0f, 20.0f);
                float jitterY = UTILS::getRandomFloat(-15.0f, 15.0f);

                ECS::componentsPool<ECS::Particle>()[particle] = {};
                ECS::componentsPool<ECS::Position>()[particle] = { position.x, position.y };
                ECS::componentsPool<ECS::Velocity>()[particle] = { -velocity.xSpeed * 0.5f + jitterX, -velocity.ySpeed * 0.5f + jitterY };
                ECS::componentsPool<ECS::Lifetime>()[particle] = { life, life };
                ECS::componentsPool<Color>()        [particle] = UTILS::getRandColor(thrusterColors);
            }
        }
    }

    //----------------------------------------------------------------------------------
    // Draw Systems Definition (only look, never mutate)
    //----------------------------------------------------------------------------------

    // World-layer particles as small fading squares (UI-tagged ones wait for the second pass)
    void drawParticles()
    {
        for (auto& [entity, particle] : ECS::componentsPool<ECS::Particle>())
        {
            if (ECS::componentsPool<ECS::UiLayer>().count(entity))
            {
                continue;    // those draw in the UI pass
            }

            ECS::Position &position = ECS::getComponent<ECS::Position>(entity);
            ECS::Lifetime &lifetime = ECS::getComponent<ECS::Lifetime>(entity);
            Color         &color    = ECS::getComponent<Color>(entity);

            float fade = Lerp(0.2f, 1.0f, lifetime.timeLeft / lifetime.totalTime);    // 1.0 fresh, 0.2 nearly dead

            DrawRectangleRec({ position.x - 1, position.y - 1, 3, 3 }, Fade(color, fade));
        }
    }

    // Second pass: only the UI-tagged particles, called after the menu is drawn
    void drawUiParticles()
    {
        for (auto& [entity, uiTag] : ECS::componentsPool<ECS::UiLayer>())
        {
            ECS::Position &position = ECS::getComponent<ECS::Position>(entity);
            ECS::Lifetime &lifetime = ECS::getComponent<ECS::Lifetime>(entity);
            Color         &color    = ECS::getComponent<Color>(entity);

            float fade = Lerp(0.2f, 1.0f, lifetime.timeLeft / lifetime.totalTime);

            DrawRectangleRec({ position.x - 1, position.y - 1, 3, 3 }, Fade(color, fade));
        }
    }

    // The rising "+n" payout text
    void drawPopups()
    {
        for (auto& [entity, popup] : ECS::componentsPool<ECS::Popup>())
        {
            ECS::Position &position = ECS::getComponent<ECS::Position>(entity);
            ECS::Lifetime &lifetime = ECS::getComponent<ECS::Lifetime>(entity);
            Color         &color    = ECS::getComponent<Color>(entity);

            float fade = lifetime.timeLeft / lifetime.totalTime;

            DrawText(TextFormat("+%d", popup.amount), (int)position.x, (int)position.y, 14, Fade(color, fade));
        }
    }
}
