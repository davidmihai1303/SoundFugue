//
// Created by david on 10/31/2025.
//

#include "entities/Player.hpp"
#include <cmath>

Player::Player(const sf::Texture& standingTexture, const sf::Texture& walkingTexture,
               const sf::Texture& attackingTexture)
    : // --- Sprites
      m_standingSprite(standingTexture), m_walkingSprite(walkingTexture), m_attackingSprite(attackingTexture),

      m_isFrozen(false), m_shiftFromGround(false), m_dashAttack(false), m_hasDashed(false),

      // --- Animations
      m_animationToDraw(0),

      m_standing_currentFrame(0), m_standing_animDuration(Constants::Player::Animation::StandingAnimDuration), m_standing_elapsedTime(0.f),
      m_standing_numFrames(Constants::Player::Animation::StandingFrameCount),

      m_walking_currentFrame(0), m_walking_animDuration(Constants::Player::Animation::WalkingAnimDuration), m_walking_elapsedTime(0.f),
      m_walking_numFrames(Constants::Player::Animation::WalkingFrameCount),

      m_attacking_currentFrame(0), m_attacking_animDuration(Constants::Player::Animation::AttackingAnimDuration), m_attacking_elapsedTime(0.f),
      m_attacking_numFrames(Constants::Player::Animation::AttackingFrameCount) {
    // Create the player's hitbox
    m_shape.setSize(sf::Vector2f({Constants::Player::HitboxWidth, Constants::Player::HitboxHeight}));
    m_shape.setFillColor(Constants::Player::HitboxColor);
    m_shape.setPosition({0.f, 250.f});
    m_currentFacingDirection = true; // Different from all the other entities (enemies)
    m_lastFacingDirection = true;

    // -------- STANDING ANIMATION --------
    // CALCULATE FRAME SIZE
    // A horizontal strip: Width / 8, Height = Full Height
    const sf::Vector2u standingTextureSize = standingTexture.getSize();
    m_standing_frameSize = sf::Vector2u(standingTextureSize.x / m_standing_numFrames, standingTextureSize.y);
    // Set the first frame
    m_standingSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2<int>(m_standing_frameSize)));
    m_standingSprite.setOrigin({static_cast<float>(m_standing_frameSize.x) / 2 + 5.f, static_cast<float>(m_standing_frameSize.y)}); //origin in the middle bottom with offset to match the body

    //     --------- WALKING ANIMATION --------
    const sf::Vector2u walkingTextureSize = walkingTexture.getSize();
    m_walking_frameSize = sf::Vector2u(walkingTextureSize.x / m_walking_numFrames, walkingTextureSize.y);
    m_walkingSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2<int>(m_walking_frameSize)));
    m_walkingSprite.setOrigin({static_cast<float>(m_walking_frameSize.x) / 2 + 5.f, static_cast<float>(m_walking_frameSize.y)}); //origin in the middle bottom with offset to match the body

    //    --------- ATTACKING ANIMATION --------
    const sf::Vector2u attackingTextureSize = attackingTexture.getSize();
    m_attacking_frameSize = sf::Vector2u(attackingTextureSize.x / m_attacking_numFrames, attackingTextureSize.y);
    m_attackingSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2<int>(m_attacking_frameSize)));
    m_attackingSprite.setOrigin({static_cast<float>(m_standing_frameSize.x) / 2 + 5.f, static_cast<float>(m_attacking_frameSize.y)}); // We keep the same x from the standing/walking animation
}

void Player::update(const sf::Time dt, const TerrainCollision& terrain) {
    // Ground support is deliberately queried on both sides of the movement.
    // Before: movementLogic gates the jump on it, so it has to describe where she stands right
    // now. This is also what catches a respawn teleport, which World::handleCollisions performs
    // after the previous update() has already returned.
    const sf::Vector2f displacement = movementLogic(dt, terrain.hasGroundSupport(m_shape.getGlobalBounds()));
    const TerrainMove moved = resolveTerrainMovement(m_shape.getGlobalBounds(), terrain, displacement);

    // After: attackingLogic() below has to see the ground state she ended this frame in, or
    // attacking on the exact frame she lands or steps off a ledge takes the wrong branch in
    // attack(). The shape already holds the resolved position here, because
    // resolveTerrainMovement applied it before returning.
    // This asks the terrain about her final position rather than reading moved.contacts.floor:
    // a floor contact only means she met a floor at some point during the sweep, which is not the
    // same as standing on one afterwards. See the "land then slide off ledge" case in
    // tests/TerrainCollisionTests.cpp.
    if (terrain.hasGroundSupport(m_shape.getGlobalBounds()))
        m_onGround = true;
    else
        m_onGround = false;
    // Zero out velocity between frames in case of contact.
    // Only the component pointing into the contacted surface is cleared, so motion
    // along the surface and motion away from it are preserved.
    if (moved.contacts.floor && m_velocity.y > 0.f)
        m_velocity.y = 0.f;
    if (moved.contacts.ceiling && m_velocity.y < 0.f)
        m_velocity.y = 0.f;
    if (moved.contacts.leftWall && m_velocity.x < 0.f)
        m_velocity.x = 0.f;
    if (moved.contacts.rightWall && m_velocity.x > 0.f)
        m_velocity.x = 0.f;

    attackingLogic();
    animationLogic(dt);

    m_lastFacingDirection = m_currentFacingDirection; // update for next frame
}

