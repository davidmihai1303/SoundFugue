# SoundFugue — Agent Instructions

Read this before doing anything in this repo. It has two jobs: set the rules for how an agent works here, and give a thorough, accurate map of the project so you don't have to rediscover it from scratch every session.

---

## 1. Rules — David is in control, not the agent

This is David's project, built partly as his own learning exercise in C++ and game architecture. The agent's job is to implement exactly what's asked, explain reasoning clearly, and otherwise stay out of the driver's seat.

1. **David decides scope and sequencing, not the agent.** Never jump ahead in `MAP_LOADER_IMPLEMENTATION_GUIDE.md` (or any other roadmap) just because the next step is obvious. This project has a documented history of exactly this mistake — see `COLLISION_ENGINE_DEVELOPMENT_DRAFT.md`, mistake 22.15 ("Implementing later checklist work before it was requested"). If it wasn't explicitly requested, don't build it, even as a "small bonus while I'm in there."
2. **Work in small, reviewable increments.** One requested change, one diff, reviewed before moving on. Don't bundle unrelated concerns into a single edit. If a "small" fix turns out to require touching several files or restructuring an interface, say so and confirm scope before proceeding rather than silently expanding the change.
3. **Don't add things that weren't asked for**: no extra tests, no defensive code for cases that can't happen, no refactors "while I'm here," no speculative support for explicitly deferred scope (slopes, one-way platforms, moving platforms, actor pushing, rotated shapes — see the collision draft's "Scope chosen" section). Deferred means deferred until asked for.
4. **Ask when a decision is genuinely David's to make**, rather than guessing — e.g. which class owns a piece of logic, what a public interface should look like, how to interpret an ambiguous instruction. Guessing wrong on an architectural call is more expensive to undo than asking.
5. **Never commit or push without being explicitly asked.** When asked to commit, match the project's existing granularity: one commit per checklist sub-step (see `git log` — `collision engine work 5.4`, `5.5`, etc.), not a squashed batch of several sub-steps.
6. **Treat the root-level docs as living and fallible, not gospel.** They describe a moving target and can go stale (this has already happened once and been corrected). Before relying on a claim in one of them — "step X is done," "function Y exists" — check it against the actual source or `git log`.
7. **Keep the docs' roles separate** (see Section 4). Don't let historical/how-it-works content drift into the forward checklist, or vice versa.
8. **Explain before implementing.** Treat each step as a discussion: identify the exact variables and control flow involved, explain the plan, then write a small change — not a silent "smart" fix.

---

## 2. What this project is

**SoundFugue** is a 2D platformer built in C++ with SFML 3, aiming for tight, fast, fluid movement in the spirit of *Rayman Origins*. The world is built from music: Lord Tacet has silenced it, and the player character, Aeris, travels through silenced "Genre Worlds" (Rock, Disco, etc.) restoring their sound by defeating bosses and reclaiming each world's "Essence." Full pitch and feature list: `README.md`.

Current stated status (per README): Aeris' core physics/movement are considered finished; active focus has shifted to building out the world. The terrain-collision engine is complete; the Tiled map loader is the next area of work.

---

## 3. Architecture map

