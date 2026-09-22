//
// Created by david on 11/2/2025.
//

#include "entities/Enemy.hpp"

Enemy::Enemy(const sf::Vector2f& position, const sf::Vector2f& size)
{
    m_shape.setSize(size);
    m_shape.setPosition(position);
    m_currentFacingDirection = false;
    m_lastFacingDirection = false;
}

void Enemy::update(const sf::Time dt, const TerrainCollision& terrain)
{
    movementLogic(dt, 1);
    animationLogic(dt);

    m_lastFacingDirection = m_currentFacingDirection; // update for next frame
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
