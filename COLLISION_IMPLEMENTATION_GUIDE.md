# Terrain collision implementation guide

**Status:** done. Every step is complete (24 September 2026).

You write the game code. I explain each step, help you reason through it, and review what you write. Work through this guide one step at a time; finish the checkpoint before moving on. Creating this document does not carry out any of the implementation steps.

## What we are building

A reusable system that moves rectangular actor bodies through static, solid rectangular terrain. It must handle floors, walls, ceilings, corners, and fast movement for both Aeris and spiders.

The decisions for this first version are:

- Terrain is a set of static rectangles, each solid on all four sides.
- Aeris keeps the existing walking, jumping, sprinting, and attack tuning.
- Terrain correction preserves the active attack and its animation progress. Position the attack hitbox from the corrected body position before checking hits.
- A wall stops dash movement; the attack and airborne freeze finish normally.
- Death immediately cancels the attack and makes its hitbox inactive. Respawning selects the standing animation immediately, with movement and attack state reset.
- Enemies gain gravity and fall off ledges. The spider moves continuously in its facing direction and reverses on a horizontal terrain contact; fixed patrol intervals are not used.
- Enemy damage, attack hits, and note collection remain gameplay overlap checks.
- One-way platforms, slopes, rotated shapes, moving platforms, physical pushing between actors, and map-based entity spawning come later.

The current variable frame time and its 0.05-second cap stay in place. We will use swept collision, which checks the path of movement, to prevent passing through thin obstacles.

## Responsibilities

| Component | Responsibility |
| --- | --- |
| `TerrainCollision` — new | Store terrain rectangles and resolve a body's requested movement. |
| `World` | Own the terrain, supply it to actors, and coordinate gameplay interactions. |
| `Player` and the concrete enemies (`Spider`, ...) | Decide intended movement and respond to terrain contacts. |
| `Enemy` | Shared abstract base for the concrete enemies; owns the per-frame order they all run in. |
| `Entity` | Provide the shared body and movement interface. |
| `MapLayer` | Continue rendering tile layers. |

The resolver uses rectangle geometry. It does not depend on Tiled, textures, keyboard input, attacks, or a particular actor class.

## Progress and revised approach

The following preparatory fixes have already been implemented by you and reviewed:

- [x] Supply `InputState` before updating the player.
- [x] Remove `m_position` and its assignments; `Entity::getPosition` reads the shape directly.
- [x] Refresh `playerBounds` between collision stages after possible position changes.
- [x] Return from the enemy collision function immediately after respawning, before any further enemy or attack checks.

The attack-hitbox offset left by the old ground correction is resolved. Terrain resolution now runs before the existing positioning code in `Player::attackingLogic`, which reads the corrected body position, and `World::collision_player_ground` has been removed.

Death/respawn is a separate case: it occurs during enemy interactions, after the player's normal update. Step 9 handled it: `Player::respawn()` cancels the attack, resets the player's state and selects the standing pose in the same frame, and the existing early return still prevents further enemy checks in that frame.

## Step 1 — Establish the starting behavior

**Read:** `src/game/World.cpp`, `src/entities/Player.cpp`, `src/entities/Enemy.cpp`, and `src/entities/Entity.cpp`.

- [x] Run the existing game from CLion and try walking, jumping, sprinting, airborne attacks, enemy contact, and note collection.
- [x] Follow the current movement path: `World` updates the entities; the player moves its shape; `World` then checks overlap with the hardcoded floor.
- [x] Identify why the floor check is insufficient: every intersection moves Aeris onto the rectangle's top, regardless of the direction of approach.
- [x] Trace the completed input, position, and bounds fixes listed above. Preserve them while replacing the terrain collision path.
- [x] Confirm that the current TMX has only a visual tile layer. Spiders currently have horizontal patrol movement without gravity.

**Checkpoint:** You can identify where intended movement is calculated, where position changes, and where the existing ground response happens.

## Step 2 — Define the geometry and movement contract

**Create:** `src/terrain/TerrainCollision.hpp` and `src/terrain/TerrainCollision.cpp`, with `TerrainContacts.hpp` and `TerrainMove.hpp` beside them.

