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

    explicit Player(const sf::Texture& standingTexture, const sf::Texture& walkingTexture,
                    const sf::Texture& attackingTexture);

    void update(sf::Time dt, const TerrainCollision& terrain) override;

    void draw(sf::RenderTarget& target) const override;

    sf::Vector2f movementLogic(sf::Time dt, bool hasGroundSupport) override;

    void attackingLogic() override;

    //TODO ??
    sf::FloatRect getPlayerDimensions() const;

    void attack();

    void setInputState(const InputState& inputState);

    void resetDash();

    void setPosition(const sf::Vector2f& position) override;

    // Death reset: ends the attack, clears motion and input-derived state, restores the dash and the attack cooldown,
    // faces right, then moves the body and sprites. Ground support comes from the next update's pre-move check.
    void respawn(const sf::Vector2f& position);

    // Code=1
    sf::FloatRect getAttackingBounds() const;

private:
    // --- Sprites
    sf::Sprite m_standingSprite;
    sf::Sprite m_walkingSprite;
    sf::Sprite m_attackingSprite;
    // --------

    InputState m_inputState;
    sf::Time m_cooldownAttackTime = sf::seconds(Constants::Player::AttackCooldown);
    sf::Time m_activeAttackTime = sf::seconds(Constants::Player::ActiveAttackDuration);

    // Code=1
    sf::RectangleShape attackingShape;

    // Attacking in-air helpers
    bool m_isFrozen;

    bool m_shiftFromGround; // If you pressed Shift while you were on ground
    // We need this variable to tell whether the jump was while moving or while running, regardless of pressing Shift in-air

    bool m_dashAttack;
    bool m_hasDashed;

    // --- Animation Variables ---
    unsigned int m_animationToDraw; // What animation to draw on screen: 0 - standing, 1 - walking, 2 - attack

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

    // Ends the current attack: stops its timer, releases an airborne freeze with zero velocity and clears the dash.
    // Leaves the cooldown and dash availability (m_hasDashed) unchanged.
    void cancelAttack();

    void animationLogic(sf::Time dt);
    void standingAnimation(sf::Time dt);
    void walkingAnimation(sf::Time dt);
    void attackingAnimation(sf::Time dt);
};

#endif //SOUNDFUGUE_PLAYER_HPP
