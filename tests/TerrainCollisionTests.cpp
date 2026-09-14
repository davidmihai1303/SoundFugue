//
// Created by David Mihailescu on 13/09/2026.
//

#include "terrain/TerrainCollision.hpp"

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
              << "  expected position (" << expected.position.x << ", " << expected.position.y << "), size ("
              << expected.size.x << ", " << expected.size.y << ")\n"
              << "  actual position (" << actual.position.x << ", " << actual.position.y << "), size ("
              << actual.size.x << ", " << actual.size.y << ")\n";
    return false;
}

bool expectNoContacts(const char* scenario, const TerrainContacts& contacts) {
    // Empty terrain should not report a floor, ceiling, or wall impact.
    if (!contacts.floor && !contacts.ceiling && !contacts.leftWall && !contacts.rightWall) {
        return true;
    }

    std::cerr << "FAIL: " << scenario << " should have no contacts\n"
              << "  actual floor=" << contacts.floor << ", ceiling=" << contacts.ceiling
              << ", leftWall=" << contacts.leftWall << ", rightWall=" << contacts.rightWall << '\n';
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

bool expectInvalidMovement(const char* scenario, const TerrainCollision& terrain, const sf::FloatRect& body,
                           const sf::Vector2f displacement, const char* expectedMessage = nullptr) {
    // Bad body data, bad displacement, and initial penetration must all be rejected.
    try {
        terrain.resolveMovement(body, displacement);
    } catch (const std::invalid_argument& error) {
        // For overlaps, verify that the error identifies the solid without depending on its full wording.
        if (expectedMessage != nullptr) {
            const std::string_view actualMessage(error.what());
            const bool textIsMissing = actualMessage.find(expectedMessage) == std::string_view::npos;

            if (textIsMissing) {
                std::cerr << "FAIL: " << scenario << " expected message containing '" << expectedMessage
                          << "', got: " << error.what() << '\n';
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

int main() {
    int failures = 0;
    // Empty terrain isolates basic movement and input validation from collision handling.
    const TerrainCollision emptyTerrain({});
    // World-pixel rectangle: top-left (10, 20), width 16, height 32.
    const sf::FloatRect startBounds{{10.f, 20.f}, {16.f, 32.f}};

    // Positive X moves right; negative Y moves up. With no obstacles, apply the full request.
    const TerrainMove moved = emptyTerrain.resolveMovement(startBounds, {5.f, -3.f});
    if (!expectBounds("empty terrain: full displacement", moved.bounds, {{15.f, 17.f}, {16.f, 32.f}})) {
        ++failures;
    }
    if (!expectNoContacts("empty terrain: full displacement", moved.contacts)) {
        ++failures;
    }

    const TerrainMove stationary = emptyTerrain.resolveMovement(startBounds, {0.f, 0.f});
    if (!expectBounds("empty terrain: zero displacement", stationary.bounds, startBounds)) {
        ++failures;
    }
    if (!expectNoContacts("empty terrain: zero displacement", stationary.contacts)) {
        ++failures;
    }

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
        if (!expectNoContacts("negative body position", negativeMove.contacts)) {
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

    // This wall covers x=10..20 and y=0..10. Positive shared area at the start is invalid.
    const sf::FloatRect wall{{10.f, 0.f}, {10.f, 10.f}};
    const TerrainCollision wallTerrain({wall});
    // One pixel of initial overlap is enough, even with no requested movement.
    if (!expectInvalidMovement("body starts one pixel inside wall", wallTerrain,
                               {{5.f, 2.f}, {6.f, 6.f}}, {0.f, 0.f}, "overlapping terrain solid 0")) {
        ++failures;
    }
    // Moving out later cannot repair a body that began inside the wall.
    if (!expectInvalidMovement("body starts inside wall and moves out", wallTerrain,
                               {{12.f, 2.f}, {4.f, 4.f}}, {20.f, 0.f}, "overlapping terrain solid 0")) {
        ++failures;
    }
    // The initial-overlap check must inspect the second solid, not just the first.
    const sf::FloatRect distantWall{{-20.f, 0.f}, {10.f, 10.f}};
    const TerrainCollision twoWalls({distantWall, wall});
    if (!expectInvalidMovement("body starts inside second wall", twoWalls,
                               {{12.f, 2.f}, {4.f, 4.f}}, {0.f, 0.f}, "overlapping terrain solid 1")) {
        ++failures;
    }

    // Equal edges have no shared area. Test that contact on every side is accepted.
    if (!expectValidStationaryMovement("touching wall's left edge", wallTerrain, {{0.f, 2.f}, {10.f, 6.f}})) {
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

    // CTest uses the process exit code: zero passes, any nonzero value fails.
    return failures == 0 ? 0 : 1;
}