- [x] Use SFML rectangles for actor bodies and terrain. Positions are world pixels measured from the top-left; positive X goes right and positive Y goes down. Body size comes from the physical hitbox, not the sprite or attack area.
- [x] Define `TerrainContacts` to describe floor, ceiling, left-wall, and right-wall impacts. Left and right refer to the actor's sides.
- [x] Define `TerrainMove` to contain corrected bounds and contact information.
- [x] Give `TerrainCollision` a collection of solid rectangles and a movement operation. Its inputs are the body's starting bounds and requested displacement in pixels. Its output is the movement result; the caller applies it to the actor.
- [x] Provide read-only access to the solids so `World` can draw their outlines later.

**Checkpoint:** You can explain the difference between velocity, displacement, contact. A movement request does not directly modify an entity.

## Step 3 — Set up small geometry tests

**Edit:** `CMakeLists.txt`. **Create:** `tests/TerrainCollisionTests.cpp`.

- [x] Create a separate collision test executable that builds the resolver sources.
- [x] Put the resolver sources in a reusable library target and link it to both the game and the collision test executable.
- [x] Use the project's existing SFML dependency and compiler settings. No additional physics or testing library is needed.
- [x] Register the test executable with CTest so it can run from CLion or CTest without launching the game.
- [x] Make failed checks report the scenario and return a failing exit status. Checks must remain active in Release builds; do not rely solely on assertions that disappear there.
- [x] Begin with empty terrain: the body should move by the full requested displacement. Zero displacement should preserve its bounds.

**Checkpoint:** The original game still builds, and the first geometry tests run without creating a window or loading textures.

## Step 4 — Detect a swept collision against one rectangle

**Work in:** `TerrainCollision` and its tests.

- [x] Validate that body and terrain dimensions are positive and coordinates are finite. Negative world positions are allowed.
- [x] Reject a body that already penetrates terrain at the start, with a descriptive error. Exact boundary contact is valid. Automatic recovery from invalid placement is outside this version.
- [x] For one obstacle, determine when the moving body enters and leaves its horizontal and vertical spans during the requested movement.
- [x] Combine those intervals to find the first collision during the frame and identify the impacted side. Treat the start as time zero and completion of the requested displacement as time one.
- [x] Handle zero movement on either axis explicitly. Moving away from a touching surface must remain possible.
- [x] Distinguish a collision that would enter the obstacle from a path that only grazes a point and continues outside it.
- [x] Add tests for approaching all four sides and crossing a thin wall or floor in one large movement.

**Checkpoint:** A fast-moving body stops at the correct face even when its requested final position lies completely beyond the obstacle.

## Step 5 — Resolve multiple contacts and ground support

**Work in:** `TerrainCollision` and its tests.

- [x] Check all solids and choose the earliest collision. Scanning the entire collection is sufficient for the test level.
- [x] Move to that contact, block the remaining displacement into the surface, and retain the displacement along it. This allows sliding along walls and floors.
- [x] Sweep the remaining movement again. Bound the loop to four iterations and stop once no movement remains. Never apply unchecked leftover movement if the limit is reached.
- [x] Handle simultaneous impacts consistently, independent of the order of rectangles in the collection.
- [x] Ensure adjoining floor rectangles do not create false wall collisions at their seam. A supporting face must not become a side obstruction merely because another rectangle starts beside it.
- [x] Use a small, documented tolerance for boundary rounding. Do not add a large gap around the actor or accept substantial penetration.
- [x] Determine ground support at the final position: feet meet a solid's top with positive horizontal overlap. A single corner, wall, or ceiling does not provide support.
- [x] Preserve support during idle movement, clear it after leaving a ledge, and never hold an upward jump against the floor.

**5.7 design note — support-query optimization:** Keep a separate ground-support query that checks the final body bounds against `m_solids`, returning as soon as it finds support.

David proposed temporarily storing the solids that reported a floor impact during `resolveMovement`, then checking only those solids for final support. This could reduce the extra terrain scan, but that list can miss supporting terrain: zero downward movement produces no floor impact, and a body can land on floor A before sliding onto floor B without producing a new floor impact against B. Normal standing still usually includes a downward gravity request; the zero-movement case matters when gravity or movement is suspended.

Gravity will often rediscover the supporting floor next frame, but the error is not guaranteed to last only one frame. With the planned player integration, a false `m_onGround` could trigger an airborne attack while the body is actually on a platform. The attack freeze suppresses gravity, preventing a new floor impact until the freeze ends.

