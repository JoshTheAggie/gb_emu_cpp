//
// Created by joshua on 5/23/20.
//

#include <SDL2/SDL.h>

#ifndef GBEMUJM_PLATFORM_H
#define GBEMUJM_PLATFORM_H

class Platform {
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Texture* texture{};
public:
    Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight)
    {
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow(title, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        texture = SDL_CreateTexture(
                renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
    }

    ~Platform()
    {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void Update(void const* buffer, int pitch)
    {
        SDL_UpdateTexture(texture, nullptr, buffer, pitch);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    bool ProcessInput(uint8_t& dirs, uint8_t& btns)
    {
        bool quit = false;

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    quit = true;
                }
                    break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE: {
                            quit = true;
                        }
                            break;

                        case SDLK_RETURN: //start
                        {
                            btns &= 0b11110111;
                        }
                            break;

                        case SDLK_RSHIFT: //select
                        {
                            btns &= 0b11111011;
                        }
                            break;

                        case SDLK_GREATER: //a
                        {
                            btns &= 0b11111110;
                        }
                            break;

                        case SDLK_LESS: //b
                        {
                            btns &= 0b11111101;
                        }
                            break;

                        case SDLK_w: //up
                        {
                            dirs &= 0b11111011;
                        }
                            break;

                        case SDLK_a: //left
                        {
                            dirs &= 0b11111101;
                        }
                            break;

                        case SDLK_s: //down
                        {
                            dirs &= 0b11110111;
                        }
                            break;

                        case SDLK_d: //right
                        {
                            dirs &= 0b11111110;
                        }
                            break;
                    }
                    break;

                    case SDL_KEYUP: {
                        switch (event.key.keysym.sym) {
                            case SDLK_RETURN: //start
                            {
                                btns |= 0b00001000;
                            }
                                break;

                            case SDLK_RSHIFT: //select
                            {
                                btns |= 0b00000100;
                            }
                                break;

                            case SDLK_GREATER: //a
                            {
                                btns |= 0b00000001;
                            }
                                break;

                            case SDLK_LESS: //b
                            {
                                btns |= 0b00000010;
                            }
                                break;

                            case SDLK_w: //up
                            {
                                dirs |= 0b00000100;
                            }
                                break;

                            case SDLK_a: //left
                            {
                                dirs |= 0b00000010;
                            }
                                break;

                            case SDLK_s: //down
                            {
                                dirs |= 0b00001000;
                            }
                                break;

                            case SDLK_d: //right
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
