/**********************************************************************************************
*
*   Hexodus ECS - Systems Definitions (spawn, update, draw)
*
*
*   Built by Kyle aka Klutch @ quicks games - special thanks to wubs @ wubs games
*
**********************************************************************************************/
#include "ecs.hpp"
#include "utils.hpp"
#include "screens.hpp"
#include "particles.hpp"
#include <raymath.h>
#include <vector>
#include <cmath>

namespace ECS
{
    //----------------------------------------------------------------------------------
    // Module Variables Definition (local)
    //----------------------------------------------------------------------------------
    static std::vector<Entity> removalList;             // Entities queued to die at end of update

    static bool megaLaserArmed        = false;          // Campaign: first click ARMs, second click LAUNCHes
    static float megaLaserDeniedTimer = 0.0f;           // Seconds left to show "upgrade everything first"

    // The fire button is small in unlimited (hex + count) and wide in campaign (ARM/LAUNCH text)
    static Rectangle megaLaserButtonRect()
    {
        return unlimitedMode ? nukeButtonRect : sweepButtonWideRect;
    }

    //----------------------------------------------------------------------------------
    // Entity Management Functions Definition
    //----------------------------------------------------------------------------------

    // Hand out the next unused entity id (0 is reserved as "no entity")
    Entity createEntity()
    {
        static Entity entityId = 1;
        assert(entityId != 0 && "Entity ID overflow!");
        return entityId++;
    }

    // Registry of erase functions, one per component pool in use
    // Each pool self-registers on first use, so destroyEntity never needs a type list
    // *whew* haha
    std::vector<ComponentEraser> &componentErasers()
    {
        static std::vector<ComponentEraser> list;
        return list;
    }

    bool addComponentEraser(ComponentEraser eraseFunction)
    {
        componentErasers().push_back(eraseFunction);
        return true;    // exists only so a static bool can trigger this one time
    }

    // Remove every component an entity has, whatever the type - NOT safe mid-loop
    void destroyEntity(Entity e)
    {
        for (ComponentEraser erase : componentErasers())
        {
            erase(e);
        }
    }

    // Registry of whole-pool clear functions
    std::vector<PoolClearer> &poolClearers()
    {
        static std::vector<PoolClearer> list;
        return list;
    }

    bool addPoolClearer(PoolClearer clearFunction)
    {
        poolClearers().push_back(clearFunction);
        return true;
    }

    // Empty every pool at once - every entity in the game stops existing
    void clearAllPools()
    {
        for (PoolClearer clear : poolClearers())
        {
            clear();
        }

        removalList.clear();    // anything queued to die already did
    }

    // Fresh mission: wipe all entities / progress
    void resetRun()
    {
        clearAllPools();

        hexPoints        = 0;
        redPoints        = 0;
        bluePoints       = 0;
        greenPoints      = 0;
        technologyLevel  = 0;
        weaponsLevel     = 0;
        mobilityLevel    = 0;
        automationLevel  = 0;
        nukesOwned       = 0;
        nukeWaveRadius   = -1.0f;
        autoNavEnabled   = false;
        showUpgradePanel = false;
        campaignComplete = false;

        megaLaserArmed        = false;
        megaLaserDeniedTimer  = 0.0f;
    }

    // Queue an entity for destruction (safe while iterating pools)
    void markForRemoval(Entity e)
    {
        removalList.push_back(e);
    }

    // Destroy all queued entities; called once at end of update. purgggggge!
    void flushRemovalList()
    {
        for (Entity e : removalList)
        {
            destroyEntity(e);
        }

        removalList.clear();
    }

    //----------------------------------------------------------------------------------
    // Spawn Functions Definition (create an entity, give it components)
    //----------------------------------------------------------------------------------

    // Populate the sky with faint stars, the home planet, and another far away planet
    void spawnBackground()
    {
        for (int i = 0; i < 70; i++)
        {
            Entity star = createEntity();

            componentsPool<Star>()    [star] = { UTILS::getRandomFloat(0.0f, 2.0f * PI), UTILS::getRandomFloat(1.0f, 2.0f) };
            componentsPool<Position>()[star] = { UTILS::getRandomFloat(0.0f, screenWidth), UTILS::getRandomFloat(0.0f, screenHeight) };
        }

        // Home planet: center sits past the bottom-right corner
        Entity homePlanet = createEntity();

        componentsPool<Planet>()       [homePlanet] = {};
        componentsPool<Sprite>()       [homePlanet] = { planetTexture };
        componentsPool<Position>()     [homePlanet] = { screenWidth + 30.0f, screenHeight + 30.0f };
        componentsPool<Size>()         [homePlanet] = { 380.0f, 380.0f };
        componentsPool<Spin>()         [homePlanet] = { 0.6f };
        componentsPool<SpriteVariant>()[homePlanet] = { 0, UTILS::getRandomFloat(0.0f, 360.0f) };
        componentsPool<Color>()        [homePlanet] = Fade(WHITE, 0.85f);

        // Far planet: tiny, super faint, drifts across the top of the sky
        Entity farPlanet = createEntity();

        componentsPool<Planet>()       [farPlanet] = {};
        componentsPool<Sprite>()       [farPlanet] = { planetFarTexture };
        componentsPool<Position>()     [farPlanet] = { UTILS::getRandomFloat(0.0f, screenWidth), UTILS::getRandomFloat(60.0f, 250.0f) };
        componentsPool<Size>()         [farPlanet] = { 100.0f, 100.0f };
        componentsPool<Spin>()         [farPlanet] = { 3.0f };
        componentsPool<SpriteVariant>()[farPlanet] = { 0, UTILS::getRandomFloat(0.0f, 360.0f) };
        componentsPool<Color>()        [farPlanet] = Fade(WHITE, 0.15f);
        componentsPool<Velocity>()     [farPlanet] = { -1.0f, 0.0f };    // movementSystem drifts it for free
    }

    // Create a player ship
    void spawnShip()
    {
        Entity ship = createEntity();

        componentsPool<Ship>()      [ship] = {};    // tag
        componentsPool<Thruster>()  [ship] = {};
        componentsPool<Facing>()    [ship] = { 1.0f };
        componentsPool<Sprite>()    [ship] = { shipTexture };
        componentsPool<Velocity>()  [ship] = { 0, 0 };
        componentsPool<MoveSpeed>() [ship] = { 0 };
        componentsPool<Color>()     [ship] = { PURPLE };

        if (componentsPool<Ship>().size() == 1)
        {
            // Spawn first ship in the middle of the screen
            componentsPool<Position>()[ship] = { screenWidth / 2, screenHeight / 2 };
        }
        else
        {
            // Subsequent ships will spawn in a random place
            componentsPool<Position>()[ship] = { UTILS::getRandomFloat(0.0f, screenWidth - 50), UTILS::getRandomFloat(0.0f, screenHeight - 50) };
        }

        componentsPool<Cooldown>()  [ship] = { 0 };
        componentsPool<LaserBeam>() [ship] = { 0, 0, 0, 0 };

        // Spawn the initial turret, plus extras if tech is already there
        spawnTurret(ship, -10, 0);

        if (technologyLevel >= 4)
        {
            spawnTurret(ship, 8, 0);
        }
        if (technologyLevel >= 5)
        {
            spawnTurret(ship, 24, 0);
        }
    }

