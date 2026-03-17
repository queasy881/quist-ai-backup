# VOXELREAMS

A Minecraft-like voxel terrain exploration engine built with C++17 and OpenGL 4.6 Core.

## Requirements

- CMake 3.20+
- C++17 compiler (MSVC 2019+, GCC 9+, Clang 10+)
- Python 3 (for GLAD2 loader generation at configure time)
- GPU with OpenGL 4.6 Core support

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Run

Place texture PNGs in `assets/textures/` then run the executable:

```
./VoxelReams          # Linux / macOS
VoxelReams.exe        # Windows
```

## Controls

| Key       | Action          |
| --------- | --------------- |
| W/A/S/D   | Move            |
| Mouse     | Look            |
| Space     | Jump            |
| Shift     | Sprint          |
| Left Click| Mine            |
| Right Click| Place          |
| E         | Inventory       |
| Scroll    | Hotbar select   |
| F3        | Debug info      |
| F4        | Chunk borders   |
| Escape    | Release cursor  |

## Textures (assets/textures/)

The engine expects these 16×16 PNG files:

- grass_top.png
- grass_side.png
- dirt.png
- stone.png
- sand.png
- water.png
- wood.png
- leaves.png
- snow.png
- deepstone.png
- gravel.png
