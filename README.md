# PixelRPG

A C++ game featuring different types of NPCs (Orc, Squirrel, Bear, Druid) that interact with each other in a simulated environment. The game now includes an SFML visual wrapper for a graphical representation of the game world.

## Features

- NPC interaction system using the visitor pattern
- Multiple NPC types with different behaviors
- Visual representation using SFML
- Console and file logging of interactions

## Dependencies

- C++20 compiler
- CMake 3.10 or higher
- SFML 2.5 or higher (for visual wrapper)

## Building the Project

### Installing SFML

**On Windows (using vcpkg):**
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg integrate install
./vcpkg install sfml
```

**On Ubuntu/Debian:**
```bash
sudo apt-get install libsfml-dev
```

**On macOS (using Homebrew):**
```bash
brew install sfml
```

### Building

```bash
mkdir build
cd build
cmake ..
make
```

Or on Windows with Visual Studio:
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build .
```

## Running the Game

The game will start with 50 randomly placed NPCs that will move around and interact with each other. If SFML is available, a visual window will open showing the game world.

## Architecture

- **NPC Types**: Orc, Squirrel, Bear, Druid
- **Interaction System**: Uses visitor pattern for different interaction types
- **Observer Pattern**: For logging and visual updates
- **Visual Wrapper**: SFML-based graphical interface

```
docker run -it \
  --name PixelRPG \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /mnt/wslg:/mnt/wslg \
  -e DISPLAY=:0 \
  -e WAYLAND_DISPLAY=wayland-0 \
  -e XDG_RUNTIME_DIR=/mnt/wslg/runtime-dir \
  -e PULSE_SERVER=/mnt/wslg/PulseServer \
  -v D:\Projects \
  --workdir /workspaces/PixelRPG/build \
  gcc:latest
```