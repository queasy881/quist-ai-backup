# ── VoxelReams Makefile (MSYS2 UCRT64) ──────────────────────────
#
# Prerequisites (run once in MSYS2 UCRT64 terminal):
#   pacman -S make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glm
#
# Build:   make -j$(nproc)
# Clean:   make clean
# ────────────────────────────────────────────────────────────────

CXX      := g++
CC       := gcc

# Compiler flags
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra \
            -Isrc \
            -Ideps/glad/include \
            -Ideps/stb

CFLAGS   := -O2 -Wall \
            -Ideps/glad/include

# Linker flags — glfw3 via pkg-config + OpenGL + Win32 + threads
LDFLAGS  := $(shell pkg-config --static --libs glfw3) -lopengl32 -lpthread

# ── Sources ─────────────────────────────────────────────────────
SRCS     := $(shell find src -name '*.cpp')
OBJS     := $(SRCS:.cpp=.o)

GLAD_SRC := deps/glad/src/gl.c
GLAD_OBJ := $(GLAD_SRC:.c=.o)

ALL_OBJS := $(OBJS) $(GLAD_OBJ)

TARGET   := VoxelReams.exe

# ── Rules ───────────────────────────────────────────────────────
.PHONY: all clean

all: $(TARGET)
	@echo "==============================="
	@echo " Build complete: $(TARGET)"
	@echo "==============================="

$(TARGET): $(ALL_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# C++ sources (any depth under src/)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# GLAD C source
$(GLAD_OBJ): $(GLAD_SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	find src -name '*.o' -delete
	rm -f $(GLAD_OBJ) $(TARGET)
