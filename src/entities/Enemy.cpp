//
// Created by david on 11/2/2025.
//

#include "entities/Enemy.hpp"
#include "game/Constants.hpp"

Enemy::Enemy(const sf::Vector2f& position, const sf::Vector2f& size)
{
    m_shape.setSize(size);
    m_shape.setPosition(position);
    m_currentFacingDirection = false;
    m_lastFacingDirection = false;
}

void Enemy::update(const sf::Time dt, const TerrainCollision& terrain)
{
    m_velocity.y += Constants::Physics::Gravity * dt.asSeconds();
    const sf::Vector2f displacement = movementLogic(dt, terrain.hasGroundSupport(m_shape.getGlobalBounds()));
    const TerrainMove moved = resolveTerrainMovement(m_shape.getGlobalBounds(), terrain, displacement);
    if (terrain.hasGroundSupport(m_shape.getGlobalBounds()))
        m_onGround = true;
    else
        m_onGround = false;
    if (moved.contacts.floor && m_velocity.y > 0.f)
        m_velocity.y = 0.f;
    if (moved.contacts.ceiling && m_velocity.y < 0.f)
        m_velocity.y = 0.f;
    if (moved.contacts.leftWall && m_velocity.x < 0.f)
        m_velocity.x = 0.f;
    if (moved.contacts.rightWall && m_velocity.x > 0.f)
        m_velocity.x = 0.f;

    animationLogic(dt);

    // Update for next frame
    m_lastFacingDirection = m_currentFacingDirection;
    contactLogic(moved.contacts);
}

void Enemy::attackingLogic()
{
    //TODO
}

// --- Auxiliary funcs

void Enemy::setColor(const sf::Color colour)
{
    m_shape.setFillColor(colour);
}

// std::ostream& operator<<(std::ostream& os, const Enemy& e) {
//     os << "Enemy{";
//     os << static_cast<const Entity&>(e);
//     os << "}";
//     return os;
// }
