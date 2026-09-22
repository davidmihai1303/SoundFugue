#ifndef SFML_ORTHO_FLAT_HPP_
#define SFML_ORTHO_FLAT_HPP_

#include <SFML/Graphics.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <memory>
#include <vector>
#include <array>
#include <map>
#include <iostream>

class MapLayer final : public sf::Drawable, public sf::Transformable {
public:
    MapLayer(const tmx::Map& map, const std::size_t layerIdx) {
        const std::vector<tmx::Layer::Ptr>& layers = map.getLayers();
        if (map.getOrientation() != tmx::Orientation::Orthogonal) {
            std::cout << "Map is not orthogonal!" << std::endl;
            return;
        }
        if (layers[layerIdx]->getType() != tmx::Layer::Type::Tile) {
            std::cout << "Not a Tile Layer!" << std::endl;
            return;
        }

        // Number of tiles on the grid
        m_mapSize = sf::Vector2u(map.getTileCount().x, map.getTileCount().y);

        // The size of one tile in pixels
        m_tileSize = sf::Vector2u(map.getTileSize().x, map.getTileSize().y);

        m_animTiles = map.getAnimatedTiles();

        const tmx::TileLayer& layer = layers[layerIdx]->getLayerAs<tmx::TileLayer>();

        // 1. Copy the raw tile IDs into our own memory so we can change them later
        m_tileIDs = layer.getTiles();

        // 2. Load the textures and set up the vertex arrays
        setupTilesets(map.getTilesets());

        // 3. Build the geometry for the very first time
        generateGeometry();
    }

    // Change a tile dynamically during gameplay
    void setTile(std::uint32_t x, std::uint32_t y, tmx::TileLayer::Tile newTile) {
        if (x >= m_mapSize.x || y >= m_mapSize.y)
            return;

        const std::size_t idx = y * m_mapSize.x + x;
        m_tileIDs[idx] = newTile;

        // Because we don't have chunks, changing ONE tile means
        // we have to rebuild the entire map's geometry!
        generateGeometry();
    }

    // Update animations
    void update(sf::Time elapsed) {
        bool geometryNeedsUpdate = false;

        for (auto& anim : m_activeAnimations) {
            anim.currentTime += elapsed;
            std::int32_t animTime = 0;
            auto frame = anim.animTile.animation.frames.begin();

            while (animTime < anim.currentTime.asMilliseconds()) {
                if (frame == anim.animTile.animation.frames.end()) {
                    frame = anim.animTile.animation.frames.begin();
                    anim.currentTime -= sf::milliseconds(animTime);
                    animTime = 0;
                }

                // If the frame has changed, update the raw ID array
                if (m_tileIDs[anim.mapIndex].ID != frame->tileID) {
                    m_tileIDs[anim.mapIndex].ID = frame->tileID;
                    geometryNeedsUpdate = true;
                }

                animTime += frame->duration;
                frame++;
            }
        }

        // Only rebuild the geometry if an animation actually ticked over to a new frame
        if (geometryNeedsUpdate) {
            generateGeometry();
        }
    }

private:
    sf::Vector2u m_mapSize;
    sf::Vector2u m_tileSize;
    std::vector<tmx::TileLayer::Tile> m_tileIDs;
    std::map<std::uint32_t, tmx::Tileset::Tile> m_animTiles;

    // --- SUB-LAYER STRUCT ---
    // Instead of chunks, we just have one big array of vertices for each texture.
    struct TextureLayer final : public sf::Drawable {
        std::unique_ptr<sf::Texture> texture;
        std::vector<sf::Vertex> vertices;
        std::uint32_t firstGID = 0, lastGID = 0;
        sf::Vector2u texTileSize;
        sf::Vector2u texTileCount;

