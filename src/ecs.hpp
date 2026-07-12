/**********************************************************************************************
*
*   Hexodus ECS - a tiny template-based entity component system
*
*   HOW IT WORKS
*     An Entity is just an id (uint32_t). Components are small plain structs stored in
*     one unordered_map per type ("pool"), keyed by entity. There are no entity objects,
*     no base classes, no registration lists - the compiler builds a pool the first time
*     componentsPool<T>() is mentioned, and that pool self-registers its own cleanup.
*
*   HOW TO ADD A NEW SYSTEM
*     1. Write the struct below (data component, or empty struct if only a tag)
*     2. Give it to entities at spawn:   componentsPool<T>()[e] = {...}
*     3. Write the system in ecs.cpp (loop a pool, look up siblings with getComponent)
*     4. Declare it in this file under Systems Declaration
*     5. Call it in UpdateGameplayScreen (update) or DrawGameplayScreen (draw)
*
*     - getComponent<T>(e) asserts if the component is missing
*     - Killing entities mid-loop: markForRemoval(e), then flushRemovalList() runs
*       last in the update order and actually destroys them
*
*   Built by Kyle aka Klutch @ quicks games - special thanks to wubs @ wubs games
*
**********************************************************************************************/
#pragma once
#include "raylib.h"
#include "constants.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cassert>

//----------------------------------------------------------------------------------
// Global Variables Declaration (game progress - plain globals defined in screen_gameplay.cpp)
//----------------------------------------------------------------------------------
extern int hexPoints;
extern int redPoints;
extern int bluePoints;
extern int greenPoints;
extern int weaponsLevel;
extern int mobilityLevel;
extern int technologyLevel;
extern int automationLevel;
extern int nukesOwned;
extern float nukeWaveRadius;        // -1 = no Mega Laser wave running, otherwise current ring radius
extern bool showUpgradePanel;
extern bool autoNavEnabled;
extern bool campaignComplete;       // set when the Mega Laser finishes its sweep in campaign mode

//----------------------------------------------------------------------------------
// Global Variables Declaration (textures - loaded once in hexodus.cpp main)
//----------------------------------------------------------------------------------
extern Texture2D shipTexture;
extern Texture2D turretTexture;
extern Texture2D catcherTexture;
extern Texture2D planetTexture;
extern Texture2D planetFarTexture;
extern Texture2D spaceTrashSprites[3][4];    // [tier - 1][variant]

namespace ECS
{
    using Entity = uint32_t;

    //----------------------------------------------------------------------------------
    // Components
    //----------------------------------------------------------------------------------

    // World position, center of the entity
    struct Position
    {
        float x;
        float y;
    };

    // Movement in pixels per second, applied to Position by movementSystem
    struct Velocity
    {
        float xSpeed;
        float ySpeed;
    };

    // Where a ship/catcher is flying to; its ABSENCE means the entity is 'parked'
    struct MoveTarget
    {
        float x;
        float y;
    };

    // Current speed: 0 at spawn, ramped up in flight, clamped by distance to brake
    struct MoveSpeed
    {
        float speed;
    };

    // Countdown to natural death; totalTime kept so draw can fade on a 0-1 ratio
    struct Lifetime
    {
        float timeLeft;
        float totalTime;
    };

    // NOTE: RED/GREEN/BLUE are raylib color macros, so members carry a CURRENCY_ prefix
    enum class Currency : uint8_t { CURRENCY_HEX, CURRENCY_RED, CURRENCY_GREEN, CURRENCY_BLUE };

    // Which currency a piece of trash pays out when destroyed
    struct PointColor
    {
        Currency value;
    };

    // Trash size class 1-3: drawn at tier * blockSize px, also the payout amount
    struct SizeTier
    {
        uint8_t tier;
    };

    // Width and height in pixels
    struct Size
    {
        float width;
        float height;
    };

    // Which texture this entity draws with
    struct Sprite
    {
        Texture2D sprite;
    };

    // Which variant of a sprite set to use, and the current draw rotation in degrees
    struct SpriteVariant
    {
        int index;
        float rotation;
    };

    // Remaining hit points
    struct Health
    {
        float hp;
    };