sf::Vector2f Player::movementLogic(const sf::Time dt, bool hasGroundSupport) {
    // Support measured before this frame's movement; see the note in Player::update.
    if (hasGroundSupport) {
        resetDash();
        m_onGround = true;
    } else
        m_onGround = false;

    // Left-Right movement
    m_movement = sf::Vector2f(0.f, 0.f);
    if (!m_isFrozen) {
        // so the player can't move while he's attacking in air
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) {
            m_movement.x -= Constants::Player::MoveSpeed;
            m_currentFacingDirection = false;
            m_isMoving = true;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) {
            m_movement.x += Constants::Player::MoveSpeed;
            m_currentFacingDirection = true;
            m_isMoving = true;
        } else {
            m_shiftFromGround = false;
            m_isMoving = false;
        }
    }

    // Jumping logic and gravity
    // No jump while frozen: a frozen attack must finish first
    if (m_onGround && !m_isFrozen && sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Space)) {
        m_velocity.y = -Constants::Player::JumpStrength;
        m_onGround = false;
        // Stop attacking only when jump begins
        if (m_isAttacking) {
            cancelAttack();
        }
    }
    if (!m_isFrozen)
        m_velocity.y += Constants::Physics::Gravity * dt.asSeconds();
    // Make sure gravity doesn't stack up while frozen in-air

    // Sprint Logic
    if (m_inputState.shiftDown) {
        m_movement.x *= Constants::Player::SprintMultiplier;
        if (m_onGround)
            m_shiftFromGround = true;

        // Also make the animation move faster
        m_walking_animDuration = Constants::Player::Animation::WalkingAnimDuration / Constants::Player::SprintMultiplier;
    } else
        m_walking_animDuration = Constants::Player::Animation::WalkingAnimDuration;

    if (!m_inputState.shiftDown && m_onGround)
        m_shiftFromGround = false;

    // Stop moving if attacking
    if (m_isAttacking && !m_dashAttack) {
        if (m_inputState.firstPressed != 's')
            m_movement.x = 0.f;
        else
            m_movement.x *= Constants::Player::RunningAttackBoost;
    }

    // Smoothen dash attack so it decreases in speed over time
    if (m_dashAttack) {
        // Using this so we don't need exactly 60 frames epr second
        // We want to apply 0.985 roughly 60 times per second
        // Formula: factor ^ (60 * dt)
        const float friction = std::pow(Constants::Player::DashFriction, 60.f * dt.asSeconds());
        m_velocity.x *= friction;
    }

    // Flip the sprites
    if (m_lastFacingDirection != m_currentFacingDirection) {
        m_standingSprite.setScale({-1.f * m_standingSprite.getScale().x, 1.f});
        m_walkingSprite.setScale({-1.f * m_walkingSprite.getScale().x, 1.f});
        m_attackingSprite.setScale({-1.f * m_attackingSprite.getScale().x, 1.f});
    }

    return (m_movement + m_velocity) * dt.asSeconds();
}

void Player::attackingLogic() {
    // Reset the attack cooldown
    if (m_cooldownAttackClock.getElapsedTime() >= m_cooldownAttackTime)
        m_cooldownAttackClock.reset();
    // Reset the active attack
    if (m_activeAttackClock.getElapsedTime() >= m_activeAttackTime) {
        cancelAttack();
    }

    if (m_isAttacking) {
        // Stop attacking if changing facing direction
        if (m_currentFacingDirection != m_lastFacingDirection) {
            cancelAttack();
        }
    }

    // Draw a rectangle representing the hitbox of the Hit-Area
    attackingShape.setSize(sf::Vector2f({Constants::Player::AttackingHitboxWidth, Constants::Player::AttackingHitboxHeight}));
    attackingShape.setFillColor(Constants::Player::AttackingHitboxColor);
    if (m_currentFacingDirection) {
        // facing right
        attackingShape.setOrigin({0.f, 0.f}); // Set origin back to default
        attackingShape.setPosition(m_shape.getPosition() + sf::Vector2f(Constants::Player::HitboxWidth, 0.f));
    } else {
        // facing left
        attackingShape.setOrigin({attackingShape.getLocalBounds().size.x, 0.f}); // Set origin to the top-right corner
        attackingShape.setPosition(m_shape.getPosition());
    }
    if (m_inputState.hasClicked) {
        attack();
    }
}

