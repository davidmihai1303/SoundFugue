//
// Created by david on 11/1/2025.
//
#include <algorithm>
#include "game/World.hpp"
#include "entities/enemies/Spider.hpp"

World::World(sf::RenderWindow &window) : m_window(window),
                                         m_terrain(std::vector<sf::FloatRect>{
                                             {{-500.f, 550.f}, {6000.f, 50.f}},
                                             {{-400.f, 500.f},{50.f, 50.f}},
                                         }),
                                         m_playerStandingTexture("../resources/sprites/aeris_standing_animation_spritesheet.png"),
                                         m_playerWalkingTexture("../resources/sprites/aeris_walking_animation_spritesheet.png"),
                                         m_playerAttackingTexture("../resources/sprites/aeris_attacking_animation_spritesheet.png"),
                                         m_spiderWalkingTexture("../resources/sprites/spider_walking_animation_spritesheet.png")
{
    if (m_map.load("../resources/maps/untitled.tmx")) {
        const auto& layers = m_map.getLayers();

        for (std::size_t i = 0; i < layers.size(); ++i) {
            // 2. Only create a MapLayer if the Tiled layer is a "Tile" type
            if (layers[i]->getType() == tmx::Layer::Type::Tile) {
                // This triggers the internal generateGeometry() once
                m_mapLayers.push_back(std::make_unique<MapLayer>(m_map, i));
            }
        }
    }
    // Create player
    m_entities.push_back(std::make_unique<Player>(m_playerStandingTexture.get(), m_playerWalkingTexture.get(), m_playerAttackingTexture.get()));
    m_player = dynamic_cast<Player *>(m_entities.back().get()); // We keep a raw pointer to access Player faster

    // Create an enemy
    auto enemy = std::make_unique<Spider>(sf::Vector2f{400.f, 500.f}, sf::Vector2f{Constants::Spider::HitboxWidth, Constants::Spider::HitboxHeight}, m_spiderWalkingTexture.get());
    m_entities.push_back(std::move(enemy));

    // Being unique pointers, enemy and player will be automatically deleted after the constructor is finished
    // Their ownership is moved to the entities list

    m_ground.setSize({6000.f, 50.f});
    m_ground.setFillColor(sf::Color(100, 60, 30));
    m_ground.setPosition({-500.f, 550.f});

    // Code=2
    addNotes();
}

void World::update(const sf::Time dt, const InputState &inputState) {
    // Update map animations
    for (const auto& layer : m_mapLayers) {
        layer->update(dt);
    }

    if (m_player) {
        (*m_player).setInputState(inputState);
    }

    for (const auto &e: m_entities)
        (*e).update(dt, m_terrain);
    // It uses its own specific update func (the one with override)

    handleCollisions();
}

void World::handleCollisions() {
    sf::FloatRect playerBounds = (*m_player).getBounds();
    collision_player_enemies(playerBounds);

    playerBounds = (*m_player).getBounds();
    collision_player_notes(playerBounds);
}

void World::collision_player_enemies(const sf::FloatRect &playerBounds) {
    if (!m_player)
        return;

    for (auto &e: m_entities) {
        if (e.get() == m_player)
            continue;

        const sf::FloatRect enemyBounds = (*e).getBounds();
        if (playerBounds.findIntersection(enemyBounds)) {
            // simple reaction: reset player position
            (*m_player).setPosition({100.f, 100.f});
            (*m_player).setVelocity({(*m_player).getVelocity().x, 0.f});
            return;
        }
    }

    // Hit logic implementation
    if ((*m_player).getAttackingState() == true) {
        for (auto &e: m_entities) {
            if (e.get() == m_player)
                continue;

            const sf::FloatRect enemyBounds = (*e).getBounds();
            if ((*m_player).getAttackingBounds().findIntersection(enemyBounds)) {
                // simple reaction: add a filter to the enemy
                //TODO manage enemy default color so it doesn't stay purple forever (using a Clock)
                if (auto *enemy = dynamic_cast<Enemy *>(e.get())) {
                    (*enemy).setColor(sf::Color(255, 120, 255, 255));
                }
            }
        }
    }
}

void World::collision_player_notes(const sf::FloatRect &playerBounds) {
    if (!m_player)
        return;

    // Remove the notes which the player intersects
    std::erase_if(
        m_notes,
        [&](const std::unique_ptr<Note> &n) {
            const sf::FloatRect noteBounds = (*n).getBounds();
            return playerBounds.findIntersection(noteBounds).has_value();
        });
}

// Helper func for the camera logic in the Game class
sf::Vector2f World::getPlayerPosition() const {
    if (!m_player)
        return sf::Vector2f{0.f, 0.f};
    return (*m_player).getPosition();
}

void World::draw() const {
    // 1. Draw Background Map Layers first
    for (const auto& layer : m_mapLayers) {
        m_window.draw(*layer);
    }

    m_window.draw(m_ground);

    // Draw all entities
    for (const auto &e: m_entities)
        (*e).draw(m_window);

    // Draw all notes
    // Code=2
    for (const auto &n: m_notes)
        (*n).draw(m_window);
}

// Code=2
void World::addNotes() {
    for (auto &pos: positions)
        m_notes.push_back(std::make_unique<Note>(pos));
}
//
// std::ostream &operator<<(std::ostream &os, const World &w) {
//     os << "World:\n";
//     os << " Player -> " << *w.m_player << "\n";
//     for (const auto &e: w.m_entities)
//         if (e.get() != w.m_player)
//             os << " Enemy -> " << *e << "\n";
//     return os;
// }
