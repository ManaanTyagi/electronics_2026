## Project Structure

```
Embedded_UGVC/
│
├── assets/
│   ├── PCB Files/         # KiCad Files
│   ├── pcb/               # PCB layout files, Gerbers, and BOM
│
├──platformio.ini
├──src/               # Source files (.cpp)
├──include/           # Header files (.h)
├──lib/               # Private/project-specific libraries
├──test/              # Unit tests
│
└── README.md
```

---

## `src/` — Source Files

One `.cpp` file per subsystem. **Do not put everything in `main.cpp`.**

- `main.cpp` — Only `setup()` and `loop()`. Calls functions from other modules.
- Add a new `.cpp` for every new subsystem (e.g. `motor_control.cpp`, `encoder.cpp`, `jetson_comm.cpp`, `pid.cpp`, `imu.cpp`).

---

## `include/` — Header Files

Every `.cpp` in `src/` gets a matching `.h` here. Use `#pragma once` at the top of every header.h

- **All pin numbers, baud rates, and tuning constants go in `config.h`** — never hardcode them inside `.cpp` files.

---