```
src/main.cpp            Entry point — constructs Game and runs it.

src/game/
  Game.cpp/hpp           Owns the window and the frame loop: poll events -> World::update
                          -> update camera -> World::draw. Also builds InputState from raw
                          SFML events each frame.
  World.cpp/hpp           Owns all entities, the TerrainCollision instance, the drawn ground
                          rectangle, the Tiled map layers, and the notes. Passes the terrain
                          into every entity update, then coordinates gameplay-level
                          interactions (enemy contact, note pickup). m_terrain is still built
                          by hand in the constructor and holds TWO solids: the floor at
                          (-500, 550) sized 6000x50, which m_ground draws, and a 50x50 block
                          at (-400, 500) that nothing draws. Tiled supplies terrain at Step 11 of the map loader guide.
                          At the end of its constructor it checks every entity's spawn and
                          the respawn point (Constants::Player::RespawnPosition) with
                          TerrainCollision::validatePlacement(). On enemy contact it calls
                          Player::respawn() and returns early.
  InputState.hpp          Plain struct describing one frame's raw input (click/shift state,
                          click ordering for dash detection).
  Constants.hpp            All tunable numeric constants (movement, combat, animation,
                          physics tolerances used by the terrain resolver).

src/entities/
  Entity.cpp/hpp          Abstract base. Physical position lives ONLY in m_shape
                          (sf::RectangleShape) — there is no separate cached position
                          variable; getPosition() reads m_shape directly. Declares the
                          update/draw/movementLogic/attackingLogic contract every actor
                          implements; update() and movementLogic() now carry the terrain
                          reference and the pre-move ground-support flag. Provides
                          resolveTerrainMovement(), the single place where a resolved
                          position is applied to a shape.
  Player.cpp/hpp          Aeris: movement, gravity, jump, dash, sprint, attack state
                          machine, animation selection. Terrain is resolved inside update(),
                          in this order: movementLogic() returns the requested displacement
                          -> Entity::resolveTerrainMovement() sweeps it and applies the
                          corrected position -> m_onGround is set from a fresh
                          hasGroundSupport() on the final bounds -> velocity components
                          pointing into a contacted surface are cleared -> attackingLogic()
                          -> animationLogic(). Ground support is deliberately queried on both
                          sides of the movement; the comments in update() say why. Every
                          way an attack ends (finishing, jump, facing change, death) goes
                          through the private cancelAttack(); death goes through the public
                          respawn(position), the single reset path.
  Enemy.cpp/hpp           Abstract shared base for every regular enemy. Owns update(), the whole
                          shared frame: apply gravity -> movementLogic() for intent -> sweep via
                          Entity::resolveTerrainMovement() -> set m_onGround from a fresh
                          hasGroundSupport() -> clear velocity components pointing into contacted
                          surfaces -> animationLogic() -> contactLogic(). Declares two protected
                          virtuals: animationLogic() (pure) and contactLogic() (empty default, so
                          each enemy decides what a contact means). movementLogic() and draw()
                          stay pure virtual from Entity, so Enemy can't be instantiated. Also
                          holds attackingLogic() (a //TODO no-op) and setColor(). Its constructor
                          is protected and takes (position, size).
  enemies/Spider.*        The only concrete enemy today (Spider final : public Enemy). Owns the
                          walking sprite and its animation, movementLogic(), contactLogic() and
                          draw(). movementLogic() rebuilds m_movement each frame from
                          m_currentFacingDirection and returns a displacement; it never writes
                          the shape. contactLogic() reverses the facing and mirrors the sprite on
                          a left- or right-wall contact — there are no patrol limits, terrain is
                          what turns it. The remaining 4-7 enemies join it in
                          src/entities/enemies/.
  Note.cpp/hpp            Collectible pickups.

src/terrain/               A fully-tested swept-AABB collision resolver with no dependency on
  TerrainCollision.*        Tiled, textures, input or any actor class. Both Player and Enemy now
  TerrainContacts.hpp       drive it through Entity::resolveTerrainMovement(). Its rectangles are
  TerrainMove.hpp           still supplied by hand in World's constructor — TerrainMapLoader does
                            not exist yet (map loader guide, Step 10). validatePlacement() is the public
                            starting-overlap check that resolveMovement() runs first and World
                            uses for spawns. See Section 5.

src/graphics/
  TextureHolder.*          RAII texture-loading helper.
  MapLayer.hpp             Renders one Tiled tile layer (visual only, no collision data).

tests/TerrainCollisionTests.cpp
                            Headless console executable (no window) exercising the terrain
                            resolver via coordinate-based scenarios. Registered with CTest.

ext/tmxlite                 Vendored source of the tmxlite Tiled-map-parsing library (built
                            from source by CMake, not fetched as a package).

resources/                  Sprites (PNG spritesheets) and the current Tiled map
                            (resources/maps/untitled.tmx — visual tile layers only, no
                            Collision object layer yet).

scripts/                    Dev tooling: cppcheck, sanitizers, valgrind wrappers. Not part
                            of the normal build.

CMakeLists.txt               Fetches SFML 3.1.0 via FetchContent; builds tmxlite from ext/
                            as a static lib; builds the terrain resolver as its own static
                            lib (TerrainCollisionLib) linked by both the game executable and
                            TerrainCollisionTests; builds the SoundFugue executable; registers
                            TerrainCollisionTests with CTest.

.clang-format                Attached braces, 4-space indent, 200 columns (raised from 120 in
                            4fdeef1), LLVM-based. CLion honours it only with ClangFormat
                            enabled (Settings -> Editor -> Code Style -> C/C++). src/ was first
                            reformatted in 831db37; src/ and tests/ were reformatted at 200
                            columns on 24 September 2026.
```