Keep David's idea as a possible later optimization: check the stored floor-impact solids first and return `true` if one supports the final bounds. If none does, check the remaining solids before returning `false`.

**Checkpoint:** Tests pass for sliding, seams in both directions, corners, reversed collider order, standing still, jumping away from contact, and landing followed by sliding off an edge.

## Step 6 — Preserve the completed position and input fixes

**Review during integration:** `Entity`, `Player`, `Enemy`, and `World`.

- [x] Keep the physical shape as the single source of position. `Entity::getPosition` already reads it directly, and the duplicate `m_position` variable is gone.
- [x] Keep providing the current `InputState` before entity updates. Preserve the existing input controls.
- [x] Retain fresh body bounds before each interaction phase and the early return after respawn. Adapt the remaining checks when Step 7 removes the old ground-collision call.
- [x] Leave attack-hitbox positioning inside `attackingLogic` for now. Step 7 will place terrain resolution before this existing code, so a separate ground-correction helper is unnecessary.
- [x] When applying resolved positions in Steps 7 and 8, ensure sprites are synchronized before drawing. Death has its own attack and animation reset in Step 9.

**Checkpoint:** Both actors expose their current body positions, input reaches the player before its update, and interaction checks use fresh bounds. The interim ground-hitbox mismatch remains deferred to Step 7.

## Step 7 — Connect Aeris to the resolver

**Edit:** `Entity`, `Player`, `Enemy`, and `World` declarations and definitions together.

- [x] Let `World` own a `TerrainCollision` instance. Initially populate it with the existing floor: position (-500, 550), size 6000 by 50 pixels. *An undrawn 50x50 block at (-400, 500) was added when Step 7 was finished; both are placeholders until the map loader supplies the terrain.*
- [x] Pass a read-only terrain reference through the entity update and movement interfaces. Update both derived classes and the `World` call site together; the spider starts using the resolver in Step 8. *`update()` carries the terrain reference; `movementLogic()` receives only the pre-move ground-support flag, computed by the caller, because that is the one terrain fact it needs.*
- [x] In player movement, calculate input movement, jump/gravity velocity, sprint modifiers, and dash motion before asking the resolver to move the body.
- [x] Include both `m_movement` and `m_velocity` when calculating displacement. Using velocity alone would omit ordinary keyboard movement.
- [x] Replace direct shape movement with applying the resolver's corrected position exactly once.
- [x] Zero persistent velocity components pointing into contacted surfaces. Preserve motion along the surface and motion away from it.
- [x] Set `m_onGround` from final support. Restore dash availability on landing. A ceiling or wall contact must not enable jumping or restore an airborne dash.
- [x] During an airborne attack, retain the existing freeze rules. A wall clears horizontal dash velocity while the attack finishes normally.
- [x] Make the player update order explicit: calculate intended movement, resolve terrain, apply corrected position and contact responses, run `attackingLogic`, then run `animationLogic`. The existing attack-hitbox positioning block now reads the final body position. Terrain correction must not restart or cancel the attack or reset its animation progress.
- [x] Remove the old `collision_player_ground` response and its call. Keep the floor's temporary drawing until the Tiled integration is ready.

**Checkpoint:** Aeris stands, walks, jumps, and lands on the same floor using only the new resolver. During an active attack, the hitbox matches the corrected body position before enemy hit checks. There must never be two terrain correction systems running together.

## Step 8 — Connect spiders to the same resolver

**Edit:** `src/entities/Enemy.hpp` / `.cpp` (the shared base) and `src/entities/enemies/Spider.hpp` / `.cpp` (the concrete spider). `Enemy` was split into a base plus concrete enemies before this step, so the work lands in both. Everything every enemy shares -- gravity, the terrain sweep, applying the corrected position, and the velocity responses to floor and ceiling contacts -- belongs in `Enemy::update`. Deciding intended movement and deciding what a horizontal contact means belong to the concrete enemy.

The fixed patrol interval from the original mock-up is gone. An enemy now walks in its facing direction until terrain stops it, and the contact itself is what reverses it, so there are no limits to clamp against.

