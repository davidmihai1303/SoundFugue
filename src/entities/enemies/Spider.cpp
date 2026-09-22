//
// Created by david on 9/22/2026.
//

#include "entities/enemies/Spider.hpp"
#include "game/Constants.hpp"

Spider::Spider(const sf::Vector2f& position, const sf::Vector2f& size,
               const sf::Texture& walkingTexture) : Enemy(position, size),
                                                    m_walkingSprite(walkingTexture),
                                                    m_walking_currentFrame(0),
                                                    m_walking_animDuration(
                                                        Constants::Spider::Animation::WalkingAnimDuration),
                                                    m_walking_elapsedTime(0.f),
                                                    m_walking_numFrames(Constants::Spider::Animation::WalkingFrameCount)
{
    m_shape.setFillColor(sf::Color::Red);
    m_velocity = (sf::Vector2f(100.f, 0.f));

    // Set limits to which it moves
    m_leftLimit = position.x - 100.f;
    m_rightLimit = position.x + 100.f;


    const sf::Vector2u walkingTextureSize = walkingTexture.getSize();
    m_walking_frameSize = sf::Vector2u(walkingTextureSize.x / m_walking_numFrames, walkingTextureSize.y);
    m_walkingSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2<int>(m_walking_frameSize)));
    m_walkingSprite.setOrigin({
        static_cast<float>(m_walking_frameSize.x) / 2, static_cast<float>(m_walking_frameSize.y)
    }); //origin in the middle bottom
    m_walkingSprite.setScale({1.3f, 1.3f});
}

sf::Vector2f Spider::movementLogic(const sf::Time dt, bool hasGroundSupport)
{
    m_shape.move(m_velocity * dt.asSeconds());

    if (const float x = m_shape.getPosition().x; x < m_leftLimit)
    {
        m_shape.setPosition({m_leftLimit, m_shape.getPosition().y});
        m_velocity.x = std::abs(m_velocity.x);
        m_currentFacingDirection = false;
    }
    else if (x + m_shape.getSize().x > m_rightLimit)
    {
        m_shape.setPosition({m_rightLimit - m_shape.getSize().x, m_shape.getPosition().y});
        m_velocity.x = -std::abs(m_velocity.x);
        m_currentFacingDirection = true;
    }

    // Flip the sprites
    if (m_lastFacingDirection != m_currentFacingDirection)
    {
        m_walkingSprite.setScale({-1.f * m_walkingSprite.getScale().x, m_walkingSprite.getScale().y});
    }
    return {0.f, 0.f};
}

void Spider::animationLogic(const sf::Time dt)
{
    walkingAnimation(dt);
}

void Spider::walkingAnimation(const sf::Time dt)
{
    m_walking_elapsedTime += dt.asSeconds();

    // If enough time has passed, switch to next frame
    if (m_walking_elapsedTime >= m_walking_animDuration)
    {
        m_walking_elapsedTime = 0.f; // reset timer
        m_walking_currentFrame++; // next frame

        if (m_walking_currentFrame >= m_walking_numFrames)
            m_walking_currentFrame = 0;

        const long long leftAux = m_walking_currentFrame * m_walking_frameSize.x;
        int left = static_cast<int>(leftAux);
        int top = 0;
        m_walkingSprite.setTextureRect(sf::IntRect({left, top}, sf::Vector2<int>(m_walking_frameSize)));
    }

    // --- SYNC POSITION ---
    // Snap sprite to hitbox
    m_walkingSprite.setPosition({
        m_shape.getPosition().x + Constants::Spider::HitboxWidth / 2,
        m_shape.getPosition().y + Constants::Spider::HitboxHeight // bottom center of the hitbox
    });
}


void Spider::draw(sf::RenderTarget& target) const
{
    target.draw(m_shape);
    target.draw(m_walkingSprite);
}
