
BUILDTIME=$(shell date +'\"%Y-%m-%d %H:%M\"')

CC = arm-linux-gcc
CXX = arm-linux-g++
STRIP = arm-linux-strip

SYSROOT     := $(shell $(CC) --print-sysroot)
SDL_CFLAGS  := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS    := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

CFLAGS = -DTARGET_RETROFW -D__BUILDTIME__="$(BUILDTIME)" -DLOG_LEVEL=0 -g0 -Os $(SDL_CFLAGS) -I/usr/include/ -I$(SYSROOT)/usr/include/  -I$(SYSROOT)/usr/include/SDL/
CFLAGS += -std=c++11 -fdata-sections -ffunction-sections -fno-exceptions -fno-math-errno -fno-threadsafe-statics

LDFLAGS = $(SDL_LIBS) -lfreetype -lSDL_image -lSDL_ttf -lSDL -lpthread
LDFLAGS += -Wl,--as-needed -Wl,--gc-sections -s

pc:
	gcc iotester.c -g -o iotester -ggdb -O0 -DDEBUG -lSDL_image -lSDL -lSDL_ttf -I/usr/include/SDL

clean:
	rm -rf iotester
