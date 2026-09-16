#include "terrain/TerrainCollision.hpp"

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

std::optional<TerrainCollision::AxisInterval> TerrainCollision::calculateAxisInterval(const float bodyMin, const float bodyMax, const float solidMin, const float solidMax, const float displacement) {
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

TerrainCollision::TerrainCollision(std::vector<sf::FloatRect> solids)
    : m_solids(std::move(solids)) {
    // Validate every rectangle once when the terrain collection is created.
    for (std::size_t index = 0; index < m_solids.size(); ++index) {
        validateRectangle(m_solids[index], "terrain solid " + std::to_string(index));
    }
}

TerrainCollision::~TerrainCollision() = default;

TerrainMove TerrainCollision::resolveMovement(sf::FloatRect startBounds, const sf::Vector2f displacement) const {
    // Reject invalid geometry before doing any collision calculations.
    validateRectangle(startBounds, "body");
    if (!std::isfinite(displacement.x)) {
        throw std::invalid_argument("displacement: x must be finite");
    }
    if (!std::isfinite(displacement.y)) {
        throw std::invalid_argument("displacement: y must be finite");
    }

    // A valid movement must begin outside every terrain solid.
    for (std::size_t index = 0; index < m_solids.size(); ++index) {
        // Edge contact has no intersection area and is a valid starting position.
        if (startBounds.findIntersection(m_solids[index])) {
            throw std::invalid_argument("body's starting position is overlapping terrain solid " + std::to_string(index));
        }
    }

    for (const sf::FloatRect& solid : m_solids) {
        const std::optional<AxisInterval> xInterval = calculateAxisInterval(
            startBounds.position.x, startBounds.position.x + startBounds.size.x,
            solid.position.x, solid.position.x + solid.size.x, displacement.x);

        const std::optional<AxisInterval> yInterval = calculateAxisInterval(
            startBounds.position.y, startBounds.position.y + startBounds.size.y,
            solid.position.y, solid.position.y + solid.size.y, displacement.y);

        // Both axes need to have an overlap interval for a collision to be possible. (Required but not enough - the 2 intervals need a valid intersection interval)
        if (!xInterval || !yInterval) {
            continue;
        }

        // Two-dimensional overlap begins when the later axis enters and ends when the earlier axis leaves.
        const float enterTime = std::max(xInterval->enterTime, yInterval->enterTime);
        const float leaveTime = std::min(xInterval->leaveTime, yInterval->leaveTime);

        // Ignore point grazes and overlap intervals that begin outside this movement request.
        if (enterTime >= leaveTime || enterTime < 0.f || enterTime > 1.f) {
            continue;
        }

        // The axis that enters last identifies the impacted side. Equal entry times impact a corner.
        TerrainContacts impactContacts{};
        if (xInterval->enterTime >= yInterval->enterTime) {
            if (displacement.x > 0.f) {
                impactContacts.rightWall = true;
            } else if (displacement.x < 0.f) {
                impactContacts.leftWall = true;
            }
        }
        if (yInterval->enterTime >= xInterval->enterTime) {
            if (displacement.y > 0.f) {
                impactContacts.floor = true;
            } else if (displacement.y < 0.f) {
                impactContacts.ceiling = true;
            }
        }

        // The next step will use the candidate time and contacts to stop the body at impact.
    }

    // Collision stopping is added in the following sweep steps; empty terrain uses the full displacement.
    startBounds.position += displacement;
    return {startBounds, {}};
}

const std::vector<sf::FloatRect>& TerrainCollision::getSolids() const {
    return m_solids;
}
