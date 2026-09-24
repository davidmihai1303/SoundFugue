# Map loader implementation guide

You write the game code. I explain each step, help you reason through it, and review what you write. Work through this guide one step at a time; finish the checkpoint before moving on.

This guide continues `COLLISION_IMPLEMENTATION_GUIDE.md`, which is done: Steps 1–9 built the terrain collision resolver and connected it to Aeris, the spider, spawns and death. Step numbers continue from it. What is left is making the terrain come from the Tiled map instead of rectangles built by hand in `World`, then verifying the whole result.

## What we are building

- Terrain comes from a Tiled object layer named `Collision`.
- Every supported rectangle in that layer is solid on all four sides.
- The resolver (`TerrainCollision`) stays as it is and knows nothing about Tiled; the loader only converts map objects into rectangles.
- Actor placements stay in code for now (Step 11). Map-based entity spawning, one-way platforms, slopes, rotated shapes, moving platforms and physical pushing between actors come later.

## Responsibilities

| Component | Responsibility |
| --- | --- |
| `TerrainMapLoader` — new | Convert supported TMX collision objects into terrain rectangles. |
| `TerrainCollision` | Unchanged: store the rectangles and resolve movement. |
| `World` | Load the map, build its `TerrainCollision` from the loader's rectangles, and report loading errors. |
| `MapLayer` | Continue rendering tile layers. |

## Decisions already made

- The loader lives in `src/terrain/` (`TerrainMapLoader.hpp` / `.cpp`).
- It gets its own static library, `TerrainMapLoaderLib`, which compiles `TerrainMapLoader.cpp` and links `tmxlite` and SFML. The game and `TerrainMapLoaderTests` link it. `TerrainCollisionLib` and its tests stay as they are, free of Tiled.
- Step 12's collision scenarios are verified here, after the loader, because they need the Tiled terrain.

## Step 10 — Read terrain objects through tmxlite

**Create:** `src/terrain/TerrainMapLoader.hpp`, `src/terrain/TerrainMapLoader.cpp`, and `tests/TerrainMapLoaderTests.cpp`. **Update:** `CMakeLists.txt`: add `TerrainMapLoaderLib` and the `TerrainMapLoaderTests` executable (see *Decisions already made*).

- [ ] Add the `TerrainMapLoaderLib` static library to `CMakeLists.txt`, next to `TerrainCollisionLib`: it compiles `src/terrain/TerrainMapLoader.cpp` and its header, exposes `src/` as a public include directory, links `tmxlite` and `SFML::Graphics`, and gets the project's compiler flags through `set_compiler_flags`. Leave `TerrainCollisionLib` unchanged, so the resolver and its tests stay free of Tiled.
- [ ] Give the loader a parsed `tmx::Map` as input and a collection of SFML terrain rectangles as output. Keep parsing out of the collision resolver.
- [ ] Find exactly one top-level object layer named `Collision`. Report a missing layer, duplicate layer, wrong layer type, or nested `Collision` layer clearly.
- [ ] Accept only ordinary unrotated rectangles with positive dimensions and finite geometry. Reject points, polygons, ellipses, text, and tile objects even if they expose rectangular bounds.
- [ ] Convert object bounds to world pixels and apply the layer offset once. The bundled tmxlite exposes offsets as integer pixels, so use whole-pixel layer offsets for this version.
- [ ] Treat all supported rectangles in this layer as solid. Object names and classes are optional labels; hiding a layer or object in Tiled must not disable its collision.
- [ ] Include the layer and object identifier in errors so an unsupported object can be found in Tiled.
- [ ] Add a separate CTest executable, `TerrainMapLoaderTests`, that links `TerrainMapLoaderLib` and is registered with `add_test` like `TerrainCollisionTests`. Cover valid placement, offsets, hidden objects, invalid shapes, and invalid/missing layers. Use small maps loaded from strings; no rendering is needed.

**Checkpoint:** Loader tests pass independently of the game. The running game still uses the temporary floor until the map is prepared in Step 11.

## Step 11 — Replace the floor source and build a small collision course

**Edit in Tiled:** `resources/maps/untitled.tmx`. **Edit in C++:** `World` and startup error reporting in `src/main.cpp`. **Update:** `CMakeLists.txt`: link `TerrainMapLoaderLib` into the `SoundFugue` executable.

