//
// Created by david on 10/31/2025.
//

#ifndef SOUNDFUGUE_PLAYER_HPP
#define SOUNDFUGUE_PLAYER_HPP

#include <SFML/Graphics.hpp>
#include "entities/Entity.hpp"
#include "game/InputState.hpp"
#include "game/Constants.hpp"

class Player final : public Entity {
public:
    friend std::ostream& operator<<(std::ostream& os, const Player& p);

    explicit Player(const sf::Texture& standingTexture, const sf::Texture& walkingTexture, const sf::Texture& attackingTexture);

    void update(sf::Time dt, const TerrainCollision& terrain) override;

    void draw(sf::RenderTarget &target) const override;

    sf::Vector2f movementLogic(sf::Time dt) override;

    void attackingLogic() override;

    //TODO ??
    sf::FloatRect getPlayerDimensions() const;

    void setOnGround(bool value);

    void attack();

    void setInputState(const InputState &inputState);

    void setGroundBounds(const sf::FloatRect &groundBounds);

    void resetDash();

    void setPosition(const sf::Vector2f &position) override;

    // Code=1
    sf::FloatRect getAttackingBounds() const;


private:
    // --- Sprites
    sf::Sprite m_standingSprite;
    sf::Sprite m_walkingSprite;
    sf::Sprite m_attackingSprite;
    // --------

    InputState m_inputState;
    sf::FloatRect m_groundBounds;
    sf::Vector2f m_movement;
    bool m_onGround;
    sf::Time m_cooldownAttackTime = sf::seconds(Constants::Player::AttackCooldown);
    sf::Time m_activeAttackTime = sf::seconds(Constants::Player::ActiveAttackDuration);

    // Code=1
    sf::RectangleShape attackingShape;

    // Attacking in-air helpers
    bool m_isFrozen;

    bool m_shiftFromGround;    // If you pressed Shift while you were on ground
    // We need this variable to tell whether the jump was while moving or while running, regardless of pressing Shift in-air

    bool m_dashAttack;
    bool m_hasDashed;

    // --- Animation Variables ---
    unsigned int m_animationToDraw;     // What animation to draw on screen: 0 - standing, 1 - walking, 2 - attack

    int m_standing_currentFrame;       // Current frame index (starting from 0)
    float m_standing_animDuration;     // How long one frame stays on screen (in seconds)
    float m_standing_elapsedTime;      // Timer
    sf::Vector2u m_standing_frameSize; // Width and Height of a single frame
    int m_standing_numFrames;          // Total frames in spritesheet

    int m_walking_currentFrame;
    float m_walking_animDuration;
    float m_walking_elapsedTime;
    sf::Vector2u m_walking_frameSize;
    int m_walking_numFrames;

    int m_attacking_currentFrame;
    float m_attacking_animDuration;
    float m_attacking_elapsedTime;
    sf::Vector2u m_attacking_frameSize;
    int m_attacking_numFrames;
    // ---------------------------

    void animationLogic(sf::Time dt);
    void standingAnimation(sf::Time dt);
    void walkingAnimation(sf::Time dt);
    void attackingAnimation(sf::Time dt);
};


#endif //SOUNDFUGUE_PLAYER_HPP
