//
// Created by david on 11/2/2025.
//

#ifndef SOUNDFUGUE_ENEMY_HPP
#define SOUNDFUGUE_ENEMY_HPP

#include <SFML/Graphics.hpp>
#include "entities/Entity.hpp"
#include "terrain/TerrainCollision.hpp"

// Shared base for every regular enemy. It owns the order a frame runs in; each concrete enemy supplies its own intended movement and its own animations. Abstract: movementLogic() and draw() stay pure virtual from Entity, and animationLogic() is added below.
class Enemy : public Entity {
public:
    friend std::ostream& operator<<(std::ostream& os, const Enemy& e);

    // The frame order every enemy shares: decide intended movement, advance animations, then record this frame's facing for the next one.
    void update(sf::Time dt, const TerrainCollision& terrain) override;

    void attackingLogic() override;

    void setColor(sf::Color colour);

protected:
    // Only derived enemies construct one. Enemy has no movement or animations of its own, so it sets up just the physical body and the facing bookkeeping.
    Enemy(const sf::Vector2f& position, const sf::Vector2f& size);

    // Each enemy owns its own sprites and animation timers, so each selects and advances its own.
    virtual void animationLogic(sf::Time dt) = 0;
};

#endif //SOUNDFUGUE_ENEMY_HPP