        void draw(sf::RenderTarget& rt, sf::RenderStates states) const override {
            states.texture = texture.get();
            rt.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Triangles, states);
        }
    };

    std::vector<std::unique_ptr<TextureLayer>> m_textureLayers;

    struct AnimationState {
        std::size_t mapIndex;
        sf::Time currentTime;
        tmx::Tileset::Tile animTile;
    };

    std::vector<AnimationState> m_activeAnimations;

    // --- SETUP TEXTURES ---
    void setupTilesets(const std::vector<tmx::Tileset>& tilesets) {
        for (const tmx::Tileset& ts : tilesets) {
            if (ts.getImagePath().empty())
                continue;

            std::unique_ptr<TextureLayer> layer = std::make_unique<TextureLayer>();

            layer->texture = std::make_unique<sf::Texture>();

            // Note: SFML 3 loadFromFile returns an std::optional in some builds,
            // or boolean. Adjust this check depending on your exact RC version.
            if (!layer->texture->loadFromFile(ts.getImagePath())) {
                std::cout << "Failed to load texture: " << ts.getImagePath() << std::endl;
                continue;
            }

            layer->firstGID = ts.getFirstGID();
            layer->lastGID = ts.getLastGID();
            layer->texTileSize.x = ts.getTileSize().x;
            layer->texTileSize.y = ts.getTileSize().y;

            const sf::Vector2u texSize = layer->texture->getSize();

            layer->texTileCount.x = texSize.x / layer->texTileSize.x;
            layer->texTileCount.y = texSize.y / layer->texTileSize.y;

            m_textureLayers.push_back(std::move(layer));
        }
    }

    // --- BUILD THE ACTUAL TRIANGLES ---
    void generateGeometry() {
        m_activeAnimations.clear();

        // 1. Clear the old geometry
        for (const std::unique_ptr<TextureLayer>& layer : m_textureLayers)
            layer->vertices.clear();

        // 2. Loop through every single tile on the map
        for (std::uint32_t y = 0; y < m_mapSize.y; ++y) {
            for (std::uint32_t x = 0; x < m_mapSize.x; ++x) {
                const std::size_t idx = y * m_mapSize.x + x;
                std::uint32_t tileID = m_tileIDs[idx].ID;

                if (tileID == 0)
                    continue; // Skip empty tiles

                // 3. Find which texture this tile belongs to
                for (const std::unique_ptr<TextureLayer>& layer : m_textureLayers) {
                    if (tileID >= layer->firstGID && tileID <= layer->lastGID) {
                        // Check if it's an animated tile
                        if (m_animTiles.find(tileID) != m_animTiles.end()) {
                            m_activeAnimations.push_back({idx, sf::Time::Zero, m_animTiles[tileID]});
                        }

                        // Calculate physical position on the screen
                        const sf::Vector2f pos(x * m_tileSize.x, y * m_tileSize.y);

                        // Calculate where the graphic is on the spritesheet
                        const std::uint32_t localID = tileID - layer->firstGID;

                        // Cast these to floats here to keep the Vertex code clean and prevent narrowing warnings
                        const float texX = static_cast<float>((localID % layer->texTileCount.x) * layer->texTileSize.x);
                        const float texY = static_cast<float>((localID / layer->texTileCount.x) * layer->texTileSize.y);
                        const float texW = static_cast<float>(layer->texTileSize.x);
                        const float texH = static_cast<float>(layer->texTileSize.y);

                        // Create the 6 vertices (2 triangles) using SFML 3 brace initialization {}
                        sf::Vertex v[6];
                        v[0] = sf::Vertex{pos, sf::Color::White, sf::Vector2f(texX, texY)};

                        v[1] = sf::Vertex{pos + sf::Vector2f(texW, 0.f), sf::Color::White,
                                          sf::Vector2f(texX + texW, texY)};

                        v[2] = sf::Vertex{pos + sf::Vector2f(texW, texH), sf::Color::White,
                                          sf::Vector2f(texX + texW, texY + texH)};

                        v[3] = v[0];
                        v[4] = v[2];

                        v[5] = sf::Vertex{pos + sf::Vector2f(0.f, texH), sf::Color::White,
                                          sf::Vector2f(texX, texY + texH)};

                        // Apply standard Tiled flip math
                        applyFlips(m_tileIDs[idx].flipFlags, v);

                        // Push them into the layer
                        for (const sf::Vertex& i : v) {
                            layer->vertices.push_back(i);
                        }
                        break; // Stop searching layers once we found a match
                    }
                }
            }
        }
    }

    static void applyFlips(const std::uint8_t flags, sf::Vertex v[6]) {
        if (flags == 0)
            return;

        // Horizonal Flip
        if (flags & tmx::TileLayer::FlipFlag::Horizontal) {
            std::swap(v[0].texCoords.x, v[1].texCoords.x);
            std::swap(v[3].texCoords.x, v[1].texCoords.x); // v[3] is mapped to v[0]
            std::swap(v[5].texCoords.x, v[2].texCoords.x);
            std::swap(v[4].texCoords.x, v[2].texCoords.x); // v[4] is mapped to v[2]
        }
        // Vertical Flip
        if (flags & tmx::TileLayer::FlipFlag::Vertical) {
            std::swap(v[0].texCoords.y, v[5].texCoords.y);
            std::swap(v[3].texCoords.y, v[5].texCoords.y);
            std::swap(v[1].texCoords.y, v[2].texCoords.y);
            std::swap(v[4].texCoords.y, v[2].texCoords.y);
        }
        // Diagonal Flip (Swap X and Y)
        if (flags & tmx::TileLayer::FlipFlag::Diagonal) {
            std::swap(v[1].texCoords, v[5].texCoords);
            // Re-sync the overlapping triangle points
            v[3].texCoords = v[0].texCoords;
            v[4].texCoords = v[2].texCoords;
        }
    }

    void draw(sf::RenderTarget& rt, sf::RenderStates states) const override {
        states.transform *= getTransform();
        for (const auto& layer : m_textureLayers) {
            rt.draw(*layer, states);
        }
    }
};

#endif
