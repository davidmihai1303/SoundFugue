//
// Created by david on 1/5/2026.
//

#ifndef SOUNDFUGUE_CONSTANTS_HPP
#define SOUNDFUGUE_CONSTANTS_HPP

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

namespace Constants {
inline constexpr unsigned int FrameRateLimit = 540;
namespace Window {
inline constexpr float Width = 1920.f;
inline constexpr float Height = 1080.f;
inline constexpr float ViewCenterX = Width / 2.f;
inline constexpr float ViewCenterY = Height / 2.f;
} // namespace Window

namespace Physics {
inline constexpr float Gravity = 2000.f;
inline constexpr float collisionTimeTolerance = 0.00001f;
inline constexpr float groundSupportTolerance = 0.001f;
} // namespace Physics

namespace Player {
inline constexpr float MoveSpeed = 350.f;
inline constexpr float JumpStrength = 550.f;

// Placement
inline constexpr sf::Vector2f RespawnPosition{100.f, 100.f};

// Dash / Sprint
inline constexpr float DashSpeed = 300.f; //TODO experiment with other values
inline constexpr float DashFriction = 0.985f;
inline constexpr float SprintMultiplier = 1.71f; //TODO experiment with other values

// Hitbox
inline constexpr float HitboxWidth = 50.f;
inline constexpr float HitboxHeight = 100.f;
inline sf::Color HitboxColor = sf::Color::Transparent;
inline constexpr float AttackingHitboxWidth = 40.f;
inline constexpr float AttackingHitboxHeight = 65.f;
inline sf::Color AttackingHitboxColor = sf::Color::Transparent;

// Combat
inline constexpr float AttackCooldown = 0.36f;
inline constexpr float ActiveAttackDuration = 0.56f; //TODO experiment with other values
inline constexpr float RunningAttackBoost = 1.25f;

namespace Animation {
inline constexpr float StandingAnimDuration = 0.2f;
inline constexpr float WalkingAnimDuration = 0.1f;
inline constexpr float AttackingAnimDuration = 0.07f; //TODO experiment with other values
inline constexpr unsigned int StandingFrameCount = 8;
inline constexpr unsigned int WalkingFrameCount = 8;
inline constexpr unsigned int AttackingFrameCount = 8;
} // namespace Animation
} // namespace Player

namespace Spider {
// Hitbox
inline constexpr float HitboxWidth = 50.f;
inline constexpr float HitboxHeight = 50.f;
inline constexpr float Speed = 200.f;
namespace Animation {
inline constexpr float WalkingAnimDuration = 0.1f;
inline constexpr unsigned int WalkingFrameCount = 4;
} // namespace Animation
} // namespace Spider

namespace Camera {
inline constexpr float SmoothingBase = 0.04f; // 4% per frame at 60fps
}
} // namespace Constants

#endif //SOUNDFUGUE_CONSTANTS_HPP
