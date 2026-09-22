//
// Created by david on 11/6/2025.
//

#ifndef SOUNDFUGUE_NOTE_HPP
#define SOUNDFUGUE_NOTE_HPP

#include <SFML/Graphics.hpp>

class Note {
public:
    friend std::ostream& operator<<(std::ostream& os, const Note& n);

    explicit Note(const sf::Vector2f& position);

    // Constructor de copiere
    Note(const Note& other);

    // Operator= de copiere
    Note& operator=(const Note& other);

    // Destructor
    ~Note();

    void draw(sf::RenderTarget& target) const;

    sf::FloatRect getBounds() const;

private:
    sf::RectangleShape m_shape;
};

#endif //SOUNDFUGUE_NOTE_HPP