    // A laser zap, drawn while timeLeft > 0, tracks targetEntity while it lives
    // Beams are their own entities; ships and turrets ALSO carry one as aim memory only
    struct LaserBeam
    {
        float targetX;
        float targetY;
        float timeLeft;
        Entity targetEntity;
    };

    // Bolts a turret to a ship at a fixed offset; turretFollowSystem keeps it there.
    // It would suck to not have your turrets mounts hahaha
    struct TurretMount
    {
        Entity mountedOnShip;
        float offsetX;
        float offsetY;
    };

    // Time until the mining laser may fire again
    struct Cooldown
    {
        float timeUntilReady;
    };

    // Countdown to actual removal
    struct DeathTimer
    {
        float timeLeft;
    };

    // Which way the sprite faces on x: 1 = as drawn, -1 = mirrored
    // Only updated while genuinely moving, so it doesnt "flicker flip" (ow my eyes lol)
    struct Facing
    {
        float direction;
    };

    // A faint background star
    struct Star
    {
        float twinklePhase;
        float size;
    };

    // Slow constant rotation
    struct Spin
    {
        float degreesPerSecond;
    };

    // Floating "+n" payout text
    struct Popup
    {
        int amount;
    };

    //----------------------------------------------------------------------------------
    // Tags
    // Systems iterate these pools and look up sibling components
    //----------------------------------------------------------------------------------

    struct Asteroid {};     // a piece of space trash
    struct Ship {};         // a player ship
    struct Catcher {};      // an edge catcher that farms trash passively
    struct Particle {};     // a burst/exhaust pixel
    struct Thruster {};     // this entity emits engine exhaust while moving
    struct Planet {};       // a background planet
    struct UiLayer {};      // particles with this tag draw in a second pass ON TOP of the menu/HUD

    //----------------------------------------------------------------------------------
    // Entity Management Functions Declaration
    //----------------------------------------------------------------------------------

    Entity createEntity();              // Next unused id (0 = "no entity" - reserved for later!)
    void destroyEntity(Entity e);       // Erase all components immediately - NOT safe mid-loop
    void markForRemoval(Entity e);      // Queue for destruction - safe while iterating
    void flushRemovalList();            // Once at end of update. purgggggge!
    void clearAllPools();               // Every entity in the game stops existing
    void resetRun();                    // clearAllPools + zero all progress

    //----------------------------------------------------------------------------------
    // Component Storage
    //----------------------------------------------------------------------------------

    using ComponentEraser = void (*)(Entity);
    using PoolClearer     = void (*)();

    std::vector<ComponentEraser> &componentErasers();
    std::vector<PoolClearer>     &poolClearers();

    bool addComponentEraser(ComponentEraser eraseFunction);
    bool addPoolClearer(PoolClearer clearFunction);

    // The map holding every T in the game, keyed by entity
    // Self-registers an eraser (for destroyEntity) and a clearer (for clearAllPools)
    // on first use, so cleanup never needs a hand-maintained type list
    // Usage:
    //   Create at spawn:  componentsPool<T>()[e] = {...}
    //   Get a thing:      T &thing = getComponent<T>(e)
    //   Check existence:  componentsPool<T>().count(e)
    template <typename T>
    std::unordered_map<Entity, T> &componentsPool()
    {
        static std::unordered_map<Entity, T> pool;

        static ComponentEraser eraseFromThisPool = [](Entity e)
        {
            componentsPool<T>().erase(e);
        };

        static PoolClearer clearThisPool = []()
        {
            componentsPool<T>().clear();
        };

        [[maybe_unused]] static bool eraserRegistered  = addComponentEraser(eraseFromThisPool);
        [[maybe_unused]] static bool clearerRegistered = addPoolClearer(clearThisPool);

        return pool;
    }

    // Sibling lookup - asserts if the entity lacks the component
    template <typename T>
    T &getComponent(Entity e)
    {
        auto found = componentsPool<T>().find(e);
        assert(found != componentsPool<T>().end() && "Entity is missing this component!");
        return found->second;
    }

    //----------------------------------------------------------------------------------
    // Spawn Functions Declaration (create an entity, give it components)
    //----------------------------------------------------------------------------------

