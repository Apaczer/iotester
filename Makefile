
TARGET = iotester

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip

SYSROOT		?= $(shell $(CC) --print-sysroot)
PKGS		:= sdl SDL_image SDL_ttf
PKGS_CFLAGS	:= $(shell $(SYSROOT)/../../usr/bin/pkg-config --cflags $(PKGS))
PKGS_LIBS	:= $(shell $(SYSROOT)/../../usr/bin/pkg-config --libs $(PKGS))

CFLAGS = -O0 -ggdb -g3 -Os $(PKGS_CFLAGS) -I/usr/include/ -I$(SYSROOT)/usr/include/  -I$(SYSROOT)/usr/include/SDL/
CFLAGS += -std=c++11 -fdata-sections -ffunction-sections -fno-exceptions -fno-math-errno -fno-threadsafe-statics

LDFLAGS = $(SDL_LIBS) $(PKGS_LIBS)
LDFLAGS += -Wl,--as-needed -Wl,--gc-sections

all:
	gcc iotester.c -g -o $(TARGET) $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(TARGET)