    // Bolt a turret onto a ship; turretFollowSystem keeps it at the offset so you dont have floating turrets
    void spawnTurret(Entity ship, float offsetX, float offsetY)
    {
        Entity turret = createEntity();

        Position &shipPosition = getComponent<Position>(ship);

        componentsPool<TurretMount>()[turret] = { ship, offsetX, offsetY };
        componentsPool<Position>()   [turret] = { shipPosition.x + offsetX, shipPosition.y + offsetY };
        componentsPool<LaserBeam>()  [turret] = { 0, 0, 0, 0 };    // aim memory, not a live beam
        componentsPool<Cooldown>()   [turret] = { 0 };
    }

    // Create one piece of space trash just off the left or right edge, drifting inward
    Entity spawnAsteroid()
    {
        Entity asteroid = createEntity();

        Currency pointColor = Currency::CURRENCY_HEX;
        Color asteroidColor = UTILS::getRandColor({LIGHTGRAY, GRAY, DARKGRAY});

        int rollRarity     = GetRandomValue(1, 3);                           // 1-in-3 is colored
        int unlockedColors = (technologyLevel < 3) ? technologyLevel : 3;    // tech nodes past 3 are turrets, not colors
        int rollColor      = GetRandomValue(1, unlockedColors);

        if (technologyLevel > 0 && rollRarity == 1)
        {
            if (rollColor == 1)
            {
                pointColor    = Currency::CURRENCY_RED;
                asteroidColor = trashRed;
            }
            else if (rollColor == 2)
            {
                pointColor    = Currency::CURRENCY_GREEN;
                asteroidColor = trashGreen;
            }
            else
            {
                pointColor    = Currency::CURRENCY_BLUE;
                asteroidColor = trashBlue;
            }
        }

        uint8_t sizeTier = GetRandomValue(1, 3);

        bool spawnOnLeftSide = (GetRandomValue(0, 1) == 0);    // Off to the left or right edge?

        float spawnX = spawnOnLeftSide ? UTILS::getRandomFloat(-20.0f, 0.0f) : UTILS::getRandomFloat(screenWidth, screenWidth + 20.0f);
        float spawnY = UTILS::getRandomFloat(0.0f, screenHeight);

        // Drift gets faster as the players kill power grows, so the late game stays *lively* haha
        float effectiveMaxDrift = asteroidMaxDriftSpeed + (weaponsLevel + automationLevel) * 1.5f;

        float xSpeed = UTILS::getRandomFloat(asteroidMinDriftSpeed, effectiveMaxDrift);

        if (!spawnOnLeftSide)
        {
            xSpeed = -xSpeed;
        }

        float ySpeed = UTILS::getRandomFloat(-2.0f, 2.0f);    // gentle vertical drift

        componentsPool<Asteroid>()     [asteroid] = {};    // Just a tag
        componentsPool<Position>()     [asteroid] = { spawnX, spawnY };
        componentsPool<Velocity>()     [asteroid] = { xSpeed, ySpeed };
        componentsPool<Health>()       [asteroid] = { asteroidBaseHp * sizeTier };
        componentsPool<SizeTier>()     [asteroid] = { sizeTier };
        componentsPool<PointColor>()   [asteroid] = { pointColor };
        componentsPool<Color>()        [asteroid] = { asteroidColor };
        componentsPool<SpriteVariant>()[asteroid] = { GetRandomValue(0, 3), UTILS::getRandomFloat(0.0f, 90.0f) };

        return asteroid;
    }

    // A starting field so the first thing the player sees isnt empty space
    void spawnInitialAsteroids()
    {
        for (int i = 0; i < 15; i++)
        {
            Entity asteroid = spawnAsteroid();

            // Spawned off-edge like normal, then dragged to a random spot on the playfield
            getComponent<Position>(asteroid) = { UTILS::getRandomFloat(30.0f, screenWidth - 30.0f),
                                                 UTILS::getRandomFloat(hudBarHeight + 20.0f, screenHeight - 30.0f) };
        }
    }

    // Deploy a catcher: flies in from below the screen to its post on the left or right edge
    void spawnCatcher(int edge)
    {
        Entity catcher = createEntity();

        float width  = 16.0f;
        float height = 96.0f;

        float catcherX = 0.0f;
        float catcherY = 0.0f;

        if (edge == 1)    // left center
        {
            catcherX = 20.0f;
            catcherY = screenHeight / 2.0f;
        }
        else              // right center
        {
            catcherX = screenWidth - 20.0f;
            catcherY = screenHeight / 2.0f;
        }

        componentsPool<Catcher>()   [catcher] = {};    // tag
        componentsPool<Thruster>()  [catcher] = {};    // emits exhaust while flying
        componentsPool<Position>()  [catcher] = { catcherX, screenHeight + 80.0f };    // start off-screen below
        componentsPool<MoveTarget>()[catcher] = { catcherX, catcherY };                // fly to the real spot
        componentsPool<Velocity>()  [catcher] = { 0, 0 };
        componentsPool<MoveSpeed>() [catcher] = { 0 };
        componentsPool<Size>()      [catcher] = { width, height };
        componentsPool<Color>()     [catcher] = Fade(PURPLE, 0.3f);
    }

    //----------------------------------------------------------------------------------
    // Update Systems Definition (decide and mutate, never draw)
    // Listed in the order UpdateGameplayScreen calls them
    //----------------------------------------------------------------------------------

    // Left click / tap gives every ship a move target (if its a single its precise, otherwise we just stagger em)
    void inputSystem()
    {
        if (CheckCollisionPointRec(GetMousePosition(), upgradeButtonRect)
            || showUpgradePanel
            || (mobilityLevel >= 1 && CheckCollisionPointRec(GetMousePosition(), autoNavButtonRect))
            || (technologyLevel >= 5 && CheckCollisionPointRec(GetMousePosition(), megaLaserButtonRect())))
        {
            return;
        }

        if (mobilityLevel >= 1 && autoNavEnabled)
        {
            return; // auto-pilot bought - hail to the automation king, baby!
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsGestureDetected(GESTURE_TAP))
        {
            Vector2 mousePos = GetMousePosition(); // After some 'light reading' I think I discovered this counts as a 'tap' too?

            float targetX = Clamp(mousePos.x + UTILS::getRandomFloat(-300.0f, 300.0f), 20.0f, screenWidth - 20.0f);
            float targetY = Clamp(mousePos.y + UTILS::getRandomFloat(-300.0f, 300.0f), 20.0f, screenHeight - 20.0f);

            for (auto& [entity, ship] : componentsPool<Ship>())
            {
                if (componentsPool<Ship>().size() > 1)
                {
                    componentsPool<MoveTarget>()[entity] = { targetX + UTILS::getRandomFloat(-300.0f, 300.0f), targetY + UTILS::getRandomFloat(-300.0f, 300.0f) };
                }
                else
                {
                    componentsPool<MoveTarget>()[entity] = { mousePos.x, mousePos.y };
                }
            }

            PARTICLES::spawnBurst(mousePos.x, mousePos.y, 10, SKYBLUE);    // confirm the order landed
        }
    }

