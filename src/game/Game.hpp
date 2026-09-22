//
// Created by david on 10/31/2025.
//

#ifndef SOUNDFUGUE_GAME_HPP
#define SOUNDFUGUE_GAME_HPP
#include <SFML/Graphics.hpp>
#include "game/World.hpp"
#include "game/InputState.hpp"

class Game {
public:
    Game();

    void run();

private:
    void processEvents();

    void updateView(float x, float y);

    void updateCamera(sf::Time dt);

    sf::RenderWindow m_window;
    sf::View m_view;
    World m_world;
    InputState m_inputState;
};

#endif //SOUNDFUGUE_GAME_HPP
