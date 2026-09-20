//
// Created by david on 11/4/2025.
//

#ifndef SOUNDFUGUE_ENTITY_HPP
#define SOUNDFUGUE_ENTITY_HPP

#include <SFML/Graphics.hpp>
#include "terrain/TerrainCollision.hpp"

class Entity {
public:
    friend std::ostream& operator<<(std::ostream& os, const Entity& e);

    Entity();

    virtual ~Entity() = default;

    virtual void update(sf::Time dt, const TerrainCollision &terrain) = 0;

    virtual void draw(sf::RenderTarget &target) const = 0;

    virtual sf::Vector2f movementLogic(sf::Time dt) = 0;

    virtual void attackingLogic() = 0;

    [[nodiscard]] sf::FloatRect getBounds() const;

    [[nodiscard]] sf::Vector2f getPosition() const;

    virtual void setPosition(const sf::Vector2f &position);

    [[nodiscard]] bool getAttackingState() const;

    [[nodiscard]] sf::Vector2f getVelocity() const;

    void setVelocity(const sf::Vector2f &velocity);

protected:
    sf::RectangleShape m_shape;
    sf::Vector2f m_velocity;

    bool m_isMoving;

    // Attack logic
    bool m_isAttacking;
    sf::Clock m_activeAttackClock;
    sf::Clock m_cooldownAttackClock;

    // Using a bool to see if the entity is facing left or right. True = Right and False = Left. Entity can only attack in the direction he is facing
    // Player default = True / Right
    // Enemy default = False / Left (this will be revised. Code=3)
    bool m_currentFacingDirection;
    bool m_lastFacingDirection;
};


#endif //SOUNDFUGUE_ENTITY_HPP
