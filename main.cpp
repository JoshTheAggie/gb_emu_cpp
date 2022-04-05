#include <iostream>
#include "gb.h"
#include "platform.h"
#include "memory.h"

memory sharedMemory;
Platform * platform;
gb cpu{};

int cpustep(){
    cpu.CPU_execute_op();
    cpu.update_timers(cpu.cyclecount);
    cpu.gpu->update_graphics(cpu.cyclecount);
    return cpu.cyclecount;
}

bool emulatorframe(){
    bool quit = false;
    const int MAXCYCLES = 69905;
    int cycles_since_last_screen = 0;
    while (cycles_since_last_screen < MAXCYCLES){
        cycles_since_last_screen += cpustep();
    }
    cycles_since_last_screen = 0;
    platform->Update(cpu.video);
    quit = platform->ProcessInput(sharedMemory.directions, sharedMemory.buttons);
    cpu.update_joypad_reg();
    return quit;
}



int main(int argc, char **argv) { //scale as an integer, cycle period in ms, ROM name
    if(argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <Scale> <CPU_execute_op period (ms)> <ROM>\n";
        std::exit(EXIT_FAILURE);
    }

    int videoScale = std::stoi(argv[1]);
    int cycleDelay = std::stoi(argv[2]);
    char const *romFileName = argv[3];

    platform = new Platform("GB Emu", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT, videoScale);

    //gb cpu{};
    sharedMemory.LoadROM(romFileName);
    std::printf("Loaded ROM image ");
    std::printf(romFileName);
    std::printf("\n");
    bool quit = false;

    while(!quit)
    {
        quit = emulatorframe();
    }
    return 0;
}
