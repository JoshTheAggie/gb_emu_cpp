#include <iostream>
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
    std::printf("Loaded ROM image ");
    std::printf(romFileName);
    std::printf("\n");
    bool quit = false;

    const int MAXCYCLES = 69905;
    int32_t cycles_since_last_screen = 0;

    while(!quit)
    {
        while ((cycles_since_last_screen < MAXCYCLES) && !quit) {
            quit = platform.ProcessInput(sharedMemory.directions, sharedMemory.buttons);
            cpu.update_joypad_reg();
            cpu.CPU_execute_op();
            cycles_since_last_screen += cpu.cyclecount;
            cpu.update_timers(cpu.cyclecount);
            cpu.gpu->update_graphics(cpu.cyclecount);
        }
        cycles_since_last_screen = 0;
        platform.Update(cpu.video);
    }
    return 0;
}
