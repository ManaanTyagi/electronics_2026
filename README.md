## Project Structure

This repository contains 2 firmware projects, each for a different operational mode of the Rover. Only **one folder is flashed to Teensy at a time** — either autonomous or manual.

A shared `Assets/` folder holds all hardware-related documents (PCB files, datasheets, etc.) used across both modes.

Embedded_UGVC/
│
├── Assets/                    # Shared hardware resources
│   ├── PCB Files/             # KiCad schematic and layout files
│   └── Datasheet/             # IC, MCU, and component datasheets
│
├── Autonomous Mode/           # Firmware for autonomous navigation tasks
│   ├── platformio.ini
│   ├── src/                   
│   ├── include/               
│   ├── lib/                  
│   └── test/                  
│
├── Manual Mode/               # Firmware for manual / RC-controlled tasks
│   ├── platformio.ini
│   ├── src/                   
│   ├── include/               
│   ├── lib/                   
│   └── test/                  
│
└── README.md