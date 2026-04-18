## Project Structure

This repository contains 2 firmware projects, each for a different operational mode of the Rover. Only **one folder is flashed to Teensy at a time** — either autonomous or manual.

A shared `Assets/` folder holds all hardware-related documents (PCB files, datasheets, etc.) used across both modes.

```
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
├── Estop/                     # Wireless Emergency Stop system via ESP-NOW
│   ├── Estop_RX/              # Flashed into the rover's onboard ESP32 to controls the relay and contactor to cut power on estop trigger
│   │
│   ├── Estop_TX/              # Flashed into the remote unit handles button input and transmits estop signal to rover
│   │
│   └── MAC_ID                 # Flash this code in Esp first to find MAC_ID and then use it in TX code
│
└── README.md
```

## `src/` — Source Files

One `.cpp` file per subsystem. **Do not put everything in `main.cpp`.**

- `main.cpp` — Only `setup()` and `loop()`. Calls functions from other modules.
- Add a new `.cpp` for every new subsystem (e.g. `motor_control.cpp`, `encoder.cpp`, `jetson_comm.cpp`, `pid.cpp`, `imu.cpp`).

---

## `include/` — Header Files

Every `.cpp` in `src/` gets a matching `.h` here. Use `#pragma once` at the top of every header.

- **All pin numbers, baud rates, and tuning constants go in `config.h`** — never hardcode them inside `.cpp` files.