    void spawnBackground();                                         // Stars + planets, once at gameplay init
    void spawnShip();                                               // First ship at init, extras via automation upgrades
    void spawnTurret(Entity ship, float offsetX, float offsetY);    // Bolt a turret onto a ship
    Entity spawnAsteroid();                                         // One piece of trash off-edge; returns it so callers can adjust
    void spawnInitialAsteroids();                                   // Starting field scattered on-screen, once at init
    void spawnCatcher(int edge);                                    // Catcher flies in to the left (1) or right (2) edge

    //----------------------------------------------------------------------------------
    // Update Systems Declaration
    // Declared in the order UpdateGameplayScreen calls them
    //----------------------------------------------------------------------------------

    void inputSystem();                 // Click/tap -> MoveTarget for the fleet
    void moveToTargetSystem();          // MoveTarget -> Velocity (speed ramp + braking)
    void idleDriftSystem();             // Parked ships and catchers bob gently
    void facingSystem();                // Velocity direction -> sprite mirroring
    void spinSystem();                  // Spin -> SpriteVariant rotation
    void farPlanetOrbitSystem();        // Wrap the far planet around for another pass
    void movementSystem();              // Velocity -> Position, for everything at once
    void turretFollowSystem();          // Turret position = owner position + mount offset
    void shipAutoMovementSystem();      // Auto Nav: fleet repositions itself on a timer
    void asteroidSpawnSystem();         // Timed trash spawns while under the cap
    void asteroidDespawnSystem();       // Cull trash that drifted far off-screen
    void miningLaserSystem();           // Turrets on parked ships zap trash in radius
    void zapAsteroid(Entity asteroidEntity, float sourceX, float sourceY);    // One zap: beam + damage + payout on kill
    void nukeWaveSystem();              // Expand the Mega Laser ring, destroy everything it passes
    void fireNuke();                    // Light the fuse (nukeWaveSystem does the rest)
    void deathTimerSystem();            // Delayed removal of killed trash (burst included)
    void catcherCollisionSystem();      // Catchers eat trash on contact
    void upgradePanelInputSystem();     // All the buy/toggle/ARM/LAUNCH clicks

    void laserBeamExpirySystem();       // Beam entities tick, track their target, die at zero

    //----------------------------------------------------------------------------------
    // Upgrade Helper Functions Declaration
    //----------------------------------------------------------------------------------

    void payOut(Currency pointColor, uint8_t tier, float x, float y);    // Pay a kill's bounty into the matching currency
    float effectiveLaserRadius();                                        // Base radius + blue upgrades
    void growCatchers();                                                 // Stretch catchers along their long axis

    void sendFleetHome();               // Ending cinematic: every ship flies off toward the home planet
    bool allShipsOffScreen();           // True once the whole fleet has left the screen

    //----------------------------------------------------------------------------------
    // Draw Systems Declaration
    // Declared in paint order - same order DrawGameplayScreen calls them
    //----------------------------------------------------------------------------------

    void drawBackground();              // Stars, then planets
    void drawAsteroids();               // Trash sprites, tinted by point color
    void drawShips();
    void drawTurrets();
    void drawLaserBeams();
    void drawCatchers();
    void drawNukeWave();                // The expanding Mega Laser ring
    void drawData(int &hPoints, int &rPoints, int &bPoints, int &gPoints);    // The HUD bar
    void drawUpgradeMenu();             // Top-right buttons (+ the panel when open)
    void drawUpgradePanel();
    void drawTechRow(const char *label, int level, int nodeCount, const char **nodeNames, int cost, float rowY, Color boughtColor, Color buyableColor, bool unlocked);
    void drawButton(Rectangle buttRect, Color btnColor, Color txtColor, const char *label);
    void drawFinalSweepPrompt();        // Campaign only: buy/launch the Mega Laser nudges
    void drawTargetCursor();            // Reticle over the playfield, normal cursor on the HUD

    //----------------------------------------------------------------------------------
    // Debug Functions Declaration
    //----------------------------------------------------------------------------------

    void addPointsDebug(int &hPoints, int &rPoints, int &bPoints, int &gPoints);    // MO MONEYYYYYY (500 of each)
}
