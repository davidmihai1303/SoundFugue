#include "terrain/TerrainCollision.hpp"
#include "game/Constants.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

// Positions may be negative, but every value must be finite and both dimensions must be positive.
void validateRectangle(const sf::FloatRect& rect, const std::string& label) {
    if (!std::isfinite(rect.position.x)) {
        throw std::invalid_argument(label + ": x position must be finite");
    }
    if (!std::isfinite(rect.position.y)) {
        throw std::invalid_argument(label + ": y position must be finite");
    }
    if (!std::isfinite(rect.size.x)) {
        throw std::invalid_argument(label + ": width must be finite");
    }
    if (!std::isfinite(rect.size.y)) {
        throw std::invalid_argument(label + ": height must be finite");
    }
    if (rect.size.x <= 0.f) {
        throw std::invalid_argument(label + ": width must be greater than zero");
    }
    if (rect.size.y <= 0.f) {
        throw std::invalid_argument(label + ": height must be greater than zero");
    }
}

std::optional<TerrainCollision::AxisInterval>
TerrainCollision::calculateAxisInterval(const float bodyMin, const float bodyMax, const float solidMin,
                                        const float solidMax, const float displacement) {
    // With no movement, the spans either remain overlapped for the whole request or never overlap.
    if (displacement == 0.f) {
        const bool spansOverlap = bodyMin < solidMax && bodyMax > solidMin;
        // Separated spans and spans that only touch at a boundary have no positive overlap.
        if (!spansOverlap) {
            // Return std::nullopt so optional has no value at runtime (i.e. there is no overlap time).
            return std::nullopt;
        }
        // The spans overlap, and the body doesn't move, so the time of the overlap is infinite.
        constexpr float infinity = std::numeric_limits<float>::infinity();
        return AxisInterval{-infinity, infinity};
    }

    // Find when opposite body and solid edges meet as the body moves along this axis.
    const float solidMinMeetingTime = (solidMin - bodyMax) / displacement;
    const float solidMaxMeetingTime = (solidMax - bodyMin) / displacement;

    // Negative movement reverses their order, so always return the earlier time first.
    if (solidMinMeetingTime < solidMaxMeetingTime) {
        return AxisInterval{solidMinMeetingTime, solidMaxMeetingTime};
    }
    return AxisInterval{solidMaxMeetingTime, solidMinMeetingTime};
}

TerrainCollision::TerrainCollision(std::vector<sf::FloatRect> solids) : m_solids(std::move(solids)) {
    // Validate every rectangle once when the terrain collection is created.
    for (std::size_t index = 0; index < m_solids.size(); ++index) {
        validateRectangle(m_solids[index], "terrain solid " + std::to_string(index));
    }
}

void TerrainCollision::validatePlacement(const sf::FloatRect& body, const std::string& label) const {
    validateRectangle(body, label);
    for (std::size_t index = 0; index < m_solids.size(); ++index) {
        // Edge contact has no intersection area and is a valid position.
        if (body.findIntersection(m_solids[index])) {
            throw std::invalid_argument(label + "'s starting position is overlapping terrain solid " + std::to_string(index));
        }
    }
}

