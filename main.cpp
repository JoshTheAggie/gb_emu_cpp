#include <iostream>
#include <chrono>
#include <thread>
#include "gb.h"
#include "platform.h"
#include "memory.h"

memory sharedMemory;
Platform * platform;
gb cpu{};
std::chrono::steady_clock::time_point currenttime, oldtime;

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
        cycles_since_last_screen += (4 * cpustep());
    }
    cycles_since_last_screen = 0;
    platform->Update(cpu.video);
    quit = platform->ProcessInput(sharedMemory.directions, sharedMemory.buttons);
    cpu.update_joypad_reg();
    //keep frames limited to 60 FPS
    currenttime = std::chrono::steady_clock::now();
    //while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - oldtime).count() < 16200)
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //std::cout << "FPS: " << 1000000/(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - oldtime).count()) << "\n";
    oldtime = std::chrono::steady_clock::now();
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

    currenttime = std::chrono::steady_clock::now();
    oldtime = std::chrono::steady_clock::now();

    while(!quit)
    {
        quit = emulatorframe();
    }
    return 0;
}
