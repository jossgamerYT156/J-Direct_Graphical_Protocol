// here we stored the imports and most of the prototype definitions...
// version 1 from graphics.h inside ./Include/graphics/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <drm.h>
#include <drm_mode.h>
#include <dirent.h>
#include <string.h>
#include <termios.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// Function declarations
char *detect_drm_device();
void jFillScreen(char color);
void drawRectangle(uint32_t *fb_ptr, int x, int y, int width, int height, uint32_t color);
void drawMousePointer(uint32_t *fb_ptr, int x, int y, uint32_t color);

// Function to initialize the graphics environment
void init_graphics();

// Function to create a window with given dimensions and title
void create_window(int width, int height, const char *title);

// Function to display everything drawn on the screen
void display();

// Function to close the graphics environment and free resources
void close_graphics();

#endif /* GRAPHICS_H */
