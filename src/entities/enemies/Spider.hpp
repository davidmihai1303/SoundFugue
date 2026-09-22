//
// Created by david on 9/22/2026.
//

#ifndef SOUNDFUGUE_SPIDER_HPP
#define SOUNDFUGUE_SPIDER_HPP

#include <SFML/Graphics.hpp>
#include "entities/Enemy.hpp"

class Spider final : public Enemy {
public:
    Spider(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Texture& walkingTexture);

    void draw(sf::RenderTarget& target) const override;

    sf::Vector2f movementLogic(sf::Time dt, bool hasGroundSupport) override;

protected:
    void animationLogic(sf::Time dt) override;
    void contactLogic(const TerrainContacts& contacts) override;

private:
    sf::Sprite m_walkingSprite;

    int m_walking_currentFrame;
    float m_walking_animDuration;
    float m_walking_elapsedTime;
    sf::Vector2u m_walking_frameSize;
    int m_walking_numFrames;

    void walkingAnimation(sf::Time dt);
};

#endif //SOUNDFUGUE_SPIDER_HPP