TerrainMove TerrainCollision::resolveMovement(sf::FloatRect startBounds, const sf::Vector2f displacement) const {
    constexpr float collisionTimeTolerance = Constants::Physics::collisionTimeTolerance;

    // Reject invalid geometry before doing any collision calculations.
    // A valid movement must begin outside every terrain solid.
    validatePlacement(startBounds, "body");
    if (!std::isfinite(displacement.x)) {
        throw std::invalid_argument("displacement: x must be finite");
    }
    if (!std::isfinite(displacement.y)) {
        throw std::invalid_argument("displacement: y must be finite");
    }

    sf::Vector2f remainingDisplacement = displacement;
    TerrainContacts contacts{};

    constexpr int maxSweepIterations = 4;
    for (int iteration = 0; iteration < maxSweepIterations; ++iteration) {
        if (remainingDisplacement.x == 0.f && remainingDisplacement.y == 0.f)
            break;

        const sf::Vector2f sweepDisplacement = remainingDisplacement;
        bool impactFound = false;
        float earliestImpactTime = 1.f;
        // Keep face and corner candidates separate so internal terrain seams can be identified later.
        TerrainContacts earliestFaceContacts{};
        TerrainContacts earliestCornerContacts{};

        for (const sf::FloatRect& solid : m_solids) {
            const std::optional<AxisInterval> xInterval =
                calculateAxisInterval(startBounds.position.x, startBounds.position.x + startBounds.size.x,
                                      solid.position.x, solid.position.x + solid.size.x, sweepDisplacement.x);

            const std::optional<AxisInterval> yInterval =
                calculateAxisInterval(startBounds.position.y, startBounds.position.y + startBounds.size.y,
                                      solid.position.y, solid.position.y + solid.size.y, sweepDisplacement.y);

            // Both axes need to have an overlap interval for a collision to be possible. (Required but not enough - the 2 intervals need a valid intersection interval)
            if (!xInterval || !yInterval)
                continue;

            // Two-dimensional overlap begins when the later axis enters and ends when the earlier axis leaves.
            float enterTime = std::max(xInterval->enterTime, yInterval->enterTime);
            const float leaveTime = std::min(xInterval->leaveTime, yInterval->leaveTime);

            // Ignore point grazes and intervals clearly outside this sweep.
            // Values just beyond a boundary are accepted as rounding noise and clamped back into the normalized range.
            if (enterTime >= leaveTime - collisionTimeTolerance || enterTime < -collisionTimeTolerance ||
                enterTime > 1.f + collisionTimeTolerance)
                continue;
            enterTime = std::clamp(enterTime, 0.f, 1.f);

            // From this point downwards we have a confirmed collision

            // Nearly equal axis entry times mean this solid is entered through a corner.
            const bool cornerCollision =
                std::abs(xInterval->enterTime - yInterval->enterTime) <= collisionTimeTolerance;

            // The axis that enters last identifies the impacted side. Equal entry times impact a corner.
            TerrainContacts impactContacts{};
            if (cornerCollision || xInterval->enterTime > yInterval->enterTime) {
                if (sweepDisplacement.x > 0.f)
                    impactContacts.rightWall = true;
                else if (sweepDisplacement.x < 0.f)
                    impactContacts.leftWall = true;
            }
            if (cornerCollision || yInterval->enterTime > xInterval->enterTime) {
                if (sweepDisplacement.y > 0.f)
                    impactContacts.floor = true;
                else if (sweepDisplacement.y < 0.f)
                    impactContacts.ceiling = true;
            }

            // Keep the earliest impact.
            if (!impactFound || enterTime < earliestImpactTime - collisionTimeTolerance) {
                impactFound = true;
                earliestImpactTime = enterTime;

                // Contacts saved for the previous impact time are no longer relevant.
                earliestFaceContacts = {};
                earliestCornerContacts = {};
                if (cornerCollision) {
                    earliestCornerContacts = impactContacts;
                } else {
                    earliestFaceContacts = impactContacts;
                }
            }
            // Merge contacts whose impact times differ only by floating-point rounding (i.e. happen at the same time).
            else if (std::abs(enterTime - earliestImpactTime) <= collisionTimeTolerance) {
                // Moving to the smaller time avoids advancing slightly beyond either surface.
                earliestImpactTime = std::min(earliestImpactTime, enterTime);

                // Merge this candidate only into its own category to preserve where each contact came from.
                if (cornerCollision) {
                    earliestCornerContacts.floor = earliestCornerContacts.floor || impactContacts.floor;
                    earliestCornerContacts.ceiling = earliestCornerContacts.ceiling || impactContacts.ceiling;
                    earliestCornerContacts.leftWall = earliestCornerContacts.leftWall || impactContacts.leftWall;
                    earliestCornerContacts.rightWall = earliestCornerContacts.rightWall || impactContacts.rightWall;
                } else {
                    earliestFaceContacts.floor = earliestFaceContacts.floor || impactContacts.floor;
                    earliestFaceContacts.ceiling = earliestFaceContacts.ceiling || impactContacts.ceiling;
                    earliestFaceContacts.leftWall = earliestFaceContacts.leftWall || impactContacts.leftWall;
                    earliestFaceContacts.rightWall = earliestFaceContacts.rightWall || impactContacts.rightWall;
                }
            }
        }

        const bool hasFloorOrCeilingFace = earliestFaceContacts.floor || earliestFaceContacts.ceiling;
        const bool hasWallFace = earliestFaceContacts.leftWall || earliestFaceContacts.rightWall;

        // Face contacts are always preserved. Corner contacts are kept only when a perpendicular face does not prove that the corner belongs to an internal seam.
        TerrainContacts earliestImpactContacts{};
        earliestImpactContacts = earliestFaceContacts;
        if (!hasWallFace) {
            earliestImpactContacts.floor = earliestImpactContacts.floor || earliestCornerContacts.floor;
            earliestImpactContacts.ceiling = earliestImpactContacts.ceiling || earliestCornerContacts.ceiling;
        }
        if (!hasFloorOrCeilingFace) {
            earliestImpactContacts.leftWall = earliestImpactContacts.leftWall || earliestCornerContacts.leftWall;
            earliestImpactContacts.rightWall = earliestImpactContacts.rightWall || earliestCornerContacts.rightWall;
        }

        // Move to the impact and keep the unused part of the request for sliding.
        startBounds.position.x += sweepDisplacement.x * earliestImpactTime;
        startBounds.position.y += sweepDisplacement.y * earliestImpactTime;
        remainingDisplacement = {sweepDisplacement.x * (1.f - earliestImpactTime),
                                 sweepDisplacement.y * (1.f - earliestImpactTime)};

        // Separate checks let a corner block both remaining components.
        if (earliestImpactContacts.floor || earliestImpactContacts.ceiling)
            remainingDisplacement.y = 0.f;
        if (earliestImpactContacts.leftWall || earliestImpactContacts.rightWall)
            remainingDisplacement.x = 0.f;

        // Preserve both collisions
        contacts.floor = contacts.floor || earliestImpactContacts.floor;
        contacts.ceiling = contacts.ceiling || earliestImpactContacts.ceiling;
        contacts.leftWall = contacts.leftWall || earliestImpactContacts.leftWall;
        contacts.rightWall = contacts.rightWall || earliestImpactContacts.rightWall;
    }

    return {startBounds, contacts};
}

bool TerrainCollision::hasGroundSupport(const sf::FloatRect& bodyBounds) const {
    constexpr float groundSupportTolerance = Constants::Physics::groundSupportTolerance;
    validateRectangle(bodyBounds, "body");
    const float bodyLeft = bodyBounds.position.x;
    const float bodyRight = bodyBounds.position.x + bodyBounds.size.x;
    const float bodyBottom = bodyBounds.position.y + bodyBounds.size.y;

    for (const sf::FloatRect& solid : m_solids) {
        const bool feetMeetTop = std::abs(bodyBottom - solid.position.y) <= groundSupportTolerance;
        const bool overlapsHorizontally = bodyLeft < solid.position.x + solid.size.x && bodyRight > solid.position.x;
        if (feetMeetTop && overlapsHorizontally)
            return true;
    }
    return false;
}

const std::vector<sf::FloatRect>& TerrainCollision::getSolids() const {
    return m_solids;
}

TerrainCollision::~TerrainCollision() = default;