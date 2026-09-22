//
// Created by david on 11/1/2025.
//

#ifndef SOUNDFUGUE_WORLD_HPP
#define SOUNDFUGUE_WORLD_HPP
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "entities/Player.hpp"
#include "entities/Enemy.hpp"
#include "entities/Entity.hpp"
#include "entities/Note.hpp"
#include "game/InputState.hpp"
#include "graphics/TextureHolder.hpp"
#include <ostream>
#include <tmxlite/Map.hpp>
#include "graphics/MapLayer.hpp"
#include "terrain/TerrainCollision.hpp"

class World {
public:
    friend std::ostream& operator<<(std::ostream& os, const World& w);

    explicit World(sf::RenderWindow& window);

    void update(sf::Time dt, const InputState& inputState);

    void draw() const;

    sf::Vector2f getPlayerPosition() const;

    void playerAttack();

private:
    void handleCollisions();

    void collision_player_enemies(const sf::FloatRect& playerBounds);

    void collision_player_notes(const sf::FloatRect& playerBounds);

    sf::RenderWindow& m_window;

    sf::RectangleShape m_ground;

    // Temporary terrain: populated with the same rectangle as m_ground until
    // Tiled-based loading replaces it (Step 10/11 of the collision guide).
    TerrainCollision m_terrain;

    std::vector<std::unique_ptr<Entity>> m_entities;

    Player* m_player = nullptr;

    std::vector<std::unique_ptr<Note>> m_notes;

    // Code=2
    std::vector<sf::Vector2f> positions = {{100, 300}, {200, 350}, {300, 380}, {400, 320}, {500, 310},
                                           {600, 340}, {700, 360}, {800, 300}, {900, 330}, {1000, 350}};

    // --- Player textures:

    TextureHolder m_playerStandingTexture;
    TextureHolder m_playerWalkingTexture;
    TextureHolder m_playerAttackingTexture;

    // -----------

    // --- Enemy textures:

    TextureHolder m_spiderWalkingTexture;

    // -----------

    // --- Map:

    // In World.hpp private section
    tmx::Map m_map;
    std::vector<std::unique_ptr<MapLayer>> m_mapLayers; // Holds the visual geometry
    // ----------

    void addNotes();
};

#endif //SOUNDFUGUE_WORLD_HPP