    // Steer toward the move target: lerp up to speed on takeoff, clamp down by
    // distance to brake, drop the target on arrival (no MoveTarget = parked)
    void moveToTargetSystem()
    {
        std::vector<Entity> arrivedShips;   // collected here, erased after the loop (dont erase mid-iteration haha)

        for (auto& [entity, target] : componentsPool<MoveTarget>())
        {
            Position &position   = getComponent<Position>(entity);
            Velocity &velocity   = getComponent<Velocity>(entity);
            MoveSpeed &moveSpeed = getComponent<MoveSpeed>(entity);

            float dx = target.x - position.x;
            float dy = target.y - position.y;

            float distance = sqrtf(dx*dx + dy*dy);

            float effectiveShipSpeed = shipSpeed + fmaxf(0.0f, fminf(mobilityLevel - 1, 2)) * 50.0f;
            float desiredSpeed       = fminf(effectiveShipSpeed, distance * 2.0f);

            if (distance < 2.0f)    // close enough = arrived
            {
                velocity = { 0, 0 };
                arrivedShips.push_back(entity);
            }
            else                    // still flying
            {
                moveSpeed.speed = Lerp(moveSpeed.speed, desiredSpeed, shipAccelerationRate);
                moveSpeed.speed = fminf(moveSpeed.speed, desiredSpeed);
                velocity = { dx / distance * moveSpeed.speed, dy / distance * moveSpeed.speed };
            }
        }

        for (Entity e : arrivedShips)
        {
            componentsPool<MoveTarget>().erase(e);
        }
    }

    // Parked ships drift up and down cause it feels weird if they are just completely still lol
    void idleDriftSystem()
    {
        for (auto& [entity, ship] : componentsPool<Ship>())
        {
            if (componentsPool<MoveTarget>().count(entity))
            {
                continue;    // flying = no bob
            }

            Velocity &velocity = getComponent<Velocity>(entity);

            velocity.ySpeed = sinf((float)GetTime() * 1.5f + (float)entity) * 4.0f;
        }

        for (auto& [entity, catcher] : componentsPool<Catcher>())
        {
            if (componentsPool<MoveTarget>().count(entity))
            {
                continue;    // still flying in
            }

            Velocity &velocity = getComponent<Velocity>(entity);

            velocity.ySpeed = sinf((float)GetTime() * 1.0f + (float)entity) * 2.0f;
        }
    }

    // Point sprites the way they are flying
    // Ignores near-zero speeds so the idle bob cant flicker-flip anything
    void facingSystem()
    {
        for (auto& [entity, facing] : componentsPool<Facing>())
        {
            Velocity &velocity = getComponent<Velocity>(entity);

            if (velocity.xSpeed > 10.0f)
            {
                facing.direction = -1.0f;    // flying right = mirrored
            }

            if (velocity.xSpeed < -10.0f)
            {
                facing.direction = 1.0f;     // flying left = as drawn
            }
        }
    }

    // Slow constant rotation for anything with a Spin
    void spinSystem()
    {
        for (auto& [entity, spin] : componentsPool<Spin>())
        {
            SpriteVariant &variant = getComponent<SpriteVariant>(entity);

            variant.rotation += spin.degreesPerSecond * GetFrameTime();
        }
    }

    // When the far planet drifts off the left edge, send it around for another orbit
    void farPlanetOrbitSystem()
    {
        for (auto& [entity, planet] : componentsPool<Planet>())
        {
            if (componentsPool<Velocity>().count(entity) == 0)
            {
                continue;    // home planet doesnt move
            }

            Position &position = getComponent<Position>(entity);

            if (position.x < -80.0f)
            {
                position.x = screenWidth + UTILS::getRandomFloat(200.0f, 1200.0f);    // random gap = "occasional"
                position.y = UTILS::getRandomFloat(60.0f, 250.0f);
            }
        }
    }

    // Apply velocity to position for everything that moves (ships and trash alike)
    void movementSystem()
    {
        for (auto& [entity, velocity] : componentsPool<Velocity>())
        {
            Position &position = getComponent<Position>(entity);

            position.x += velocity.xSpeed * GetFrameTime(); // deltaTime (reminder because im a silly goose)
            position.y += velocity.ySpeed * GetFrameTime();
        }
    }

    // Keep turrets glued to their ship at the mount offset; orphaned turrets die
    void turretFollowSystem()
    {
        for (auto& [entity, mount] : componentsPool<TurretMount>())
        {
            if (componentsPool<Position>().count(mount.mountedOnShip) == 0)
            {
                markForRemoval(entity);
                continue;
            }

            Position &shipPosition   = getComponent<Position>(mount.mountedOnShip);
            Position &turretPosition = getComponent<Position>(entity);
            Facing   &shipFacing     = getComponent<Facing>(mount.mountedOnShip);

            turretPosition.x = shipPosition.x + mount.offsetX * shipFacing.direction;
            turretPosition.y = shipPosition.y + mount.offsetY;
        }
    }

    // Auto Nav: every so often the fleet picks new random spots to park at
    void shipAutoMovementSystem()
    {
        if (mobilityLevel < 1 || !autoNavEnabled)
        {
            return;
        }

        static float timeUntilNextMove = UTILS::getRandomFloat(shipMoveIntervalMin, shipMoveIntervalMax);

        timeUntilNextMove -= GetFrameTime();

        if (timeUntilNextMove <= 0.0f)
        {
            for (auto& [shipEntity, ship] : componentsPool<Ship>())
            {
                componentsPool<MoveTarget>()[shipEntity] = { UTILS::getRandomFloat(20.0f, screenWidth - 20.f), UTILS::getRandomFloat(20.0f, screenHeight - 20.f) };
            }

            timeUntilNextMove = UTILS::getRandomFloat(shipMoveIntervalMin, shipMoveIntervalMax);
        }
    }

    // Spawn trash on a timer while the field is under the cap
    // Spawn count scales with kill power so the late game turns into a firehose
    void asteroidSpawnSystem()
    {
        static float timeUntilNextSpawn = asteroidSpawnInterval;

        timeUntilNextSpawn -= GetFrameTime();

        int asteroidCount = componentsPool<Asteroid>().size();

        if (timeUntilNextSpawn <= 0.0f && asteroidCount < maxAsteroids)
        {
            int spawnCount = 1 + (weaponsLevel + automationLevel) * 4;   // maxed = 49/tick, PIXELS EVERYWHERE

            for (int i = 0; i < spawnCount; i++)
            {
                if (componentsPool<Asteroid>().size() >= maxAsteroids)
                {
                    break;
                }

                spawnAsteroid();
            }

            timeUntilNextSpawn = asteroidSpawnInterval;
        }
    }

    // Remove trash that drifted past the screen edges
    // NOTE( LOL ): kill margin (100px) must stay BIGGER than the spawn band (20px) or asteroids die at birth xD
    // I wonder why theres no asteroids at this point....
    void asteroidDespawnSystem()
    {
        for (auto& [entity, asteroid] : componentsPool<Asteroid>())
        {
            Position &position = getComponent<Position>(entity);

            if (position.x <= -100.0f || position.x >= screenWidth + 100.0f || position.y <= -100.0f || position.y >= screenHeight + 100.0f)
            {
                markForRemoval(entity);
            }
        }
    }