void Player::attack() {
    // if cooldown has passed
    if (m_cooldownAttackClock.getElapsedTime() == sf::Time::Zero && !m_dashAttack) {
        m_isAttacking = true;
        m_cooldownAttackClock.start();
        // Using restart() to be able to spam-attack
        m_activeAttackClock.restart();
        // Restart the animation
        m_attacking_elapsedTime = 0.f;
        m_attacking_currentFrame = 0;
        m_attackingSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2<int>(m_attacking_frameSize)));

        if (!m_onGround) {
            // We are already in the air when starting the attack
            m_isFrozen = true;
            if (m_shiftFromGround && !m_hasDashed) {
                m_hasDashed = true;
                // Running jump - keep moving forward, no A/D control
                m_dashAttack = true;
                constexpr float dashSpeed = Constants::Player::DashSpeed;
                m_velocity.x = m_currentFacingDirection ? dashSpeed : -dashSpeed;
                m_velocity.y = 0.f;
            } else {
                // Neutral jump -> classic freeze in air
                m_velocity = {0.f, 0.f};
            }
        }
    }
}

void Player::cancelAttack() {
    m_isAttacking = false;
    m_activeAttackClock.reset();
    if (m_isFrozen) {
        m_velocity = {0.f, 0.f};
        m_isFrozen = false;
    }
    m_dashAttack = false;
}

void Player::animationLogic(const sf::Time dt) {
    if (m_isAttacking) {
        m_animationToDraw = 2;  // Tell which sprite to draw
        attackingAnimation(dt); // Animation logic

        // --- Reset the other animations so they start from the beginning next time
        m_standing_elapsedTime = 0.f;
        m_standing_currentFrame = 0;

        m_walking_elapsedTime = 0.f;
        m_walking_currentFrame = 0;
    } else if (!m_isMoving) {
        m_animationToDraw = 0; // Tell which sprite to draw
        standingAnimation(dt); // Animation logic

        // --- Reset the other animations so they start from the beginning next time
        m_walking_elapsedTime = 0.f;
        m_walking_currentFrame = 0;

        m_attacking_elapsedTime = 0.f;
        m_attacking_currentFrame = 0;
    } else {
        m_animationToDraw = 1; // Tell which sprite to draw
        walkingAnimation(dt);  // Animation logic

        // --- Reset the other animations so they start form the beginning next time
        m_standing_elapsedTime = 0.f;
        m_standing_currentFrame = 0;

        m_attacking_elapsedTime = 0.f;
        m_attacking_currentFrame = 0;
    }
}

void Player::standingAnimation(const sf::Time dt) {
    m_standing_elapsedTime += dt.asSeconds();

    // If enough time passed, switch to next frame
    if (m_standing_elapsedTime >= m_standing_animDuration) {
        m_standing_elapsedTime = 0.f; // Reset timer
        m_standing_currentFrame++;    // Next frame

        // Loop back to 0 if we exceed the count (0 -> 1 -> ... -> 7 -> 0)
        if (m_standing_currentFrame >= m_standing_numFrames)
            m_standing_currentFrame = 0;

        // Calculating the coordinate for every frame;
        // Frame x starts at currentFrame * width (80 here)
        const long long leftAux = m_standing_currentFrame * m_standing_frameSize.x;
        int left = static_cast<int>(leftAux); // weird conversion

        int top = 0; // Top is always 0 for a single row spritesheet

        m_standingSprite.setTextureRect(sf::IntRect({left, top}, sf::Vector2<int>(m_standing_frameSize)));
    }

    // --- SYNC POSITION ---
    // Snap sprite to hitbox
    m_standingSprite.setPosition({
        m_shape.getPosition().x + Constants::Player::HitboxWidth / 2,
        m_shape.getPosition().y + Constants::Player::HitboxHeight // bottom center of the hitbox
    });
}