- [ ] Add a top-level object layer named `Collision` with zero offset.
- [ ] Start by adding only the floor rectangle from the table below. Keep the existing tile artwork.
- [ ] After loading the map successfully, have `World` obtain rectangles from `TerrainMapLoader` and construct its terrain system from them.
- [ ] Link `TerrainMapLoaderLib` into the `SoundFugue` executable, since `World` now calls the loader.
- [ ] Report map-loading or collision-data errors and stop startup clearly. Do not silently fall back to the old hardcoded floor.
- [ ] Remove the hardcoded floor collider and drawing. Draw temporary outlines directly from the loaded terrain rectangles, using their actual world bounds.
- [ ] Verify the floor behaves exactly as before, then add the other test objects one at a time.

| Object label | X | Y | Width | Height | Purpose |
| --- | ---: | ---: | ---: | ---: | --- |
| Floor | -500 | 550 | 6000 | 50 | Preserve the current ground. |
| Platform | 560 | 500 | 128 | 16 | A 50-pixel rise for landing and ledge checks. |
| Ceiling | 250 | 390 | 128 | 16 | Stop an upward jump without grounding Aeris. |
| Wall | 800 | 450 | 32 | 100 | Stop walking, sprinting, and dash movement. |

These are temporary testing objects. They do not create visible tile artwork automatically; the outlines make their locations clear while testing.

- [ ] Keep the current actor placements in code and verify their full bodies remain clear of terrain: Aeris starts at (0, 250), respawns at (100, 100), and the spider starts at (400, 500).
- [ ] Add a loader test for the actual project map so future map edits cannot silently remove the collision layer or invalidate these placements. Pass the map's path to `TerrainMapLoaderTests` from CMake, for example as a compile definition built from `CMAKE_SOURCE_DIR`, instead of relying on the working directory, which differs between CLion and CTest.

**Checkpoint:** Moving a rectangle in Tiled changes collision after restarting the game. Tile rendering remains in `MapLayer`; terrain behavior comes entirely from the object layer.

## Step 12 — Verify the complete result

Run the geometry and loader tests in both Debug and Release. Then run the game from CLion and check the following:

| Scenario | Required result |
| --- | --- |
| Stand still or walk across adjoining floor rectangles | No sinking, jitter, or snagging. |
| Terrain correction during an attack, facing either direction | The attack hitbox follows the corrected body position; attack timing and animation progress continue normally. |
| Jump while touching the floor | Upward movement begins freely. |
| Hit the underside of a platform | Upward velocity stops; jumping and dash are not restored. |
| Walk or sprint into either wall face | The body stops outside the wall. |
| Dash into a wall | Horizontal movement stops; the attack finishes and gravity subsequently resumes. |
| Walk off a platform | Ground support clears and falling begins. |
| High displacement across a thin obstacle | The swept path catches the obstacle. |
| Diagonal corner or multiple simultaneous contacts | Stable result, unchanged when collider storage order is reversed. |
| Frozen airborne attack with zero displacement | Aeris is grounded only if there is actual support below the feet. |
| Enemy contact and respawn | Body, sprites, camera, and later interaction checks use the respawn position; no remaining enemy checks use the previous position. |
| Die during a grounded attack | The hitbox becomes inactive and the standing pose appears at respawn in the same frame. |
| Die during an airborne freeze or dash | The attack ends immediately; old momentum and freeze state are cleared, and normal gravity resumes. |
| Start a new attack after respawn | The animation starts at its first frame and no previous attack state carries over. |
| Invalid collision object or overlapping spawn | A useful error identifies the placement or map problem. |

For spider checks, use temporary test placements: put a wall in its path, then place it on a platform with an open edge. Keep its starting body clear of terrain. Confirm that it reverses at the wall, that its sprite flips with it, and that it falls from the platform, then restore the normal test-map placement.

Check movement at different frame rates and with the existing maximum frame duration. Do not compensate for collision defects by changing jump strength, speed, or gravity.

**Finished when:** Both actors consistently obey the same Tiled terrain, the automated checks pass, the manual scenarios behave as described, and the old floor-specific collision path is gone.

## How we proceed together

For each step, we discuss the idea, you implement that step, and I review the result and help with any failed checks. We move to the next step after its checkpoint passes.
