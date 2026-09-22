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
  World.cpp/hpp           Owns all entities, the TerrainCollision instance, the drawn ground
                          rectangle, the Tiled map layers, and the notes. Passes the terrain
                          into every entity update, then coordinates gameplay-level
                          interactions (enemy contact, note pickup). m_terrain is still built
                          by hand in the constructor and holds TWO solids: the floor at
                          (-500, 550) sized 6000x50, which m_ground draws, and a 50x50 block
                          at (-400, 500) that nothing draws. Tiled supplies terrain at Step 11.
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
                          sides of the movement; the comments in update() say why.
  Enemy.cpp/hpp           Abstract shared base for every regular enemy. Owns update(), the frame
                          order all enemies run: movementLogic() -> animationLogic() -> record
                          facing. Adds a protected pure-virtual animationLogic(); movementLogic()
                          and draw() stay pure virtual from Entity, so Enemy can't be
                          instantiated. Also holds attackingLogic() (a //TODO no-op) and
                          setColor(). Its constructor is protected and takes (position, size).
                          update() receives the terrain and ignores it for now — Step 8 puts the
                          sweep and the shared contact responses here, once, for every enemy.
  enemies/Spider.*        The only concrete enemy today (Spider final : public Enemy). Owns the
                          patrol limits, the walking sprite and its animation, movementLogic()
                          and draw(). Still moves m_shape directly and clamps after moving, and
                          has no gravity — Step 8 replaces that; KNOWN_ISSUES.md #5 has the
                          specifics. The remaining 4-7 enemies join it in src/entities/enemies/.
  Note.cpp/hpp            Collectible pickups.

src/terrain/               A fully-tested swept-AABB collision resolver with no dependency on
  TerrainCollision.*        Tiled, textures, input or any actor class. It now drives Player
  TerrainContacts.hpp       through Entity::resolveTerrainMovement(); Enemy still bypasses it
  TerrainMove.hpp           (Step 8). Its rectangles are still supplied by hand in World's
                            constructor — TerrainMapLoader does not exist yet (Step 10).
                            See Section 5.

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

## 4. The four root-level docs — what each one is for

| File | Role | Tense / content rule |
| --- | --- | --- |
| `COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` | Narrative history of the terrain resolver: how it works, why each piece exists, mistakes made and fixed. | Past/present tense only — describes exclusively what is already implemented and tested. Deliberately excludes future/planned work; update it once a step is actually finished, never before. |
| `COLLISION_IMPLEMENTATION_GUIDE.md` | The authoritative forward checklist for connecting the resolver to the game (Steps 1–12, checkbox per sub-item). | This is "what's next" — follow it one step at a time, in order, and don't tick boxes that haven't actually been done. |
| `KNOWN_ISSUES.md` | Bugs found while auditing Step 7 and deliberately left unfixed. Each entry records the mechanism, how to trigger it, what the player sees, and which guide step owns the fix. Also records two things that were checked and found sound, so they aren't re-investigated. | A register of accepted debt, not a task list. Don't fix an entry as a side errand — it belongs to its owning step, and fixing issues 1–4 piecemeal before Step 9 risks writing the same cleanup twice. Add to it when an audit finds something out of scope. |
| `MOUSE_INPUT_DIAGNOSTICS.md` | An open, unresolved investigation into intermittent mouse-click/attack failures. | Diagnostic protocol only — no fix has been applied yet and no root cause is confirmed. Follow its procedure to gather evidence rather than guessing at a fix. |

`COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` and `KNOWN_ISSUES.md` are untracked: they're listed in `.git/info/exclude`, so they never appear as untracked files in `git status` either. `COLLISION_IMPLEMENTATION_GUIDE.md` is committed (`cac7405`), so its checkbox state is part of the history and shows up in diffs. Don't commit anything without being asked (Rule 5).

---

## 5. Current implementation status (verify before trusting — see Rule 6)

As of `3d27c0d collision engine work 7 done` (21 September 2026):

- **Steps 1–7 of `COLLISION_IMPLEMENTATION_GUIDE.md` are complete.** Every box through Step 7 is ticked, and the commit history matches it at one commit per sub-step.
- **Terrain resolver (`src/terrain/`)**: `TerrainCollision::resolveMovement()` does swept AABB collision with repeated sweeping for sliding (capped at 4 iterations), merges simultaneous multi-solid impacts independent of storage order, separates face vs. corner contacts to avoid false hits at terrain seams, uses a documented floating-point tolerance, and `hasGroundSupport()` answers final ground support as an independent query. `tests/TerrainCollisionTests.cpp` covers it in 13 headless scenario groups.
- **Aeris runs on the resolver (Step 7).** `World` owns the `TerrainCollision` and passes it into every `Entity::update`. The old `World::collision_player_ground` response and its call are gone, and so are `Player::groundCollisionLogic()` and `Player::setGroundBounds()` — the interim arrangement Step 6 created. There is now exactly one terrain correction path; if a second one ever appears, that's the bug.
- **`Enemy` has been split into a shared base plus concrete enemies** (22 September 2026), ahead of Step 8 and outside the guide's numbered steps. `Enemy` is now abstract and owns the frame order; `Spider` is the one concrete enemy, under `src/entities/enemies/`. Behaviour is unchanged — it was a pure restructure, so that Step 8's terrain sequencing is written once in `Enemy::update` instead of being repeated in each of the 5–8 planned enemies. Verified by syntax-checking every affected translation unit; no new compiler warnings.
- **Step 8 (spiders on the same resolver) is next, and none of it has been started.** `KNOWN_ISSUES.md` #5 is effectively its to-do list: `Enemy::update` takes `terrain` and ignores it and passes the literal `1` where real ground support belongs, `Spider::movementLogic` still moves the shape before clamping rather than after, and there is no gravity. The work now lands in two files — see the guide's Step 8 scope line.
- **One decision is owed before Step 8.** `KNOWN_ISSUES.md` #4 suggests pulling the spawn-validation item forward from Step 9. The spider spawns at y 500–550 against a floor whose top is y 550 — legal exact-boundary contact, but with zero margin. Nothing checks it today because the spider bypasses the resolver; Step 8 makes it checked every frame, at which point a one-pixel change to the spider or the floor becomes an uncaught `std::invalid_argument` rather than a visual glitch. Whether to reorder is David's call (Rule 1), not the agent's.
- **Steps 9–12** (the single death/respawn reset path, `TerrainMapLoader`, the Tiled `Collision` object layer replacing the hardcoded floor, final verification) — **not started.**
- **`COLLISION_ENGINE_DEVELOPMENT_DRAFT.md` has not been brought up to date for Steps 6 and 7.** Its header still says "Steps 1 through 5," and its Section 21 source map still claims `Player.cpp` "does not yet call TerrainCollision" and `World.cpp` "does not yet own a TerrainCollision instance." Treat those as stale rather than current, and see Rule 6 before relying on anything else in it.
- **Known open bugs**: `KNOWN_ISSUES.md` holds the six recorded during the Step 7 audit — four in the respawn path (owned by Step 9), one covering the enemy placeholders (Step 8), and one game-feel question about dash-attack input that a play test should settle rather than a code change. Separately, intermittent mouse-click/attack failures remain unexplained; `MOUSE_INPUT_DIAGNOSTICS.md` has the diagnostic protocol, and no fix has been attempted.

Confirm the above against `git log --oneline` and the checkbox state in `COLLISION_IMPLEMENTATION_GUIDE.md` before acting on it — this section will drift out of date as work continues.