### Build / test

The project is normally driven from CLion, which already has `cmake-build-debug` configured (Ninja generator). From the command line:

```sh
cmake --build cmake-build-debug --target SoundFugue           # build the game
cmake --build cmake-build-debug --target TerrainCollisionTests # build the resolver tests
./cmake-build-debug/TerrainCollisionTests                      # run them directly
ctest --test-dir cmake-build-debug                              # or via CTest
```

The game loads resources via relative paths (`"../resources/..."`), so run the `SoundFugue` binary with `cmake-build-debug` as the working directory, matching how CLion launches it.

---

## 4. The four root-level docs — what each one is for

| File | Role | Tense / content rule |
| --- | --- | --- |
| `COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` | Narrative history of the terrain resolver and its integration: how it works, why each piece exists, mistakes made and fixed. | Past/present tense only — describes exclusively what is already implemented and tested. Deliberately excludes future/planned work; update it once a step is actually finished, never before. |
| `COLLISION_IMPLEMENTATION_GUIDE.md` | The checklist that built the resolver and connected it to the game, Steps 1–9. Done. | A finished record; don't reopen it. |
| `MAP_LOADER_IMPLEMENTATION_GUIDE.md` | The authoritative forward checklist for loading terrain from Tiled and verifying the whole result (Steps 10–12; numbering continues from the collision guide). | This is "what's next" — follow it one step at a time, in order, and don't tick boxes that haven't actually been done. |
| `MOUSE_INPUT_DIAGNOSTICS.md` | An open, unresolved investigation into intermittent mouse-click/attack failures. | Diagnostic protocol only — no fix has been applied yet and no root cause is confirmed. Follow its procedure to gather evidence rather than guessing at a fix. |

`COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` is untracked: it's listed in `.git/info/exclude`, so it never appears as an untracked file in `git status` either. Both guides are tracked in git, so their checkbox state is part of the history and shows up in diffs. Don't commit anything without being asked (Rule 5).

---

## 5. Current implementation status (verify before trusting — see Rule 6)

As of 24 September 2026 (collision engine finished in `98d1433`, guides split afterwards), with the docs brought up to date:

- **The collision engine is done: every step of `COLLISION_IMPLEMENTATION_GUIDE.md` is complete.** Work continues in `MAP_LOADER_IMPLEMENTATION_GUIDE.md`, whose steps are numbered from 10; Step 10 is next. Step 9: 9.1 (`9444829`, `validatePlacement()` plus the startup spawn checks), 9.5 and 9.9 (`37b6907`, `cancelAttack()` used at every attack end), 9.6 (`4fdeef1`, `Player::respawn()`) and 9.7 (`98d1433`, the standing pose set inside `respawn()`). 9.2, 9.3, 9.4 and 9.8 needed no code; each is ticked with a note in the guide saying why. Step 9's checkpoint passed its play test on 24 September 2026. Steps 10 and 11 are untouched.
- **The map loader's place is decided.** It lives in `src/terrain/TerrainMapLoader.*` and gets its own static library, `TerrainMapLoaderLib` (linking tmxlite and SFML), used by the game and `TerrainMapLoaderTests`, so `TerrainCollisionLib` stays Tiled-free. Step 12's collision scenarios are verified after the loader, because they need the Tiled terrain.
- **Terrain resolver (`src/terrain/`)**: `TerrainCollision::resolveMovement()` does swept AABB collision with repeated sweeping for sliding (capped at 4 iterations), merges simultaneous multi-solid impacts independent of storage order, separates face vs. corner contacts to avoid false hits at terrain seams, uses a documented floating-point tolerance, and `hasGroundSupport()` answers final ground support as an independent query. `tests/TerrainCollisionTests.cpp` covers it in 13 headless scenario groups.
- **Both actors run on the resolver.** `World` owns the `TerrainCollision` and passes it into every `Entity::update`. Aeris was wired in at Step 7, enemies at Step 8, both through `Entity::resolveTerrainMovement()` — the single place a resolved position is applied to a shape. No actor writes its own body position outside its constructor and `Player::respawn()`. There is exactly one terrain correction path; if a second one ever appears, that's the bug.
- **`Enemy` is now a shared base with `Spider` as its one concrete enemy** (`a92bdaa`), a restructure made outside the guide's numbered steps so that Step 8's sequencing is written once in `Enemy::update` rather than repeated in each of the 5-8 planned enemies.
- **Step 8's design changed mid-step.** The original mock-up patrol interval is gone. An enemy walks in its facing direction until terrain stops it, and the horizontal contact itself is what reverses it, through `Enemy`'s `contactLogic()` hook. The guide's Step 8 was rewritten to match, so its old boxes about clamping to a patrol interval no longer exist.
- **Step 8 was play-tested on 22 September 2026 and passed its checkpoint**: resting on the floor without jitter, the turn reading correctly, falling when it leaves a ledge. In the current world the terrain that turns the spider is the undrawn 50x50 block. Step 12's deliberate spider tests (now in the map loader guide) — a wall placed in its path and a platform with an open edge — remain part of final verification.
- **Attack-input changes (`79679e2`, 23 September 2026), made outside the guide's numbered steps.** `Constants::Player::AttackCooldown` is 0.36 s (was 0.5). `Player::attack()` accepts a click whenever the cooldown is ready and no dash is in progress (`!m_dashAttack`), and every accepted click fully restarts the attack: its timer, its animation time and frame, and the attack sprite's texture rectangle. The jump in `Player::movementLogic` also requires `!m_isFrozen`, so a jump can no longer cancel a frozen attack. Committed in `79679e2` together with the tracked doc edits of 22-23 September, and play-tested on 23 September 2026: everything behaved as intended.
- **Formatting**: `.clang-format`'s column limit is 200 (raised from 120 in `4fdeef1`). All 24 tracked files under `src/` and `tests/TerrainCollisionTests.cpp` were reformatted to it on 24 September 2026, a whitespace-only change, and all pass `clang-format --dry-run --Werror`.
- **Steps 10-12 of `MAP_LOADER_IMPLEMENTATION_GUIDE.md`** (`TerrainMapLoader`, the Tiled `Collision` object layer replacing the hardcoded floor, final verification) — **not started.**
- **`COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` covers Steps 1-9.** Sections 1-15 are the starting point and the resolver (Steps 1-5), 16-19 the integration (Steps 6-9), and 20-26 the reference sections (algorithm, invariants, mistakes, testing, glossary, source map, working method). Sections 16-19 were written from the code and git history and have no "What was challenging" subsections; those are David's to write.
- **Known open bugs**: none. `KNOWN_ISSUES.md` was deleted on 24 September 2026 once every entry was closed: #1-#4 by Step 9, #5 by Step 8, #7 by refusing the jump while frozen, and #6 (a dash attack needs the direction key held without a break) confirmed as intended. Separately, intermittent mouse-click/attack failures remain unexplained; `MOUSE_INPUT_DIAGNOSTICS.md` has the diagnostic protocol, and no fix has been attempted.

Confirm the above against `git log --oneline` and the checkbox state in the two guides before acting on it — this section will drift out of date as work continues.