    // Every turret on a parked ship zaps the first asteroid in radius, on cooldown
    // High weapons levels chain the zap to nearby trash
    void miningLaserSystem()
    {
        for (auto& [turretEntity, mount] : componentsPool<TurretMount>())
        {
            LaserBeam &laserBeam = getComponent<LaserBeam>(turretEntity);
            laserBeam.timeLeft  -= GetFrameTime();

            if (componentsPool<MoveTarget>().count(mount.mountedOnShip) == 0) // ship is 'parked'!
            {
                Position &turretPosition = getComponent<Position>(turretEntity);
                Cooldown &turretCooldown = getComponent<Cooldown>(turretEntity);

                turretCooldown.timeUntilReady -= GetFrameTime();

                if (turretCooldown.timeUntilReady > 0.0f)
                {
                    continue;
                }

                for (auto& [asteroidEntity, asteroid] : componentsPool<Asteroid>())
                {
                    if (componentsPool<DeathTimer>().count(asteroidEntity))
                    {
                        continue; // Already dying - not a target!
                    }

                    Position &asteroidPosition = getComponent<Position>(asteroidEntity);

                    float dx = asteroidPosition.x - turretPosition.x;
                    float dy = asteroidPosition.y - turretPosition.y;

                    float distance = sqrtf(dx*dx + dy*dy);

                    // Fire the frickin laser!
                    if (distance < effectiveLaserRadius())
                    {
                        zapAsteroid(asteroidEntity, turretPosition.x, turretPosition.y);

                        laserBeam = { asteroidPosition.x, asteroidPosition.y, shipLaserBeamDuration, asteroidEntity };

                        turretCooldown.timeUntilReady = laserCooldown - fminf((float)weaponsLevel, 2.0f) * 0.5f;

                        int chainsLeft = (int)fmaxf(0.0f, (weaponsLevel - 4.0f) * 2.0f);    // node 5 = 2 chains

                        if (weaponsLevel >= 6)
                        {
                            chainsLeft *= 2;    // second chain node doubles them = 8
                        }

                        for (auto& [chainEntity, chainTag] : componentsPool<Asteroid>())
                        {
                            if (chainsLeft <= 0)
                            {
                                break;
                            }

                            if (chainEntity == asteroidEntity)
                            {
                                continue; // already zapped
                            }

                            if (componentsPool<DeathTimer>().count(chainEntity))
                            {
                                continue; // already dying
                            }

                            Position &chainPosition = getComponent<Position>(chainEntity);

                            float chainDx = chainPosition.x - asteroidPosition.x;
                            float chainDy = chainPosition.y - asteroidPosition.y;

                            if (sqrtf(chainDx * chainDx + chainDy * chainDy) < 120.0f)
                            {
                                zapAsteroid(chainEntity, asteroidPosition.x, asteroidPosition.y);
                                chainsLeft--;
                            }
                            SetSoundPitch(fxPop, UTILS::getRandomFloat(0.6f, 1.6f));
                            PlaySound(fxPop);
                        }

                        break;
                    }
                }
            }
        }
    }

    // Zap one asteroid: spawn a beam entity from the source point, deal damage,
    // pay out and start the delayed death on a kill
    void zapAsteroid(Entity asteroidEntity, float sourceX, float sourceY)
    {
        Position   &asteroidPosition   = getComponent<Position>(asteroidEntity);
        Health     &asteroidHealth     = getComponent<Health>(asteroidEntity);
        SizeTier   &asteroidTier       = getComponent<SizeTier>(asteroidEntity);
        PointColor &asteroidPointColor = getComponent<PointColor>(asteroidEntity);

        Entity beam = createEntity();

        componentsPool<Position>() [beam] = { sourceX, sourceY };
        componentsPool<LaserBeam>()[beam] = { asteroidPosition.x, asteroidPosition.y, shipLaserBeamDuration, asteroidEntity };

        float effectiveLaserDamage = shipLaserDamage + fmaxf(0.0f, fminf(weaponsLevel - 2.0f, 2.0f)) * 5.0f;

        asteroidHealth.hp -= effectiveLaserDamage;


        if (asteroidHealth.hp <= 0)
        {
            payOut(asteroidPointColor.value, asteroidTier.tier, asteroidPosition.x, asteroidPosition.y);

            componentsPool<DeathTimer>()[asteroidEntity] = { shipLaserBeamDuration * 0.7f };
        }
    }

    // Expand the blast ring from screen center, because f*** space trash!
    // It does what a screen nuke has historically always done lol
    void nukeWaveSystem()
    {
        if (nukeWaveRadius < 0.0f)
        {
            return;    // no wave running
        }

        nukeWaveRadius += nukeWaveSpeed * GetFrameTime();

        for (auto& [asteroidEntity, asteroid] : componentsPool<Asteroid>())
        {
            if (componentsPool<DeathTimer>().count(asteroidEntity))
            {
                continue;    // already dying
            }

            Position &position = getComponent<Position>(asteroidEntity);

            float dx = position.x - screenWidth  / 2.0f;
            float dy = position.y - screenHeight / 2.0f;

            if (sqrtf(dx*dx + dy*dy) > nukeWaveRadius)
            {
                continue;    // wave hasnt reached it yet
            }

            Color      &color      = getComponent<Color>(asteroidEntity);
            SizeTier   &sizeTier   = getComponent<SizeTier>(asteroidEntity);
            PointColor &pointColor = getComponent<PointColor>(asteroidEntity);

            payOut(pointColor.value, sizeTier.tier, position.x, position.y);

            PARTICLES::spawnBurst(position.x, position.y, 16, color);

            componentsPool<DeathTimer>()[asteroidEntity] = { shipLaserBeamDuration * 0.7f };
        }

        if (nukeWaveRadius > nukeWaveMaxRadius)
        {
            nukeWaveRadius = -1.0f;    // swept the whole screen, all done

            if (!unlimitedMode)
            {
                campaignComplete = true;    // the gameplay screen sees this and rolls credits
            }
        }
    }

    // Light the fuse: nukeWaveSystem takes it from here
    void fireNuke()
    {
        nukeWaveRadius = 1.0f;

        PlaySound(fxCatcherDeploy);
    }

    // Tick dying trash, burst and remove it once the timer expires
    // It outlives the killing blow so the laser has something to point at while it fades
    void deathTimerSystem()
    {
        for (auto& [deathEntity, deathTimer] : componentsPool<DeathTimer>())
        {
            deathTimer.timeLeft -= GetFrameTime();

            if (deathTimer.timeLeft <= 0)
            {
                Position &position = getComponent<Position>(deathEntity);
                Color    &color    = getComponent<Color>(deathEntity);

                PARTICLES::spawnBurst(position.x, position.y, 24, color);
                SetSoundPitch(fxPop, UTILS::getRandomFloat(0.75f, 1.25f));
                PlaySound(fxPop);

                markForRemoval(deathEntity);
            }
        }
    }

