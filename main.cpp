#include <iostream>
#include <chrono>
#include <SDL2/SDL.h>
#include "gb.h"
#include "platform.h"
#include "memory.h"

memory sharedMemory;

int main(int argc, char **argv) { //scale as an integer, cycle period in ms, ROM name
    if(argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <Scale> <CPU_execute_op period (ms)> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    int videoScale = std::stoi(argv[1]);
    int cycleDelay = std::stoi(argv[2]);
    char const *romFileName = argv[3];

    Platform platform("Joshua Mashburn's GB Emu", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT, videoScale);

    gb cpu{};
    sharedMemory.LoadROM(romFileName);

    auto lastCycleTime = std::chrono::high_resolution_clock::now();
    bool quit = false;

    const int MAXCYCLES = 69905;
    uint32_t oldCycles = 0, deltaCycles = 0;

    while(!quit)
    {
        //quit = platform.ProcessInput(cpu.directions, cpu.buttons);
        //auto currentTime = std::chrono::high_resolution_clock::now();
        //float dt = std::chrono::duration<float, std::chrono::nanoseconds::period>(currentTime-lastCycleTime).count();
        //if (dt > cycleDelay){
            //lastCycleTime = currentTime;
        while ((cpu.cycles_since_last_screen < MAXCYCLES) && !quit) {
            quit = platform.ProcessInput(sharedMemory.directions, sharedMemory.buttons);
            cpu.update_joypad_reg();
            cpu.CPU_execute_op();
            deltaCycles = cpu.cycles_since_last_screen - oldCycles;
            cpu.update_timers(deltaCycles);
            cpu.gpu->update_graphics(deltaCycles);
            oldCycles = cpu.cycles_since_last_screen;
        }
        cpu.cycles_since_last_screen = 0;
        platform.Update(cpu.video);

    }
    return 0;
}