void Player::walkingAnimation(const sf::Time dt) {
    m_walking_elapsedTime += dt.asSeconds();

    // If enough time has passed, switch to next frame
    if (m_walking_elapsedTime >= m_walking_animDuration) {
        m_walking_elapsedTime = 0.f; // reset timer
        m_walking_currentFrame++;    // next frame

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
        m_shape.getPosition().x + Constants::Player::HitboxWidth / 2,
        m_shape.getPosition().y + Constants::Player::HitboxHeight // bottom center of the hitbox
    });
}

void Player::attackingAnimation(const sf::Time dt) {
    m_attacking_elapsedTime += dt.asSeconds();

    // If enough time has passed, switch to next frame
    if (m_attacking_elapsedTime >= m_attacking_animDuration) {
        m_attacking_elapsedTime = 0.f; // reset timer
        m_attacking_currentFrame++;    // next frame

        if (m_attacking_currentFrame >= m_attacking_numFrames)
            m_attacking_currentFrame = 0;

        const long long leftAux = m_attacking_currentFrame * m_attacking_frameSize.x;
        int left = static_cast<int>(leftAux);
        int top = 0;
        m_attackingSprite.setTextureRect(sf::IntRect({left, top}, sf::Vector2<int>(m_attacking_frameSize)));
    }

    // --- SYNC POSITION ---
    // Snap sprite to hitbox
    m_attackingSprite.setPosition({
        m_shape.getPosition().x + Constants::Player::HitboxWidth / 2,
        m_shape.getPosition().y + Constants::Player::HitboxHeight // bottom center of the hitbox
    });
}

void Player::setPosition(const sf::Vector2f& position) {
    m_shape.setPosition(position);
    m_standingSprite.setPosition({m_shape.getPosition().x + Constants::Player::HitboxWidth / 2, m_shape.getPosition().y + Constants::Player::HitboxHeight});
    m_walkingSprite.setPosition({m_shape.getPosition().x + Constants::Player::HitboxWidth / 2, m_shape.getPosition().y + Constants::Player::HitboxHeight});
    m_attackingSprite.setPosition({m_shape.getPosition().x + Constants::Player::HitboxWidth / 2, m_shape.getPosition().y + Constants::Player::HitboxHeight});
}

void Player::respawn(const sf::Vector2f& position) {
    cancelAttack();
    m_velocity = {0.f, 0.f};
    m_movement = {0.f, 0.f};
    m_isMoving = false;
    m_shiftFromGround = false;
    resetDash();
    m_cooldownAttackClock.reset();

    // Face right, as at spawn; the sprites always match the current facing here, so mirror them back if needed
    if (!m_currentFacingDirection) {
        m_standingSprite.setScale({-1.f * m_standingSprite.getScale().x, 1.f});
        m_walkingSprite.setScale({-1.f * m_walkingSprite.getScale().x, 1.f});
        m_attackingSprite.setScale({-1.f * m_attackingSprite.getScale().x, 1.f});
    }
    m_currentFacingDirection = true;
    m_lastFacingDirection = true;

    // Death happens after animationLogic() has already picked this frame's sprite, so pick the standing pose here
    m_animationToDraw = 0;
    m_standing_currentFrame = 0;
    m_standing_elapsedTime = 0.f;
    m_standingSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2<int>(m_standing_frameSize)));

    setPosition(position);
}

void Player::draw(sf::RenderTarget& target) const {
    target.draw(m_shape);

    if (m_isAttacking) {
        target.draw(attackingShape);
    }

    switch (m_animationToDraw) {
    case 0:
        target.draw(m_standingSprite);
        break;

    case 1:
        target.draw(m_walkingSprite);
        break;

    case 2:
        target.draw(m_attackingSprite);
        break;

    default:
        throw std::runtime_error("Invalid animation index");
    }
}

// ----- Auxiliar funcs

void Player::setInputState(const InputState& inputState) {
    m_inputState = inputState;
}

// Code=1
sf::FloatRect Player::getAttackingBounds() const {
    return attackingShape.getGlobalBounds();
}

sf::FloatRect Player::getPlayerDimensions() const {
    return m_shape.getLocalBounds();
}

void Player::resetDash() {
    m_hasDashed = false;
}

// -------

// std::ostream &operator<<(std::ostream &os, const Player &p) {
//     os << "Player{";
//     os << static_cast<const Entity &>(p); // apelează operatorul din Entity
//     os << ", attacking=" << (p.m_isAttacking ? "true" : "false");
//     os << ", sprite= " << p.m_standingSprite.getLocalBounds().size.x << ", " << p.m_standingSprite.getLocalBounds().size
//             .y;
//     os << "}";
//     return os;
// }
