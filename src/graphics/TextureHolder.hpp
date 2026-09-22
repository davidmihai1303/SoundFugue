//
// Created by david on 11/2/2025.
//

#ifndef SOUNDFUGUE_TEXTUREHOLDER_HPP
#define SOUNDFUGUE_TEXTUREHOLDER_HPP
#include <SFML/Graphics.hpp>
#include <string>

class TextureHolder {
public:
    explicit TextureHolder(const std::string& path);

    ~TextureHolder();

    // Forbid copying textures. Only copy by reference
    TextureHolder(const TextureHolder&) = delete;

    TextureHolder& operator=(const TextureHolder&) = delete;

    // We use reference to avoid making a copy of the large texture object
    [[nodiscard]] sf::Texture& get() const;

private:
    sf::Texture* m_texture;
};

#endif //SOUNDFUGUE_TEXTUREHOLDER_HPP
