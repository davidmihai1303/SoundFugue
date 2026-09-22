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

    const sf::Vector2u walkingTextureSize = walkingTexture.getSize();
    m_walking_frameSize = sf::Vector2u(walkingTextureSize.x / m_walking_numFrames, walkingTextureSize.y);
    m_walkingSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2<int>(m_walking_frameSize)));
    m_walkingSprite.setOrigin({
        static_cast<float>(m_walking_frameSize.x) / 2, static_cast<float>(m_walking_frameSize.y)
    }); //origin in the middle bottom
    m_walkingSprite.setScale({1.3f, 1.3f});
    if (!m_currentFacingDirection)
        m_walkingSprite.setScale({-1.f * m_walkingSprite.getScale().x, m_walkingSprite.getScale().y});
}

sf::Vector2f Spider::movementLogic(const sf::Time dt, bool hasGroundSupport)
{
    m_movement = {0.f, 0.f};
    // As long as there is no horizontal collision, the spider just moves , otherwise it changes direction
    if (m_currentFacingDirection)
    {
        m_movement.x += Constants::Spider::Speed;
    }else
        m_movement.x -= Constants::Spider::Speed;

    return (m_velocity + m_movement) * dt.asSeconds();
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

void Spider::contactLogic(const TerrainContacts& contacts)
{
    if (contacts.leftWall || contacts.rightWall)
    {
        m_currentFacingDirection = !m_currentFacingDirection;
        // Flip the sprite
        m_walkingSprite.setScale({-1.f * m_walkingSprite.getScale().x, m_walkingSprite.getScale().y});
    }
}

void Spider::draw(sf::RenderTarget& target) const
{
    target.draw(m_shape);
    target.draw(m_walkingSprite);
}
