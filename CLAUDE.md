# SoundFugue — Agent Instructions

Read this before doing anything in this repo. It has two jobs: set the rules for how an agent works here, and give a thorough, accurate map of the project so you don't have to rediscover it from scratch every session.

---

## 1. Rules — David is in control, not the agent

This is David's project, built partly as his own learning exercise in C++ and game architecture. The agent's job is to implement exactly what's asked, explain reasoning clearly, and otherwise stay out of the driver's seat.

1. **David decides scope and sequencing, not the agent.** Never jump ahead in `COLLISION_IMPLEMENTATION_GUIDE.md` (or any other roadmap) just because the next step is obvious. This project has a documented history of exactly this mistake — see `COLLISION_ENGINE_DEVELOPMENT_DRAFT.md`, mistake 18.15 ("Implementing later checklist work before it was requested"). If it wasn't explicitly requested, don't build it, even as a "small bonus while I'm in there."
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

Current stated status (per README): Aeris' core physics/movement are considered finished; active focus has shifted to building out the world. In practice, the terrain-collision rewrite (below) is the most recently active area of work.

---

## 3. Architecture map

```
src/main.cpp            Entry point — constructs Game and runs it.

src/game/
  Game.cpp/hpp           Owns the window and the frame loop: poll events -> World::update
                          -> update camera -> World::draw. Also builds InputState from raw
                          SFML events each frame.
  World.cpp/hpp           Owns all entities, the (currently hardcoded) ground rectangle, the
                          Tiled map layers, and the notes. Coordinates gameplay-level
                          interactions (enemy contact, note pickup) after entities update.
  InputState.hpp          Plain struct describing one frame's raw input (click/shift state,
                          click ordering for dash detection).
  Constants.hpp            All tunable numeric constants (movement, combat, animation,
                          physics tolerances used by the terrain resolver).

src/entities/
  Entity.cpp/hpp          Abstract base. Physical position lives ONLY in m_shape
                          (sf::RectangleShape) — there is no separate cached position
                          variable; getPosition() reads m_shape directly. Declares the
                          update/draw/movementLogic/attackingLogic contract every actor
                          implements.
  Player.cpp/hpp          Aeris: movement, gravity, jump, dash, sprint, attack state
                          machine, animation selection. Ground correction currently lives
                          here (groundCollisionLogic(), run between movementLogic and
                          attackingLogic) using the hardcoded floor World hands it each
                          frame via setGroundBounds() — see Section 5.
  Enemy.cpp/hpp           Spiders: horizontal patrol between two X limits. No gravity, no
                          terrain collision yet.
  Note.cpp/hpp            Collectible pickups.

src/terrain/               A standalone, fully-tested swept-AABB collision resolver. NOT
  TerrainCollision.*        currently connected to Player, Enemy, or World — it only exists
  TerrainContacts.hpp       and is exercised inside tests/TerrainCollisionTests.cpp today.
  TerrainMove.hpp           See Section 5 for exactly what it can do and what's missing
                            before it's wired in.

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

## 4. The three root-level docs — what each one is for

| File | Role | Tense / content rule |
| --- | --- | --- |
| `COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` | Narrative history of the terrain resolver: how it works, why each piece exists, mistakes made and fixed. | Past/present tense only — describes exclusively what is already implemented and tested. Deliberately excludes future/planned work; update it once a step is actually finished, never before. |
| `COLLISION_IMPLEMENTATION_GUIDE.md` | The authoritative forward checklist for connecting the resolver to the game (Steps 1–12, checkbox per sub-item). | This is "what's next" — follow it one step at a time, in order, and don't tick boxes that haven't actually been done. |
| `MOUSE_INPUT_DIAGNOSTICS.md` | An open, unresolved investigation into intermittent mouse-click/attack failures. | Diagnostic protocol only — no fix has been applied yet and no root cause is confirmed. Follow its procedure to gather evidence rather than guessing at a fix. |

Both `COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` and `COLLISION_IMPLEMENTATION_GUIDE.md` are currently untracked in git — they're David's working drafts, not yet committed. Don't commit them, or anything else, without being asked (Rule 5).

---

## 5. Current implementation status (verify before trusting — see Rule 6)

As of the last work in this repo:

- **Terrain resolver (`src/terrain/`)**: Steps 1–5 of the implementation guide are complete. `TerrainCollision::resolveMovement()` does swept AABB collision with repeated sweeping for sliding (capped at 4 iterations), merges simultaneous multi-solid impacts independent of storage order, separates face vs. corner contacts to avoid false hits at terrain seams, uses a documented floating-point tolerance, and `hasGroundSupport()` answers final ground support as an independent query. All of this is covered by `tests/TerrainCollisionTests.cpp`. **It is not called from anywhere outside `src/terrain/` yet.**
- **Step 6 (review checkpoint)**: done. As part of it, the *old* hardcoded-floor ground correction was moved from `World::handleCollisions()` into `Player` itself — `Player::groundCollisionLogic()` now runs between `movementLogic()` and `attackingLogic()` inside `Player::update()`, using bounds `World` hands it each frame via `Player::setGroundBounds()` (mirroring the existing `setInputState()` pattern). This still uses the hardcoded floor rectangle, **not** `TerrainCollision`. That swap is Step 7.
- **Step 7 onward** (actually wiring `TerrainCollision` into `Player`/`Enemy`/`World`, loading terrain from Tiled via a `TerrainMapLoader`) — **not started.**
- **Known open bug**: intermittent mouse-click/attack failures, cause unconfirmed. See `MOUSE_INPUT_DIAGNOSTICS.md` for the diagnostic protocol; no fix has been attempted yet.

Confirm the above against `git log --oneline` and the checkbox state in `COLLISION_IMPLEMENTATION_GUIDE.md` before acting on it — this section will drift out of date as work continues.
