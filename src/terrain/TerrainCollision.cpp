#include "terrain/TerrainCollision.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

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

TerrainCollision::TerrainCollision(std::vector<sf::FloatRect> solids)
    : m_solids(std::move(solids)) {
    for (std::size_t index = 0; index < m_solids.size(); ++index) {
        validateRectangle(m_solids[index], "terrain solid " + std::to_string(index));
    }
}

TerrainCollision::~TerrainCollision() = default;

TerrainMove TerrainCollision::resolveMovement(sf::FloatRect startBounds, const sf::Vector2f displacement) const {
    validateRectangle(startBounds, "body");
    if (!std::isfinite(displacement.x)) {
        throw std::invalid_argument("displacement: x must be finite");
    }
    if (!std::isfinite(displacement.y)) {
        throw std::invalid_argument("displacement: y must be finite");
    }

    for (std::size_t index = 0; index < m_solids.size(); ++index) {
        // Edge contact has no intersection area and is a valid starting position.
        if (startBounds.findIntersection(m_solids[index])) {
            throw std::invalid_argument("body's starting position is overlapping terrain solid " + std::to_string(index));
        }
    }

    startBounds.position += displacement;
    return {startBounds, {}};
}

const std::vector<sf::FloatRect>& TerrainCollision::getSolids() const {
    return m_solids;
}
