//
// Created by David Mihailescu on 13/09/2026.
//

#include "terrain/TerrainCollision.hpp"
#include "game/Constants.hpp"

#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

bool expectBounds(const char* scenario, const sf::FloatRect& actual, const sf::FloatRect& expected) {
    // These examples use whole pixels, which can be compared exactly as floats.
    if (actual == expected) {
        return true;
    }

    std::cerr << "FAIL: " << scenario << '\n'
              << "  expected position (" << expected.position.x << ", " << expected.position.y << "), size (" << expected.size.x << ", " << expected.size.y << ")\n"
              << "  actual position (" << actual.position.x << ", " << actual.position.y << "), size (" << actual.size.x << ", " << actual.size.y << ")\n";
    return false;
}

bool expectContacts(const char* scenario, const TerrainContacts& actual, const TerrainContacts& expected) {
    if (actual.floor == expected.floor && actual.ceiling == expected.ceiling && actual.leftWall == expected.leftWall && actual.rightWall == expected.rightWall) {
        return true;
    }

    std::cerr << "FAIL: " << scenario << " reported the wrong contacts\n"
              << "  expected floor=" << expected.floor << ", ceiling=" << expected.ceiling << ", leftWall=" << expected.leftWall << ", rightWall=" << expected.rightWall << '\n'
              << "  actual floor=" << actual.floor << ", ceiling=" << actual.ceiling << ", leftWall=" << actual.leftWall << ", rightWall=" << actual.rightWall << '\n';
    return false;
}

bool expectValidStationaryMovement(const char* scenario, const TerrainCollision& terrain, const sf::FloatRect& body) {
    // A body resting exactly against an edge is valid even when it does not move.
    try {
        const TerrainMove result = terrain.resolveMovement(body, {0.f, 0.f});
        return expectBounds(scenario, result.bounds, body);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << scenario << " was rejected: " << error.what() << '\n';
        return false;
    }
}

bool expectInvalidTerrain(const char* scenario, const sf::FloatRect& solid) {
    // Construction should reject this one invalid terrain rectangle.
    try {
        TerrainCollision({solid});
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << scenario << " threw another exception: " << error.what() << '\n';
        return false;
    }

    std::cerr << "FAIL: " << scenario << " did not throw std::invalid_argument\n";
    return false;
}

bool expectInvalidMovement(const char* scenario, const TerrainCollision& terrain, const sf::FloatRect& body, const sf::Vector2f displacement, const char* expectedMessage = nullptr) {
    // Bad body data, bad displacement, and initial penetration must all be rejected.
    try {
        terrain.resolveMovement(body, displacement);
    } catch (const std::invalid_argument& error) {
        // For overlaps, verify that the error identifies the solid without depending on its full wording.
        if (expectedMessage != nullptr) {
            const std::string_view actualMessage(error.what());
            const bool textIsMissing = actualMessage.find(expectedMessage) == std::string_view::npos;

            if (textIsMissing) {
                std::cerr << "FAIL: " << scenario << " expected message containing '" << expectedMessage << "', got: " << error.what() << '\n';
                return false;
            }
        }
        return true;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << scenario << " threw another exception: " << error.what() << '\n';
        return false;
    }

    std::cerr << "FAIL: " << scenario << " did not throw std::invalid_argument\n";
    return false;
}

// Checks full and zero displacement in empty terrain, including the absence of contacts.
int testEmptyTerrainMovement() {
    int failures = 0;
    // Empty terrain isolates basic movement from collision handling.
    const TerrainCollision emptyTerrain({});
    // World-pixel rectangle: top-left (10, 20), width 16, height 32.
    const sf::FloatRect startBounds{{10.f, 20.f}, {16.f, 32.f}};

    // Positive X moves right; negative Y moves up. With no obstacles, apply the full request.
    const TerrainMove moved = emptyTerrain.resolveMovement(startBounds, {5.f, -3.f});
    if (!expectBounds("empty terrain: full displacement", moved.bounds, {{15.f, 17.f}, {16.f, 32.f}})) {
        ++failures;
    }
    if (!expectContacts("empty terrain: full displacement", moved.contacts, {})) {
        ++failures;
    }

    const TerrainMove stationary = emptyTerrain.resolveMovement(startBounds, {0.f, 0.f});
    if (!expectBounds("empty terrain: zero displacement", stationary.bounds, startBounds)) {
        ++failures;
    }
    if (!expectContacts("empty terrain: zero displacement", stationary.contacts, {})) {
        ++failures;
    }

    return failures;
}