    // Catchers eat any trash that touches them - instant payout, no laser required
    void catcherCollisionSystem()
    {
        for (auto& [catcherEntity, catcher] : componentsPool<Catcher>())
        {
            Position &catcherPosition = getComponent<Position>(catcherEntity);
            Size     &catcherSize     = getComponent<Size>(catcherEntity);

            Rectangle catcherRect = { catcherPosition.x - catcherSize.width / 2, catcherPosition.y - catcherSize.height / 2, catcherSize.width, catcherSize.height };

            for (auto& [asteroidEntity, asteroid] : componentsPool<Asteroid>())
            {
                if (componentsPool<DeathTimer>().count(asteroidEntity))
                {
                    continue;
                }

                SizeTier   &asteroidTier       = getComponent<SizeTier>(asteroidEntity);
                PointColor &asteroidPointColor = getComponent<PointColor>(asteroidEntity);
                Position   &asteroidPosition   = getComponent<Position>(asteroidEntity);

                float side = asteroidTier.tier * blockSize;

                Rectangle asteroidRect = { asteroidPosition.x - side / 2, asteroidPosition.y - side / 2, side, side };

                if (CheckCollisionRecs(catcherRect, asteroidRect))
                {
                    payOut(asteroidPointColor.value, asteroidTier.tier, asteroidPosition.x, asteroidPosition.y);

                    markForRemoval(asteroidEntity);
                }
            }
        }
    }

