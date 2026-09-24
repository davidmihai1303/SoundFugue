#ifndef SOUNDFUGUE_TERRAINCOLLISION_HPP
#define SOUNDFUGUE_TERRAINCOLLISION_HPP

#include <SFML/Graphics/Rect.hpp>
#include <optional>
#include <string>
#include <vector>

#include "terrain/TerrainMove.hpp"

// Stores the map's solid rectangles and resolves an actor body's requested movement against them.
class TerrainCollision {
public:
    // Takes ownership of the supplied terrain rectangles and validates them.
    explicit TerrainCollision(std::vector<sf::FloatRect> solids);
    ~TerrainCollision();

    // Throws std::invalid_argument if the body's bounds are invalid or it overlaps a solid. Touching a solid is valid.
    // The label names the body in the error message.
    void validatePlacement(const sf::FloatRect& body, const std::string& label) const;

    // Starts with the supplied body bounds and returns its resulting bounds and terrain contacts.
    [[nodiscard]] TerrainMove resolveMovement(sf::FloatRect startBounds, sf::Vector2f displacement) const;
    // Returns the stored rectangles without copying them.
    [[nodiscard]] const std::vector<sf::FloatRect>& getSolids() const;
    // We need to know if the final position is supported by a solid
    [[nodiscard]] bool hasGroundSupport(const sf::FloatRect& bodyBounds) const;

private:
    // The range of normalized movement times during which the body's span has positive overlap with a solid's span on one axis. Time 0 is the start and time 1 is the requested end.
    struct AxisInterval {
        float enterTime;
        float leaveTime;
    };

    // Returns the overlap interval for one axis, or no value when no interval exists.
    static std::optional<AxisInterval> calculateAxisInterval(float bodyMin, float bodyMax, float solidMin, float solidMax, float displacement);

    // Collision rectangles that make up the solid terrain in world pixels.
    std::vector<sf::FloatRect> m_solids;
};

#endif // SOUNDFUGUE_TERRAINCOLLISION_HPP
