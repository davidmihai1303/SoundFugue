//
// Created by david on 11/2/2025.
//

#include "graphics/TextureHolder.hpp"
#include <iostream>

TextureHolder::TextureHolder(const std::string& path) {
    m_texture = new sf::Texture();
    if (!m_texture->loadFromFile(path)) {
        std::cerr << "Error: couldn't load texture at " << path << "\n";
    }
}

TextureHolder::~TextureHolder() {
    delete m_texture;
}

sf::Texture& TextureHolder::get() const {
    if (!m_texture) {
        throw std::runtime_error("Texture not loaded!");
    }
    return *m_texture;
}
