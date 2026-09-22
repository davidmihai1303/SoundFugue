//
// Created by david on 10/31/2025.
//

#include "game/Game.hpp"
#include <iostream>
#include <cmath>
#include "game/Constants.hpp"
#include <tmxlite/Map.hpp>

Game::Game()
    : m_window(sf::VideoMode({1920, 1080}), "SoundFugue"),
      m_view({Constants::Window::ViewCenterX, Constants::Window::ViewCenterY},
             {Constants::Window::Width, Constants::Window::Height}),
      m_world(m_window) {}

void Game::run() {
    m_window.setFramerateLimit(Constants::FrameRateLimit);
    m_window.setVerticalSyncEnabled(false);
    m_window.setView(m_view);
    sf::Clock clock;

    sf::Clock fpsTimer;   // Renamed 'aux' to be clearer
    int frameCounter = 0; // NEW: Count frames manually    clock.restart();

    while (m_window.isOpen()) {
        sf::Time dt = clock.restart();

        // Clamp to prevent large jumps (resize, alt-tab, etc.)
        if (dt.asSeconds() > 0.05f)
            dt = sf::seconds(0.05f);

        processEvents();
        m_world.update(dt, m_inputState);
        updateCamera(dt);

        m_window.setView(m_view);

        m_window.clear();
        m_world.draw();
        m_window.display();
        //        std::cout << m_world;

        frameCounter++;

        if (fpsTimer.getElapsedTime().asSeconds() >= 1.0f) {
            m_window.setTitle("SoundFugue | FPS: " + std::to_string(frameCounter));

            // Reset for the next second
            frameCounter = 0;
            fpsTimer.restart();
        }
    }
}

void Game::processEvents() {
    m_inputState.hasClicked = false;
    while (auto event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
        } else if (auto* resize = event->getIf<sf::Event::Resized>()) {
            updateView(static_cast<float>(resize->size.x), static_cast<float>(resize->size.y));
        } else if (auto* button = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (button->button == sf::Mouse::Button::Left) {
                m_inputState.clickDown = true;
                m_inputState.hasClicked = true;
                if (m_inputState.shiftDown && m_inputState.firstPressed == '0')
                    m_inputState.firstPressed = 's';
            }
        } else if (auto* key = event->getIf<sf::Event::KeyPressed>()) {
            if (key->scancode == sf::Keyboard::Scan::LShift) {
                m_inputState.shiftDown = true;
                // if mouse already down, mouse came first
                if (m_inputState.clickDown && m_inputState.firstPressed == '0')
                    m_inputState.firstPressed = 'c';
            }
        } else if (auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            if (keyReleased->scancode == sf::Keyboard::Scan::LShift)
                m_inputState.shiftDown = false;
        } else if (auto* buttonReleased = event->getIf<sf::Event::MouseButtonReleased>())
            if (buttonReleased->button == sf::Mouse::Button::Left)
                m_inputState.clickDown = false;
    }
    if (m_inputState.firstPressed == 's') {
        if (!m_inputState.shiftDown && !m_inputState.clickDown)
            m_inputState.firstPressed = '0';
    } else if (m_inputState.firstPressed == 'c')
        if (!m_inputState.shiftDown || !m_inputState.clickDown)
            m_inputState.firstPressed = '0';
}

// Letter-box type view. Black bars will appear in order to keep the aspect ratio 16:9
void Game::updateView(const float x, const float y) {
    constexpr float baseRatio = Constants::Window::Width / Constants::Window::Height;
    const float newRatio = x / y;

    float sizeX = 1.f;
    float sizeY = 1.f;
    float posX = 0.f;
    float posY = 0.f;

    if (newRatio > baseRatio) {
        // Window is wider -> add vertical bars on the sides
        sizeX = baseRatio / newRatio;
        posX = (1.f - sizeX) / 2.f;
    } else if (newRatio < baseRatio) {
        // Window is taller -> add horizontal bars top/bottom
        sizeY = newRatio / baseRatio;
        posY = (1.f - sizeY) / 2.f;
    }

    // Reset the view to the logical world (1920x1080)
    m_view.setSize({Constants::Window::Width, Constants::Window::Height});
    m_view.setViewport(sf::FloatRect({posX, posY}, {sizeX, sizeY}));
    m_window.setView(m_view);
}

void Game::updateCamera(const sf::Time dt) {
    // Camera follows the player, but with added delay to make it more fluid
    const sf::Vector2f currentCenter = m_view.getCenter();
    const sf::Vector2f target = m_world.getPlayerPosition();

    // We want to smooth by 0.04 (4%) 60 times in a second
    // This means we retain 0.96 (96%) of the distance.
    constexpr float friction = 1.0f - Constants::Camera::SmoothingBase;

    // Calculate how much distance to retain for this specific dt
    const float frameDamping = std::pow(friction, 60.f * dt.asSeconds());

    // Invert it to get the movement amount
    const float smoothing = 1.0f - frameDamping;

    m_view.setCenter(currentCenter + (target - currentCenter) * smoothing);
}
