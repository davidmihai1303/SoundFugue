//
// Created by david on 11/2/2025.
//

#ifndef SOUNDFUGUE_ENEMY_HPP
#define SOUNDFUGUE_ENEMY_HPP

#include <SFML/Graphics.hpp>
#include "entities/Entity.hpp"
#include "terrain/TerrainCollision.hpp"

class Enemy : public Entity {
public:
    friend std::ostream& operator<<(std::ostream& os, const Enemy& e);

    Enemy(const sf::Vector2f &position, const sf::Vector2f &size, const sf::Texture &walkingTexture);

    void update(sf::Time dt, const TerrainCollision& terrain) override;

    void draw(sf::RenderTarget &target) const override;

    sf::Vector2f movementLogic(sf::Time dt, bool hasGroundSupport) override;

    void attackingLogic() override;

    void setColor(sf::Color colour);


private:
    float m_leftLimit;
    float m_rightLimit;

    sf::Sprite m_walkingSprite;

    int m_walking_currentFrame;
    float m_walking_animDuration;
    float m_walking_elapsedTime;
    sf::Vector2u m_walking_frameSize;
    int m_walking_numFrames;

    void animationLogic(sf::Time dt);
    void walkingAnimation(sf::Time dt);
};

#endif //SOUNDFUGUE_ENEMY_HPP