    // All the buy/toggle/fire clicks: panel toggle, Auto Nav, the four tech rows,
    // Mega Laser purchases and the ARM/LAUNCH two-step
    void upgradePanelInputSystem()
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), upgradeButtonRect))
        {
            showUpgradePanel = !showUpgradePanel;
            SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
            PlaySound(fxSelection);
        }

        if (mobilityLevel >= 1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), autoNavButtonRect))
        {
            autoNavEnabled = !autoNavEnabled;
            SetSoundPitch(fxSelection, UTILS::getRandomFloat(0.75f, 1.25f));
            PlaySound(fxSelection);
        }

        bool clickedFirepower  = showUpgradePanel && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), firepowerRowRect);
        bool clickedMobility   = showUpgradePanel && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), shipRowRect);
        bool clickedAutomation = showUpgradePanel && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), automationRowRect);
        bool clickedTechnology = showUpgradePanel && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), colorsRowRect);

        if (clickedFirepower)
        {
            int upgradeCost = (weaponsLevel + 1) * 3;

            if (technologyLevel >= 1 && weaponsLevel < 6 && redPoints >= upgradeCost)
            {
                redPoints -= upgradeCost;
                weaponsLevel++;
                SetSoundPitch(fxUpgrade, UTILS::getRandomFloat(0.75f, 1.25f));
                PlaySound(fxUpgrade);
                PARTICLES::spawnBurst(GetMousePosition().x, GetMousePosition().y, 30, trashRed, true);
            }
        }

        if (clickedMobility)
        {
            int upgradeCost = (mobilityLevel + 1) * 5;

            if (technologyLevel >= 3 && mobilityLevel < 6 && bluePoints >= upgradeCost)
            {
                bluePoints -= upgradeCost;
                mobilityLevel++;

                if (mobilityLevel == 1)
                {
                    autoNavEnabled = true;
                }

                SetSoundPitch(fxUpgrade, UTILS::getRandomFloat(0.75f, 1.25f));
                PlaySound(fxUpgrade);
                PARTICLES::spawnBurst(GetMousePosition().x, GetMousePosition().y, 30, trashBlue, true);
            }
        }

        if (clickedAutomation)
        {
            int upgradeCost = (automationLevel + 1) * 5;

            if (technologyLevel >= 2 && automationLevel < 6 && greenPoints >= upgradeCost)
            {
                greenPoints -= upgradeCost;
                automationLevel++;
                SetSoundPitch(fxUpgrade, UTILS::getRandomFloat(0.75f, 1.25f));
                PlaySound(fxUpgrade);
                PARTICLES::spawnBurst(GetMousePosition().x, GetMousePosition().y, 30, trashGreen, true);

                if (automationLevel <= 2)
                {
                    spawnCatcher(automationLevel);
                    SetSoundPitch(fxCatcherDeploy, UTILS::getRandomFloat(0.75f, 1.25f));
                    PlaySound(fxCatcherDeploy);
                }
                else if (automationLevel <= 4)
                {
                    growCatchers();
                }
                else if (automationLevel <= 6)
                {
                    spawnShip();
                    SetSoundPitch(fxCatcherDeploy, UTILS::getRandomFloat(0.75f, 1.25f));
                    PlaySound(fxCatcherDeploy);
                }
            }
        }

        if (clickedTechnology)
        {
            int upgradeCost = (technologyLevel < 5) ? technologyNodeCosts[technologyLevel] : nukeCost;

            if (technologyLevel < 5)
            {
                if (hexPoints >= upgradeCost)
                {
                    hexPoints -= upgradeCost;
                    technologyLevel++;

                    for (auto& [shipEntity, shipTag] : componentsPool<Ship>())
                    {
                        if (technologyLevel >= 4)
                        {
                            spawnTurret(shipEntity, 8, 0);
                            SetSoundPitch(fxCatcherDeploy, UTILS::getRandomFloat(0.75f, 1.25f));
                            PlaySound(fxCatcherDeploy);
                        }

                        if (technologyLevel >= 5)
                        {
                            spawnTurret(shipEntity, 24, 0);
                            SetSoundPitch(fxCatcherDeploy, UTILS::getRandomFloat(0.75f, 1.25f));
                            PlaySound(fxCatcherDeploy);
                        }
                    }
                    SetSoundPitch(fxUpgrade, UTILS::getRandomFloat(0.75f, 1.25f));
                    PlaySound(fxUpgrade);
                    PARTICLES::spawnBurst(GetMousePosition().x, GetMousePosition().y, 30, GRAY, true);
                }
            }
            else if (hexPoints >= nukeCost)    // maxed tree sells nukes I guess wtf haha
            {
                // Campaign rule: the Mega Laser is the ENDING - every other tree must be maxed first
                bool everythingElseMaxed  = (weaponsLevel >= 6 && mobilityLevel >= 6 && automationLevel >= 6);
                bool campaignAlreadyOwned = (!unlimitedMode && nukesOwned > 0);    // one Mega Laser per campaign

                if ((unlimitedMode || everythingElseMaxed) && !campaignAlreadyOwned)
                {
                    hexPoints -= nukeCost;
                    nukesOwned++;
                    SetSoundPitch(fxUpgrade, UTILS::getRandomFloat(0.75f, 1.25f));
                    PlaySound(fxUpgrade);
                    PARTICLES::spawnBurst(GetMousePosition().x, GetMousePosition().y, 50, GOLD, true);
                }
                else if (!unlimitedMode && !everythingElseMaxed)
                {
                    megaLaserDeniedTimer = 2.5f;    // tell them why the buy bounced
                }
            }
        }

        megaLaserDeniedTimer = fmaxf(0.0f, megaLaserDeniedTimer - GetFrameTime());

        // Fire button next to Auto Nav: unlimited fires instantly, campaign is a two-step ARM then LAUNCH
        if (technologyLevel >= 5 && nukesOwned > 0 && nukeWaveRadius < 0.0f && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), megaLaserButtonRect()))
        {
            if (unlimitedMode)
            {
                nukesOwned--;
                fireNuke();
            }
            else if (!megaLaserArmed)
            {
                megaLaserArmed = true;
                SetSoundPitch(fxUpgrade, UTILS::getRandomFloat(0.75f, 1.25f));
                PlaySound(fxUpgrade);
            }
            else
            {
                nukesOwned--;
                megaLaserArmed = false;
                fireNuke();
            }
        }
    }

    // Beam entities: tick down, track their target while it lives, die at zero
    // Ships and turrets also carry a LaserBeam as aim memory - those are skipped
    void laserBeamExpirySystem()
    {
        for (auto& [beamEntity, laserBeam] : componentsPool<LaserBeam>())
        {
            if (componentsPool<Ship>().count(beamEntity))
            {
                continue; // Ships aim memory
            }

            if (componentsPool<TurretMount>().count(beamEntity))
            {
                continue; // Turrets aim memory
            }

            laserBeam.timeLeft -= GetFrameTime();

            if (laserBeam.timeLeft <= 0.0f)
            {
                markForRemoval(beamEntity);
                
            }

            if (laserBeam.timeLeft > 0.0f && componentsPool<Asteroid>().count(laserBeam.targetEntity))
            {
                Position &targetPosition = getComponent<Position>(laserBeam.targetEntity);

                laserBeam.targetX = targetPosition.x;
                laserBeam.targetY = targetPosition.y;
            }
        }
    }

    //----------------------------------------------------------------------------------
    // Upgrade Helper Functions Definition
    //----------------------------------------------------------------------------------

    // Ending cinematic: point the whole fleet at the home planet (past the bottom-right corner)
    void sendFleetHome()
    {
        for (auto& [entity, ship] : componentsPool<Ship>())
        {
            componentsPool<MoveTarget>()[entity] = { screenWidth + 80.0f + UTILS::getRandomFloat(0.0f, 60.0f), screenHeight + 80.0f + UTILS::getRandomFloat(0.0f, 60.0f) };
            
            PlaySound(fxCatcherDeploy);
        }
    }

    // True once every ship has cleared the screen edges
    bool allShipsOffScreen()
    {
        for (auto& [entity, ship] : componentsPool<Ship>())
        {
            Position &position = getComponent<Position>(entity);

            if (position.x < screenWidth + 40.0f && position.y < screenHeight + 40.0f)
            {
                return false;    // this one is still visible
            }
        }

        return true;
    }

    // Pay a kill's bounty into the currency matching its "hex" color, popup included
    void payOut(Currency pointColor, uint8_t tier, float x, float y)
    {
        Color popupColor = GRAY;

        switch (pointColor)
        {
            case Currency::CURRENCY_RED:
                redPoints += tier;
                popupColor = trashRed;
                break;

            case Currency::CURRENCY_GREEN:
                greenPoints += tier;
                popupColor = trashGreen;
                break;

            case Currency::CURRENCY_BLUE:
                bluePoints += tier;
                popupColor = trashBlue;
                break;

            default:
                hexPoints += tier;
                break;
        }

        PARTICLES::spawnPopup(x, y, tier, popupColor);
    }

    // The laser's true reach: base radius plus blue upgrades
    float effectiveLaserRadius()
    {
        return shipLaserRadius + fmaxf(0.0f, fminf(mobilityLevel - 3, 3)) * 25.0f;
    }

    // Make sure to grow them in the right direction, we dont want them to get 'thicker', just 'wider'
    void growCatchers()
    {
        for (auto& [entity, catcher] : componentsPool<Catcher>())
        {
            Size &size = getComponent<Size>(entity);

            if (size.width > size.height)
            {
                size.width *= 1.5f;
            }
            else
            {
                size.height *= 1.5f;
            }
        }
    }

    //----------------------------------------------------------------------------------
    // Draw Systems Definition (only look, never mutate)
    // Listed in paint order - same order DrawGameplayScreen calls them
    //----------------------------------------------------------------------------------

    // Faint twinkling stars first, then the planets on top of them
    void drawBackground()
    {
        for (auto& [entity, star] : componentsPool<Star>())
        {
            Position &position = getComponent<Position>(entity);

            // Each star breathes around its own base brightness
            float twinkle = 0.15f + 0.08f * sinf((float)GetTime() * 0.8f + star.twinklePhase);

            DrawRectangleRec({ position.x, position.y, star.size, star.size }, Fade(WHITE, twinkle));
        }

        for (auto& [entity, planet] : componentsPool<Planet>())
        {
            Position      &position = getComponent<Position>(entity);
            Sprite        &sprite   = getComponent<Sprite>(entity);
            Size          &size     = getComponent<Size>(entity);
            SpriteVariant &variant  = getComponent<SpriteVariant>(entity);
            Color         &color    = getComponent<Color>(entity);

            Rectangle source      = { 0.0f, 0.0f, (float)sprite.sprite.width, (float)sprite.sprite.height };
            Rectangle destination = { position.x, position.y, size.width, size.height };
            Vector2   origin      = { size.width / 2, size.height / 2 };

            DrawTexturePro(sprite.sprite, source, destination, origin, variant.rotation, color);
        }
    }

    // Space trash: sprite picked by tier + variant, tinted by its point color
    void drawAsteroids()
    {
        for (auto& [entity, asteroid] : componentsPool<Asteroid>())
        {
            Position      &position = getComponent<Position>(entity);
            SizeTier      &sizeTier = getComponent<SizeTier>(entity);
            Color         &color    = getComponent<Color>(entity);
            SpriteVariant &variant  = getComponent<SpriteVariant>(entity);

            float side = sizeTier.tier * blockSize;

            Texture2D &sprite = spaceTrashSprites[sizeTier.tier - 1][variant.index];

            Rectangle source      = { 0.0f, 0.0f, (float)sprite.width, (float)sprite.height };
            Rectangle destination = { position.x - side / 2, position.y - side / 2, side, side };
            Vector2   origin      = { side / 2, side / 2 };

            DrawTexturePro(sprite, source, destination, origin, variant.rotation, color);
        }
    }

    // Ships: negative source width mirrors the sprite when facing the other way
    void drawShips()
    {
        for (auto& [entity, ship] : componentsPool<Ship>())
        {
            Position &position = getComponent<Position>(entity);
            Facing   &facing   = getComponent<Facing>(entity);

            Rectangle shipSource      = { 0.0f, 0.0f, shipWidth * facing.direction, shipHeight };
            Rectangle shipDestination = { position.x - shipWidth / 2, position.y - shipHeight / 2, shipWidth, shipHeight };

            DrawTexturePro(shipTexture, shipSource, shipDestination, { 0, 0 }, 0.0f, WHITE);
        }
    }

    // Turrets sit at their mount positions, rotated toward their last laser target
    void drawTurrets()
    {
        for (auto& [entity, mount] : componentsPool<TurretMount>())
        {
            Position  &position  = getComponent<Position>(entity);
            LaserBeam &laserBeam = getComponent<LaserBeam>(entity);

            float angleDegrees = atan2f(laserBeam.targetY - position.y, laserBeam.targetX - position.x) * RAD2DEG;

            DrawTexturePro(turretTexture, { 0, 0, 24, 24 }, { position.x, position.y, 24, 24 }, { 12, 12 }, angleDegrees, WHITE);
        }
    }

    // Live laser beams: grow from turret to target over the first 20% of life, fade in/out
    // NOTE: everything is derived from timeLeft - the beam has no other state
    void drawLaserBeams()
    {
        for (auto& [entity, laserBeam] : componentsPool<LaserBeam>())
        {
            if (componentsPool<Ship>().count(entity))
            {
                continue;    // aim memory, not a beam
            }

            if (componentsPool<TurretMount>().count(entity))
            {
                continue;    // aim memory, not a beam
            }

            Position &turretPosition = getComponent<Position>(entity);

            if (laserBeam.timeLeft > 0.0f)
            {
                float beamAge     = 1.0f - laserBeam.timeLeft / shipLaserBeamDuration;    // 0 = just fired, 1 = gone
                float laserGrowth = fminf(1.0f, beamAge / 0.2f);

                float laserTipX = turretPosition.x + (laserBeam.targetX - turretPosition.x) * laserGrowth;
                float laserTipY = turretPosition.y + (laserBeam.targetY - turretPosition.y) * laserGrowth;

                float alpha = fminf(1.0f, fminf(beamAge / 0.2f, (1.0f - beamAge) / 0.3f));

                float widthBoost = weaponsLevel * 0.4f;    // fatter beam as firepower grows

                DrawLineEx({ turretPosition.x, turretPosition.y }, { laserTipX, laserTipY }, UTILS::getRandomFloat(1.0f, 3.0f) + widthBoost, Fade(laserPink, alpha));
                DrawLineEx({ turretPosition.x, turretPosition.y }, { laserTipX, laserTipY }, UTILS::getRandomFloat(0.0f, 1.0f) + widthBoost * 0.5f, Fade(WHITE, alpha));
            }
        }
    }

    // Catchers: one tileable strip, rotated to face incoming trash on its side of the screen
    void drawCatchers()
    {
        for (auto& [entity, catcher] : componentsPool<Catcher>())
        {
            Position &position = getComponent<Position>(entity);
            Size     &size     = getComponent<Size>(entity);

            float textureHeight = (float)catcherTexture.height;    // one full tile of the strip

            // The art faces the left edge as drawn; mirror it for the right-side catcher
            float sourceWidth = (position.x < screenWidth / 2.0f) ? size.width : -size.width;

            float topY        = position.y - size.height / 2;
            float drawnHeight = 0.0f;

            while (drawnHeight < size.height)
            {
                float segment = fminf(textureHeight, size.height - drawnHeight);    // last tile may be a partial

                Rectangle source      = { 0.0f, 0.0f, sourceWidth, segment };
                Rectangle destination = { position.x - size.width / 2, topY + drawnHeight, size.width, segment };

                DrawTexturePro(catcherTexture, source, destination, { 0, 0 }, 0.0f, WHITE);

                drawnHeight += segment;
            }
        }
    }

    // The expanding Mega Laser blast ring
    void drawNukeWave()
    {
        if (nukeWaveRadius < 0.0f)
        {
            return;
        }

        float waveProgress = nukeWaveRadius / nukeWaveMaxRadius;    // 0 = just fired, 1 = gone

        DrawRing({ screenWidth / 2.0f, screenHeight / 2.0f }, nukeWaveRadius - 6.0f, nukeWaveRadius, 0.0f, 360.0f, 64, Fade(BLUE, 1.0f - waveProgress));
        DrawRing({ screenWidth / 2.0f, screenHeight / 2.0f }, nukeWaveRadius - 2.0f, nukeWaveRadius, 0.0f, 360.0f, 64, Fade(SKYBLUE, 1.0f - waveProgress));
    }

    // The HUD bar: backdrop, comms-style corner brackets, one hexagon slot per unlocked currency
    // Hex points = tech currency, Red = weapons, Blue = mobility, Green = automation
    void drawData(int &hPoints, int &rPoints, int &bPoints, int &gPoints)
    {
        DrawRectangle(0, 0, (int)screenWidth, (int)hudBarHeight, hudBackgroundColor);
        DrawLineEx({ 0.0f, hudBarHeight }, { screenWidth, hudBarHeight }, 2.0f, screenBorderColor);

        // Corner brackets on the divider ends, echoing the briefing screen's comms frame
        float arm = 18.0f;

        DrawLineEx({ 2.0f, hudBarHeight }, { 2.0f + arm, hudBarHeight }, 3.0f, SKYBLUE);
        DrawLineEx({ 2.0f, hudBarHeight }, { 2.0f, hudBarHeight - arm }, 3.0f, SKYBLUE);
        DrawLineEx({ screenWidth - 2.0f, hudBarHeight }, { screenWidth - 2.0f - arm, hudBarHeight }, 3.0f, SKYBLUE);
        DrawLineEx({ screenWidth - 2.0f, hudBarHeight }, { screenWidth - 2.0f, hudBarHeight - arm }, 3.0f, SKYBLUE);

        // Slot order matches unlock order: hex, then red (tech 1), green (tech 2), blue (tech 3)
        int values[4]    = { hPoints, rPoints, gPoints, bPoints };
        Color colors[4]  = { GRAY, trashRed, trashGreen, trashBlue };
        bool unlocked[4] = { true, technologyLevel >= 1, technologyLevel >= 2, technologyLevel >= 3 };

        for (int i = 0; i < 4; i++)
        {
            if (!unlocked[i])
            {
                continue;
            }

            float slotX = 25.0f + i * 100.0f;    // one 80px slot per currency

            // Centered on the button row: buttons run y 10-40, so everything centers on y 25
            DrawPoly({ slotX, 25.0f }, 6, 10.0f, 0.0f, colors[i]);
            DrawText(TextFormat("%d", values[i]), (int)slotX + 20, 14, 22, colors[i]);
        }
    }

    // Top-right buttons: upgrade panel toggle, Auto Nav, and the Mega Laser fire button
    void drawUpgradeMenu()
    {
        if (showUpgradePanel)
        {
            drawUpgradePanel();
            drawButton(upgradeButtonRect, upgradeButtonBgColor, upgradeButtonTextColor, "Close [X]");
        }
        else
        {
            drawButton(upgradeButtonRect, upgradeButtonBgColor, upgradeButtonTextColor, upgradeButtonLabel);
        }

        if (mobilityLevel >= 1)
        {
            Color navColor        = autoNavEnabled ? DARKGREEN : DARKGRAY;
            Color navTxtColor     = autoNavEnabled ? LIME : LIGHTGRAY;
            const char *navButton = autoNavEnabled ? "Nav ON": "Nav OFF";

            drawButton(autoNavButtonRect, navColor, navTxtColor, navButton);
        }

        if (technologyLevel >= 5)
        {
            if (unlimitedMode)
            {
                Color nukeColor    = (nukesOwned > 0) ? MAROON : DARKGRAY;
                Color nukeTxtColor = (nukesOwned > 0) ? WHITE    : GRAY;

                // Hexagon icon + charge count, same look as the HUD currency slots
                DrawRectangleRec(nukeButtonRect, nukeColor);
                DrawPoly({ nukeButtonRect.x + 10.0f, nukeButtonRect.y + 16.0f }, 6, 7.0f, 0.0f, nukeTxtColor);
                DrawText(TextFormat("%d", nukesOwned), (int)nukeButtonRect.x + 21, (int)nukeButtonRect.y + 6, hudButtonFontSize, nukeTxtColor);
            }
            else if (nukesOwned > 0)    // campaign: the button only exists once the Mega Laser is bought
            {
                Color buttonColor = megaLaserArmed ? MAROON : Fade(GOLD, 0.4f);
                Color textColor   = megaLaserArmed ? WHITE    : GOLD;

                const char *fireLabel = megaLaserArmed ? "FIRE" : "ARM";

                DrawRectangleRec(sweepButtonWideRect, buttonColor);
                DrawText(fireLabel, (int)(sweepButtonWideRect.x + (sweepButtonWideRect.width - MeasureText(fireLabel, hudButtonFontSize)) / 2), (int)sweepButtonWideRect.y + 6, hudButtonFontSize, textColor);
            }
        }
    }

    // The upgrade panel: four tech rows plus the gold Mega Laser node
    void drawUpgradePanel()
    {
        DrawRectangleRec(upgradePanelRect, upgradePanelBgColor);

        int technologyCost = (technologyLevel < 5) ? technologyNodeCosts[technologyLevel] : nukeCost;    // maxed tree sells nukes I guess wtf haha

        drawTechRow("Technology", technologyLevel, 6, technologyNodeNames, technologyCost, 100.0f, GRAY, LIGHTGRAY, true);

        // The Mega Laser node: gold once reachable, grayed out after the campaign's one purchase
        if (technologyLevel >= 5)
        {
            bool soldOut = (!unlimitedMode && nukesOwned > 0);

            DrawPoly({ 130.0f + 5 * 90.0f, 170.0f }, 6, 30.0f, 0.0f, soldOut ? upgradePanelLockedColor : GOLD);

            if (!soldOut)
            {
                DrawText("BUY", (int)(130.0f + 5 * 90.0f) - 20, 160, 20, BLACK);
            }
        }

        drawTechRow("Firepower",  weaponsLevel,    6, weaponNodeNames,     (weaponsLevel    + 1) * 3, 230.0f, RED,   MAROON,  technologyLevel >= 1);
        drawTechRow("Automation", automationLevel, 6, automationNodeNames, (automationLevel + 1) * 5, 360.0f, GREEN, LIME,    technologyLevel >= 2);
        drawTechRow("Mobility",   mobilityLevel,   6, mobilityNodeNames,   (mobilityLevel   + 1) * 5, 490.0f, BLUE,  SKYBLUE, technologyLevel >= 3);
    }

    // One tree row: header text, next-node name, and a hexagon per node
    // Bought nodes get the row color, the next one is buyable, the rest stay locked-gray
    void drawTechRow(const char *label, int level, int nodeCount, const char **nodeNames, int cost, float rowY, Color boughtColor, Color buyableColor, bool unlocked)
    {
        if (!unlocked)
        {
            DrawText(TextFormat("%s  LOCKED", label), 100, rowY - 5, 20, upgradePanelLockedColor);

            for (int i = 0; i < nodeCount; i++)
            {
                DrawPoly({ 130.0f + i * 90.0f, rowY + 70.0f }, 6, 30.0f, 0.0f, upgradePanelLockedColor);
            }

            return;
        }

        DrawText(TextFormat("%s  Lv %d  Cost: %d", label, level, cost), 100, rowY - 5, 20, boughtColor);

        const char *nextUpgradeName = (level < nodeCount) ? nodeNames[level] : "MAXED";

        DrawText(nextUpgradeName, 100, rowY + 15, 20, boughtColor);

        for (int i = 0; i < nodeCount; i++)
        {
            Color hexColor = upgradePanelLockedColor;

            if (i < level)
            {
                hexColor = boughtColor;      // bought
            }
            else if (i == level)
            {
                hexColor = buyableColor;     // next one buyable
            }

            DrawPoly({ 130.0f + i * 90.0f, rowY + 70.0f }, 6, 30.0f, 0.0f, hexColor);

            if (i == level)
            {
                DrawText("BUY", (int)(130.0f + i * 90.0f) - 20, (int)rowY + 60, 20, BLACK);
            }
        }
    }

    // A plain rectangular button with a properly centered label
    void drawButton(Rectangle buttRect, Color btnColor, Color txtColor, const char *label)
    {
        DrawRectangleRec(buttRect, btnColor);
        DrawText(label,
                 (int)(buttRect.x + (buttRect.width - MeasureText(label, hudButtonFontSize)) / 2),
                 (int)(buttRect.y + (buttRect.height - hudButtonFontSize) / 2),
                 hudButtonFontSize, txtColor);
    }

    // Campaign guidance: once everything is maxed, point the player at the exit
    void drawFinalSweepPrompt()
    {
        if (unlimitedMode)
        {
            return;
        }

        // Bounced buy: explain the rule for a couple of seconds
        if (megaLaserDeniedTimer > 0.0f)
        {
            const char *denied = "Upgrade everything else before the Mega Laser!";
            DrawText(denied, (int)(screenWidth - MeasureText(denied, 20)) / 2, (int)screenHeight - 70, 20, RED);
        }

        bool everythingMaxed = (weaponsLevel >= 6 && mobilityLevel >= 6 && automationLevel >= 6 && technologyLevel >= 5);

        if (!everythingMaxed)
        {
            return;
        }

        const char *prompt = (nukesOwned == 0) ? "Buy the Mega Laser!" : "Launch the Mega Laser so we can get out of here!";

        float pulse = 0.65f + 0.35f * sinf((float)GetTime() * 3.0f);

        DrawText(prompt, (int)(screenWidth - MeasureText(prompt, 20)) / 2, (int)screenHeight - 40, 20, Fade(GOLD, pulse));
    }

    // Replace the mouse cursor with a target reticle over the playfield (normal cursor on the HUD row)
    void drawTargetCursor()
    {
        Vector2 mouse = GetMousePosition();

        if (mouse.y < hudBarHeight || showUpgradePanel)
        {
            ShowCursor();
            return;
        }

        HideCursor();

        DrawCircleLines(mouse.x, mouse.y, 10.0f, RED);
        DrawLine(mouse.x - 14, mouse.y, mouse.x - 6, mouse.y, RED);     // left tick
        DrawLine(mouse.x + 6,  mouse.y, mouse.x + 14, mouse.y, RED);    // right tick
        DrawLine(mouse.x, mouse.y - 14, mouse.x, mouse.y - 6, RED);     // top tick
        DrawLine(mouse.x, mouse.y + 6,  mouse.x, mouse.y + 14, RED);    // bottom tick
    }

    //----------------------------------------------------------------------------------
    // Debug Functions Definition
    //----------------------------------------------------------------------------------

    // Give yourself MO MONEYYYYYY (500 of each point)
    void addPointsDebug(int &hPoints, int &rPoints, int &bPoints, int &gPoints)
    {
        hPoints = 500;
        rPoints = 500;
        bPoints = 500;
        gPoints = 500;
    }
}
