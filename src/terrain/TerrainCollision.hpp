#ifndef SOUNDFUGUE_TERRAINCOLLISION_HPP
#define SOUNDFUGUE_TERRAINCOLLISION_HPP

#include <SFML/Graphics/Rect.hpp>
#include <vector>

#include "terrain/TerrainMove.hpp"

class TerrainCollision {
public:
    explicit TerrainCollision(std::vector<sf::FloatRect> solids);
    ~TerrainCollision();

    TerrainMove resolveMovement(sf::FloatRect startBounds, sf::Vector2f displacement) const;
    [[nodiscard]] const std::vector<sf::FloatRect>& getSolids() const;

private:
    std::vector<sf::FloatRect> m_solids;
};

#endif // SOUNDFUGUE_TERRAINCOLLISION_HPP