// Checks valid negative positions and rejects invalid terrain, body, and displacement values.
int testGeometryValidation() {
    int failures = 0;
    const TerrainCollision emptyTerrain({});
    const sf::FloatRect startBounds{{10.f, 20.f}, {16.f, 32.f}};

    // Negative world coordinates are valid for terrain and should be stored unchanged.
    const sf::FloatRect negativeSolid{{-20.f, -10.f}, {16.f, 32.f}};
    try {
        const TerrainCollision terrain({negativeSolid});
        if (!expectBounds("negative terrain position", terrain.getSolids().at(0), negativeSolid)) {
            ++failures;
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: negative terrain position was rejected: " << error.what() << '\n';
        ++failures;
    }

    // Reuse the rectangle as a body, but move it through empty terrain to test negative coordinates.
    try {
        const TerrainMove negativeMove = emptyTerrain.resolveMovement(negativeSolid, {5.f, -3.f});
        if (!expectBounds("negative body position", negativeMove.bounds, {{-15.f, -13.f}, {16.f, 32.f}})) {
            ++failures;
        }
        if (!expectContacts("negative body position", negativeMove.contacts, {})) {
            ++failures;
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: negative body position was rejected: " << error.what() << '\n';
        ++failures;
    }

    // NaN and infinity are special float values that geometry validation must reject.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();

    // The constructor validates each terrain rectangle and rejects invalid input.
    if (!expectInvalidTerrain("terrain with zero width", {{0.f, 0.f}, {0.f, 32.f}})) {
        ++failures;
    }
    if (!expectInvalidTerrain("terrain with negative height", {{0.f, 0.f}, {16.f, -32.f}})) {
        ++failures;
    }
    if (!expectInvalidTerrain("terrain with NaN y position", {{0.f, nan}, {16.f, 32.f}})) {
        ++failures;
    }
    if (!expectInvalidTerrain("terrain with infinite width", {{0.f, 0.f}, {infinity, 32.f}})) {
        ++failures;
    }
    // resolveMovement validates the body and the requested displacement.
    if (!expectInvalidMovement("body with zero height", emptyTerrain, {{0.f, 0.f}, {16.f, 0.f}}, {0.f, 0.f})) {
        ++failures;
    }
    if (!expectInvalidMovement("body with NaN x position", emptyTerrain, {{nan, 0.f}, {16.f, 32.f}}, {0.f, 0.f})) {
        ++failures;
    }
    if (!expectInvalidMovement("infinite x displacement", emptyTerrain, startBounds, {infinity, 0.f})) {
        ++failures;
    }
    if (!expectInvalidMovement("NaN y displacement", emptyTerrain, startBounds, {0.f, nan})) {
        ++failures;
    }

    return failures;
}

// Rejects initial penetration, even when moving out, while allowing exact edge contact.
int testStartingOverlap() {
    int failures = 0;

    // This wall covers x=10..20 and y=0..10. Positive shared area at the start is invalid.
    const sf::FloatRect wall{{10.f, 0.f}, {10.f, 10.f}};
    const TerrainCollision wallTerrain({wall});
    // One pixel of initial overlap is enough, even with no requested movement.
    if (!expectInvalidMovement("body starts one pixel inside wall", wallTerrain, {{5.f, 2.f}, {6.f, 6.f}}, {0.f, 0.f}, "overlapping terrain solid 0")) {
        ++failures;
    }
    // Moving out later cannot repair a body that began inside the wall.
    if (!expectInvalidMovement("body starts inside wall and moves out", wallTerrain, {{12.f, 2.f}, {4.f, 4.f}}, {20.f, 0.f}, "overlapping terrain solid 0")) {
        ++failures;
    }
    // The initial-overlap check must inspect the second solid, not just the first.
    const sf::FloatRect distantWall{{-20.f, 0.f}, {10.f, 10.f}};
    const TerrainCollision twoWalls({distantWall, wall});
    if (!expectInvalidMovement("body starts inside second wall", twoWalls, {{12.f, 2.f}, {4.f, 4.f}}, {0.f, 0.f}, "overlapping terrain solid 1")) {
        ++failures;
    }

    // Equal edges have no shared area. Test that contact on every side is accepted.
    const sf::FloatRect bodyTouchingLeftEdge{{0.f, 2.f}, {10.f, 6.f}};
    if (!expectValidStationaryMovement("touching wall's left edge", wallTerrain, bodyTouchingLeftEdge)) {
        ++failures;
    }
    if (!expectValidStationaryMovement("touching wall's right edge", wallTerrain, {{20.f, 2.f}, {10.f, 6.f}})) {
        ++failures;
    }
    if (!expectValidStationaryMovement("touching wall's top edge", wallTerrain, {{12.f, -5.f}, {6.f, 5.f}})) {
        ++failures;
    }
    if (!expectValidStationaryMovement("touching wall's bottom edge", wallTerrain, {{12.f, 10.f}, {6.f, 5.f}})) {
        ++failures;
    }

    // Contact must not hold the body against a surface when it requests movement away from it.
    const TerrainMove movingAway = wallTerrain.resolveMovement(bodyTouchingLeftEdge, {-5.f, 0.f});
    if (!expectBounds("move away from touching wall", movingAway.bounds, {{-5.f, 2.f}, {10.f, 6.f}})) {
        ++failures;
    }
    if (!expectContacts("move away from touching wall", movingAway.contacts, {})) {
        ++failures;
    }

    return failures;
}

// Distinguishes touching a corner for one instant from moving into a corner.
int testDiagonalCornerPaths() {
    int failures = 0;

    const sf::FloatRect solid{{10.f, 10.f}, {10.f, 10.f}};
    const TerrainCollision terrain({solid});

    // At time 0.5, the body's bottom-right corner touches the solid's top-left
    // corner. It then continues above the solid, so this point graze is not a collision.
    const sf::FloatRect grazingBody{{0.f, 15.f}, {5.f, 5.f}};
    const TerrainMove graze = terrain.resolveMovement(grazingBody, {10.f, -20.f});
    if (!expectBounds("diagonal point graze", graze.bounds, {{10.f, -5.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("diagonal point graze", graze.contacts, {})) {
        ++failures;
    }

    // Both axes enter the solid at time 0.5. The body must stop where its
    // bottom-right corner meets the solid and report both impacted body sides.
    const sf::FloatRect approachingBody{{0.f, 0.f}, {5.f, 5.f}};
    const TerrainMove cornerImpact = terrain.resolveMovement(approachingBody, {10.f, 10.f});
    if (!expectBounds("genuine diagonal corner collision", cornerImpact.bounds, {{5.f, 5.f}, {5.f, 5.f}})) {
        ++failures;
    }
    const TerrainContacts expectedCornerContacts{.floor = true, .rightWall = true};
    if (!expectContacts("genuine diagonal corner collision", cornerImpact.contacts, expectedCornerContacts)) {
        ++failures;
    }

    return failures;
}

// Checks that approaching each face stops at the surface and reports the body side that hit it.
int testApproachingEverySide() {
    int failures = 0;

    const sf::FloatRect solid{{10.f, 10.f}, {10.f, 10.f}};
    const TerrainCollision terrain({solid});

    const TerrainMove fromLeft = terrain.resolveMovement({{0.f, 12.f}, {5.f, 5.f}}, {10.f, 0.f});
    if (!expectBounds("approach from left", fromLeft.bounds, {{5.f, 12.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("approach from left", fromLeft.contacts, {.rightWall = true})) {
        ++failures;
    }

    const TerrainMove fromRight = terrain.resolveMovement({{25.f, 12.f}, {5.f, 5.f}}, {-10.f, 0.f});
    if (!expectBounds("approach from right", fromRight.bounds, {{20.f, 12.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("approach from right", fromRight.contacts, {.leftWall = true})) {
        ++failures;
    }

    const TerrainMove fromAbove = terrain.resolveMovement({{12.f, 0.f}, {5.f, 5.f}}, {0.f, 10.f});
    if (!expectBounds("approach from above", fromAbove.bounds, {{12.f, 5.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("approach from above", fromAbove.contacts, {.floor = true})) {
        ++failures;
    }

    const TerrainMove fromBelow = terrain.resolveMovement({{12.f, 25.f}, {5.f, 5.f}}, {0.f, -10.f});
    if (!expectBounds("approach from below", fromBelow.bounds, {{12.f, 20.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("approach from below", fromBelow.contacts, {.ceiling = true})) {
        ++failures;
    }

    return failures;
}

// Checks the complete swept path so large movement cannot tunnel through thin terrain.
int testCrossingThinTerrain() {
    int failures = 0;

    const TerrainCollision thinWallTerrain({{{10.f, 0.f}, {1.f, 10.f}}});
    const TerrainMove wallImpact = thinWallTerrain.resolveMovement({{0.f, 2.f}, {5.f, 5.f}}, {20.f, 0.f});
    if (!expectBounds("cross thin wall", wallImpact.bounds, {{5.f, 2.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("cross thin wall", wallImpact.contacts, {.rightWall = true})) {
        ++failures;
    }

    const TerrainCollision thinFloorTerrain({{{0.f, 10.f}, {10.f, 1.f}}});
    const TerrainMove floorImpact = thinFloorTerrain.resolveMovement({{2.f, 0.f}, {5.f, 5.f}}, {0.f, 20.f});
    if (!expectBounds("cross thin floor", floorImpact.bounds, {{2.f, 5.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("cross thin floor", floorImpact.contacts, {.floor = true})) {
        ++failures;
    }

    return failures;
}

int testEarliestCollision() {
    int failures = 0;
    constexpr sf::FloatRect nearWall = {{10.f, 0.f}, {10.f, 20.f}};
    constexpr sf::FloatRect farWall = {{30.f, 0.f}, {10.f, 20.f}};
    std::vector twoWalls = {nearWall, farWall};
    const TerrainCollision twoWallsTerrain(twoWalls);
    const TerrainMove impact = twoWallsTerrain.resolveMovement({{0.f, 5.f}, {5.f, 10.f}}, {100.f, 0.f});
    if (!expectBounds("nearest wall listed first", impact.bounds, {{5.f, 5.f}, {5.f, 10.f}})) {
        ++failures;
    }
    if (!expectContacts("nearest wall listed first", impact.contacts, {.rightWall = true})) {
        ++failures;
    }

    twoWalls = {farWall, nearWall};
    const TerrainCollision twoWallsTerrainCopy(twoWalls);
    const TerrainMove impactCopy = twoWallsTerrainCopy.resolveMovement({{0.f, 5.f}, {5.f, 10.f}}, {100.f, 0.f});

    if (!expectBounds("nearest wall listed second", impactCopy.bounds, {{5.f, 5.f}, {5.f, 10.f}})) {
        ++failures;
    }
    if (!expectContacts("nearest wall listed second", impactCopy.contacts, {.rightWall = true})) {
        ++failures;
    }

    return failures;
}

int testMultipleCollisions() {
    int failures = 0;
    constexpr sf::FloatRect floor = {{0.f, 20.f}, {20.f, 10.f}};
    constexpr sf::FloatRect rightWall = {{10.f, 0.f}, {10.f, 20.f}};
    const TerrainCollision terrain({floor, rightWall});
    TerrainMove impact = terrain.resolveMovement({{0.f, 0.f}, {5.f, 5.f}}, {8.f, 30.f});

    if (!expectBounds("two collisions in one request", impact.bounds, {{5.f, 15.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("two collisions in one request", impact.contacts, {.floor = true, .rightWall = true})) {
        ++failures;
    }

    return failures;
}

int testSimultaneousCollisions() {
    int failures = 0;
    constexpr sf::FloatRect floor = {{0.f, 10.f}, {10.f, 10.f}};
    constexpr sf::FloatRect rightWall = {{10.f, 0.f}, {10.f, 10.f}};
    constexpr sf::FloatRect body = {{0.f, 0.f}, {5.f, 5.f}};
    constexpr sf::Vector2f displacement = {10.f, 10.f};
    constexpr sf::FloatRect expectedBounds = {{5.f, 5.f}, {5.f, 5.f}};
    constexpr TerrainContacts expectedContacts{.floor = true, .rightWall = true};

    const TerrainCollision floorFirstTerrain({floor, rightWall});
    const TerrainMove floorFirstImpact = floorFirstTerrain.resolveMovement(body, displacement);
    if (!expectBounds("simultaneous impacts with floor listed first", floorFirstImpact.bounds, expectedBounds)) {
        ++failures;
    }
    if (!expectContacts("simultaneous impacts with floor listed first", floorFirstImpact.contacts, expectedContacts)) {
        ++failures;
    }

    const TerrainCollision wallFirstTerrain({rightWall, floor});
    const TerrainMove wallFirstImpact = wallFirstTerrain.resolveMovement(body, displacement);
    if (!expectBounds("simultaneous impacts with wall listed first", wallFirstImpact.bounds, expectedBounds)) {
        ++failures;
    }
    if (!expectContacts("simultaneous impacts with wall listed first", wallFirstImpact.contacts, expectedContacts)) {
        ++failures;
    }

    return failures;
}

// Treats impacts separated only by the time tolerance as simultaneous, stopping at the earlier surface.
int testNearlySimultaneousCollisions() {
    int failures = 0;
    constexpr float movementDistance = 100.f;
    constexpr float timeDifference = Constants::Physics::collisionTimeTolerance / 4.f;
    constexpr sf::FloatRect floor = {{0.f, 55.f}, {200.f, 10.f}};
    constexpr sf::FloatRect rightWall = {{55.f + movementDistance * timeDifference, 0.f}, {10.f, 200.f}};
    constexpr sf::FloatRect body = {{0.f, 0.f}, {5.f, 5.f}};
    constexpr sf::Vector2f displacement = {movementDistance, movementDistance};

    // The wall is listed first and is reached a tiny bit later than the floor.
    // The tolerance must merge their contacts and std::min must retain the floor's earlier time.
    const TerrainCollision terrain({rightWall, floor});
    const TerrainMove impact = terrain.resolveMovement(body, displacement);
    if (!expectBounds("nearly simultaneous floor and wall", impact.bounds, {{50.f, 50.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("nearly simultaneous floor and wall", impact.contacts, {.floor = true, .rightWall = true})) {
        ++failures;
    }

    return failures;
}

int testAdjoiningFloorSeams() {
    int failures = 0;
    constexpr sf::FloatRect leftFloor = {{0.f, 10.f}, {10.f, 10.f}};
    constexpr sf::FloatRect rightFloor = {{10.f, 10.f}, {10.f, 10.f}};
    const TerrainCollision terrain({leftFloor, rightFloor});

    const TerrainMove movingRight = terrain.resolveMovement({{5.f, 5.f}, {5.f, 5.f}}, {4.f, 1.f});
    if (!expectBounds("cross floor seam moving right", movingRight.bounds, {{9.f, 5.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("cross floor seam moving right", movingRight.contacts, {.floor = true})) {
        ++failures;
    }

    const TerrainMove movingLeft = terrain.resolveMovement({{10.f, 5.f}, {5.f, 5.f}}, {-4.f, 1.f});
    if (!expectBounds("cross floor seam moving left", movingLeft.bounds, {{6.f, 5.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("cross floor seam moving left", movingLeft.contacts, {.floor = true})) {
        ++failures;
    }

    constexpr sf::FloatRect upperWall = {{20.f, 0.f}, {10.f, 10.f}};
    constexpr sf::FloatRect lowerWall = {{20.f, 10.f}, {10.f, 10.f}};
    const TerrainCollision wallTerrain({upperWall, lowerWall});

    const TerrainMove movingDown = wallTerrain.resolveMovement({{15.f, 5.f}, {5.f, 5.f}}, {5.f, 5.f});
    if (!expectBounds("cross wall seam moving down", movingDown.bounds, {{15.f, 10.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("cross wall seam moving down", movingDown.contacts, {.rightWall = true})) {
        ++failures;
    }

    return failures;
}

// Checks final-position ground support, corner-only contact, and the vertical tolerance.
int testGroundSupport() {
    int failures = 0;
    constexpr float groundSupportTolerance = Constants::Physics::groundSupportTolerance;
    constexpr sf::FloatRect floor = {{10.f, 20.f}, {20.f, 10.f}};
    const TerrainCollision terrain({floor});

    // Feet are at y=20. Different X and Y values catch a mistaken X-based bottom calculation.
    constexpr sf::FloatRect supportedBody = {{12.f, 15.f}, {5.f, 5.f}};
    if (!terrain.hasGroundSupport(supportedBody)) {
        std::cerr << "FAIL: body standing on floor should have ground support\n";
        ++failures;
    }

    // The body's bottom-left corner meets the floor's top-right corner, with no horizontal overlap.
    constexpr sf::FloatRect cornerOnlyBody = {{30.f, 15.f}, {5.f, 5.f}};
    if (terrain.hasGroundSupport(cornerOnlyBody)) {
        std::cerr << "FAIL: corner-only contact should not provide ground support\n";
        ++failures;
    }

    // A gap of half the tolerance is accepted as rounding noise.
    constexpr sf::FloatRect withinToleranceBody = {{12.f, 15.f - groundSupportTolerance / 2.f}, {5.f, 5.f}};
    if (!terrain.hasGroundSupport(withinToleranceBody)) {
        std::cerr << "FAIL: gap within ground support tolerance should be accepted\n";
        ++failures;
    }

    // A gap of twice the tolerance is too large to count as feet meeting the floor.
    constexpr sf::FloatRect beyondToleranceBody = {{12.f, 15.f - groundSupportTolerance * 2.f}, {5.f, 5.f}};
    if (terrain.hasGroundSupport(beyondToleranceBody)) {
        std::cerr << "FAIL: gap beyond ground support tolerance should be rejected\n";
        ++failures;
    }

    return failures;
}

// Checks support using the resolver's final bounds after idle, ledge, and jump movement.
int testGroundSupportAfterMovement() {
    int failures = 0;
    constexpr sf::FloatRect floor = {{10.f, 20.f}, {20.f, 10.f}};
    constexpr sf::FloatRect standingBody = {{12.f, 15.f}, {5.f, 5.f}};
    const TerrainCollision terrain({floor});

    // No movement produces no impact, but the unchanged feet still rest on the floor.
    const TerrainMove idle = terrain.resolveMovement(standingBody, {0.f, 0.f});
    if (!expectBounds("idle on floor", idle.bounds, standingBody)) {
        ++failures;
    }
    if (!expectContacts("idle on floor", idle.contacts, {})) {
        ++failures;
    }
    if (!terrain.hasGroundSupport(idle.bounds)) {
        std::cerr << "FAIL: idle body should retain ground support without a floor impact\n";
        ++failures;
    }

    // Horizontal movement carries the whole body beyond the floor's right edge at x=30.
    const TerrainMove walkedOff = terrain.resolveMovement(standingBody, {20.f, 0.f});
    if (!expectBounds("walk off ledge", walkedOff.bounds, {{32.f, 15.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("walk off ledge", walkedOff.contacts, {})) {
        ++failures;
    }
    if (terrain.hasGroundSupport(walkedOff.bounds)) {
        std::cerr << "FAIL: body that walked off a ledge should lose ground support\n";
        ++failures;
    }

    // Land at t=0.5, then slide off with the remaining X movement.
    // The floor impact stays recorded even though there is no support at the final position.
    const TerrainMove landedThenSlidOff = terrain.resolveMovement({{12.f, 5.f}, {5.f, 5.f}}, {20.f, 20.f});
    if (!expectBounds("land then slide off ledge", landedThenSlidOff.bounds, {{32.f, 15.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("land then slide off ledge", landedThenSlidOff.contacts, {.floor = true})) {
        ++failures;
    }
    if (terrain.hasGroundSupport(landedThenSlidOff.bounds)) {
        std::cerr << "FAIL: earlier floor impact should not preserve support after sliding off\n";
        ++failures;
    }

    // Touching the floor initially must not block a jump away from it.
    const TerrainMove jumped = terrain.resolveMovement(standingBody, {0.f, -5.f});
    if (!expectBounds("jump away from floor", jumped.bounds, {{12.f, 10.f}, {5.f, 5.f}})) {
        ++failures;
    }
    if (!expectContacts("jump away from floor", jumped.contacts, {})) {
        ++failures;
    }
    if (terrain.hasGroundSupport(jumped.bounds)) {
        std::cerr << "FAIL: body that jumped above the floor should lose ground support\n";
        ++failures;
    }

    return failures;
}

int main() {
    int failures = 0;
    failures += testEmptyTerrainMovement();
    failures += testGeometryValidation();
    failures += testStartingOverlap();
    failures += testDiagonalCornerPaths();
    failures += testApproachingEverySide();
    failures += testCrossingThinTerrain();
    failures += testEarliestCollision();
    failures += testMultipleCollisions();
    failures += testSimultaneousCollisions();
    failures += testNearlySimultaneousCollisions();
    failures += testAdjoiningFloorSeams();
    failures += testGroundSupport();
    failures += testGroundSupportAfterMovement();
    // CTest uses the process exit code: zero passes, any nonzero value fails.
    return failures == 0 ? 0 : 1;
}
