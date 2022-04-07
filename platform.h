//
// Created by joshua on 5/23/20.
//

#include <SFML/Graphics.hpp>
#include "memory.h"

#ifndef GBEMUJM_PLATFORM_H
#define GBEMUJM_PLATFORM_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

class Platform {
    sf::RenderWindow* window{};
    sf::Texture* texture{};
    sf::Sprite* sprite{};
public:
    Platform(char const* title, unsigned int windowWidth, unsigned int windowHeight, unsigned int textureWidth, unsigned int textureHeight, int scale)
    {
        window = new sf::RenderWindow(sf::VideoMode(windowWidth, windowHeight), title);
        window->clear();
        texture = new sf::Texture();
        if (!texture->create(textureWidth, textureHeight))
        {
            std::cout << "Error initializing texture\n";
        }
        sprite = new sf::Sprite();
        sprite->setScale(scale, scale);
        sprite->setTexture(*texture);
    }

    ~Platform()
    {
        window->close();
        delete window;
        delete sprite;
        delete texture;
    }

    void Update(void const* buffer)
    {
        texture->update((sf::Uint8*)buffer);
        window->clear();
        window->draw(*sprite);
        window->display();
    }

    bool ProcessInput(uint8_t& dirs, uint8_t& btns)
    {
        bool quit = false;

        sf::Event event;

        while (window->pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed: {
                    quit = true;
                }
                    break;

                case sf::Event::KeyPressed: {
                    switch (event.key.code) {
                        case sf::Keyboard::Key::Escape: {
                            quit = true;
                        }
                            break;

                        case sf::Keyboard::Key::Return: //start
                        {
                            btns &= 0b11110111;
                        }
                            break;

                        case sf::Keyboard::Key::RShift: //select
                        {
                            btns &= 0b11111011;
                        }
                            break;

                        case sf::Keyboard::Key::Period: //a
                        {
                            btns &= 0b11111110;
                        }
                            break;

                        case sf::Keyboard::Key::Comma: //b
                        {
                            btns &= 0b11111101;
                        }
                            break;

                        case sf::Keyboard::Key::W: //up
                        {
                            dirs &= 0b11111011;
                        }
                            break;

                        case sf::Keyboard::Key::A: //left
                        {
                            dirs &= 0b11111101;
                        }
                            break;

                        case sf::Keyboard::Key::S: //down
                        {
                            dirs &= 0b11110111;
                        }
                            break;

                        case sf::Keyboard::Key::D: //right
                        {
                            dirs &= 0b11111110;
                        }
                            break;
                    }
                    break;

                    case sf::Event::KeyReleased: {
                        switch (event.key.code) {
                            case sf::Keyboard::Key::Return: //start
                            {
                                btns |= 0b00001000;
                            }
                                break;

                            case sf::Keyboard::Key::RShift: //select
                            {
                                btns |= 0b00000100;
                            }
                                break;

                            case sf::Keyboard::Key::Period: //a
                            {
                                btns |= 0b00000001;
                            }
                                break;

                            case sf::Keyboard::Key::Comma: //b
                            {
                                btns |= 0b00000010;
                            }
                                break;

                            case sf::Keyboard::Key::W: //up
                            {
                                dirs |= 0b00000100;
                            }
                                break;

                            case sf::Keyboard::Key::A: //left
                            {
                                dirs |= 0b00000010;
                            }
                                break;

                            case sf::Keyboard::Key::S: //down
                            {
                                dirs |= 0b00001000;
                            }
                                break;

                            case sf::Keyboard::Key::D: //right
                            {
                                dirs |= 0b00000001;
                            }
                                break;
                        }
                    }
                    break;
                }
            }
        }
        return quit;
    }

};

#endif //GBEMUJM_PLATFORM_H
