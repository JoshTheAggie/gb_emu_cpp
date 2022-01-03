#include <iostream>
#include <chrono>
#include <SDL2/SDL.h>
#include "gb.h"
#include "platform.h"

gb cpu;

int main(int argc, char **argv) { //scale as an integer, cycle period in ms, ROM name
    if(argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <Scale> <Cycle period (ms)> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    int videoScale = std::stoi(argv[1]);
    int cycleDelay = std::stoi(argv[2]);
    char const *romFileName = argv[3];

    Platform platform("Joshua Mashburn's Game Boy Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

    gb cpu{};
    cpu.LoadROM(romFileName);

    int videoPitch = sizeof(cpu.video[0]) * VIDEO_WIDTH;

    auto lastCycleTime = std::chrono::high_resolution_clock::now();
    bool quit = false;

    while(!quit)
    {
        quit = platform.ProcessInput(cpu.directions, cpu.buttons);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::nanoseconds::period>(currentTime-lastCycleTime).count();

        if (dt > cycleDelay)
        {
            lastCycleTime = currentTime;
            cpu.Joypad();
            cpu.Cycle();
            platform.Update(cpu.video, videoPitch);
        }
    }
    return 0;
}
