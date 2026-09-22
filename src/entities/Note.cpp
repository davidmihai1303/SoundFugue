//
// Created by david on 11/6/2025.
//

#include "entities/Note.hpp"

Note::Note(const sf::Vector2f& position) {
    m_shape.setSize(sf::Vector2f(20.f, 20.f));
    m_shape.setFillColor(sf::Color::Yellow);
    m_shape.setPosition(position);
}

// Constructor de copiere
Note::Note(const Note& other) : m_shape(other.m_shape) {
    // sf::RectangleShape are operator= deja implementat corect
}

// Operator= de copiere
Note& Note::operator=(const Note& other) {
    if (this != &other) {
        m_shape = other.m_shape;
    }
    return *this;
}

// Destructor
Note::~Note() {}

void Note::draw(sf::RenderTarget& target) const {
    target.draw(m_shape);
}

sf::FloatRect Note::getBounds() const {
    return m_shape.getGlobalBounds();
}

// std::ostream& operator<<(std::ostream& os, const Note& n) {
// const sf::FloatRect bounds = n.getBounds();
// os << "Note(size: " << bounds.size.x << "x" << bounds.size.y
//    << ", pos: " << bounds.position.x << ", " << bounds.position.y << ")";
//     return os;
// }