- [x] Apply the existing gravity constant to vertical velocity in `Enemy::update`, before intended movement is decided, so the displacement includes this frame's gravity.
- [x] Build each frame's horizontal movement from the enemy's stored facing direction into `m_movement`, resetting it first. `m_movement` is intent and carries no memory between frames; `m_velocity` stays physics-only, so clearing it on contact cannot destroy the direction the enemy is walking.
- [x] Have `movementLogic` return the composed displacement and never write the shape. `Enemy::update` resolves it and applies the corrected position exactly once, so nothing can place a body past the sweep that produced it.
- [x] Resolve both movement axes through the shared terrain system and synchronize the corrected body and sprite positions.
- [x] Stop downward velocity on landing and upward velocity at a ceiling.
- [x] Reverse direction on a horizontal terrain contact and flip the sprite with it, through a virtual hook on `Enemy` that hands the contacts down. The hook has an empty default, so each enemy decides for itself what a wall means.
- [x] Allow unsupported enemies to fall. Do not add a ledge-avoidance probe.

**Checkpoint:** The spider rests on the floor under gravity, walks continuously, reverses direction and flips its sprite when terrain blocks it horizontally, and falls when it leaves a ledge. The resolver contains no enemy-specific rules, and no enemy writes its own position.

## Step 9 — Make spawning and interactions use valid positions

**Edit:** `World` and the player's placement/respawn behavior.

- [x] Validate every initial body and the player respawn rectangle against terrain. Use the same starting-overlap checks as movement; a spawn touching the floor is valid.
- [x] Initialize ground support from the spawn position before the first movement update. *Covered without a code change: `Player::update` queries `hasGroundSupport()` on the current bounds before `movementLogic()`, so her first update already starts from the spawn's real support, and `Enemy` never reads `m_onGround`.*
- [x] Keep player/enemy body contact, attack hits, and note collection as separate gameplay checks after actor movement has finished. *Already true, no code change: `World::update` runs every entity's `update()` first, then `handleCollisions()`, which runs all three as plain overlap checks outside the resolver.*
- [x] Read current bounds for each interaction phase. After a respawn, discard bounds from the previous position. *Already true, no code change: `handleCollisions()` reads the player's bounds before enemy contact and again before note collection, attack hits read `getAttackingBounds()` directly, and the early return after respawn stops any enemy check from using the old position.*
- [x] Centralize attack cancellation: clear `m_isAttacking`, stop the active attack timer, release airborne freeze and dash state, and reset attack-animation elapsed time and frame. Reset the attack sprite's texture rectangle too, so the next attack starts from its first frame. Keep the existing cooldown behavior for ordinary cancellation. *Implemented as `Player::cancelAttack()`. The animation reset lives in `attack()` instead, which restarts the animation and its texture rectangle whenever an attack starts, so `cancelAttack()` does not repeat it.*
- [x] Give death/respawn one consistent reset path that cancels the attack, clears velocity and transient movement, restores dash availability, resets the cooldown for the new life, and synchronizes placement and ground support. The attack rectangle may remain allocated; a false attacking flag must disable both its drawing and its hit checks. *Implemented as `Player::respawn()`. Ground support comes from the next update's pre-move check, as in the spawn item above.*
- [x] Immediately select the standing animation at the respawn position and apply its first frame. Death occurs after `animationLogic` has already run, so clearing only the attacking flag can leave `m_animationToDraw` selecting the attack sprite for the rest of that frame. Do not wait for the next player update to correct the visible pose.
- [x] Keep the early return from enemy collision handling after respawn. Any later checks, such as note collection, must use the respawned state and fresh body bounds. *Already true, no code change: `collision_player_enemies()` returns right after the respawn and note collection re-reads the bounds. The respawn reset path from the item above must keep that return.*
- [x] Use consistent cancellation cleanup for existing jump and facing-change cancellations as well as normal attack completion. Ensure gravity is released, and preserve movement deliberately initiated by the transition, such as the new jump velocity.

**Checkpoint:** Dying during a grounded attack, an airborne freeze, or a dash immediately disables the attack hitbox and shows the standing pose at respawn. Old motion and attack state do not carry over; camera and pickup checks use the new position immediately. Ground correction continues to preserve active attacks as specified in Step 7.

**Status:** done. Every item is implemented or confirmed, and the checkpoint passed its play test on 24 September 2026.

## How we proceed together

Every step in this guide is complete. Each one was discussed first, then implemented and reviewed, and the next step started only after its checkpoint passed. Step 6 was a review of the completed fixes during integration; it did not require an interim attack-positioning helper.
