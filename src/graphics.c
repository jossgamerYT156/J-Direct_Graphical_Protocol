// lots of function definitions and a lot of different includes i already put inside of graphics.h but i include again because of issues when building.
// remove includes at your own risk.
// it might not work if you do.
#include <Include/graphical/graphics.h>
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

// some function prototypes i should've defined inside graphics.h
static int drm_fd = -1;
static drmModeCrtc *saved_crtc = NULL;
static uint32_t *framebuffer = NULL;
static struct drm_mode_create_dumb create_dumb;
static drmModeCrtc *crtc = NULL;
static drmModeConnector *connector = NULL;
static uint32_t fb_id;

// begining of useful functions.
// Function to autodetect the DRM device
char *detect_drm_device() {
    DIR *dir = opendir("/dev/dri");
    if (!dir) {
        perror("Failed to open /dev/dri");
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "card", 4) == 0) {
            char *drm_device = malloc(strlen("/dev/dri/") + strlen(entry->d_name) + 1);
            if (!drm_device) {
                perror("Failed to allocate memory for DRM device path");
                closedir(dir);
                return NULL;
            }
            sprintf(drm_device, "/dev/dri/%s", entry->d_name);
            closedir(dir);
            return drm_device;
        }
    }

    closedir(dir);
    return NULL;
}

// Function to detect DRM device, initialize graphics environment, and set up the framebuffer
void init_graphics() {
    char *drm_device = detect_drm_device();
    if (!drm_device) {
        fprintf(stderr, "Failed to detect DRM device\n");
        exit(EXIT_FAILURE);
    }

    drm_fd = open(drm_device, O_RDWR);
    free(drm_device);
    if (drm_fd < 0) {
        perror("Failed to open DRM device");
        exit(EXIT_FAILURE);
    }

    drmModeRes *resources = drmModeGetResources(drm_fd);
    if (!resources) {
        perror("Failed to get DRM resources");
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < resources->count_connectors; ++i) {
        connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (connector->connection == DRM_MODE_CONNECTED) {
            break;
        }
        drmModeFreeConnector(connector);
        connector = NULL;
    }

    if (!connector) {
        fprintf(stderr, "No connected connector found\n");
        drmModeFreeResources(resources);
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    drmModeEncoder *encoder = drmModeGetEncoder(drm_fd, connector->encoder_id);
    if (!encoder) {
        fprintf(stderr, "Failed to get encoder\n");
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    crtc = drmModeGetCrtc(drm_fd, encoder->crtc_id);
    if (!crtc) {
        fprintf(stderr, "Failed to get CRTC\n");
        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    saved_crtc = drmModeGetCrtc(drm_fd, encoder->crtc_id);

    memset(&create_dumb, 0, sizeof(create_dumb));
    create_dumb.width = crtc->width;
    create_dumb.height = crtc->height;
    create_dumb.bpp = 32;

    if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) < 0) {
        perror("Failed to create dumb buffer");
        drmModeFreeCrtc(crtc);
        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    struct drm_mode_map_dumb map_dumb = {0};
    map_dumb.handle = create_dumb.handle;
    if (drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb) < 0) {
        perror("Failed to map dumb buffer");
        drmModeFreeCrtc(crtc);
        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    framebuffer = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map_dumb.offset);
    if (framebuffer == MAP_FAILED) {
        perror("Failed to mmap framebuffer");
        drmModeFreeCrtc(crtc);
        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    if (drmModeAddFB(drm_fd, crtc->width, crtc->height, 24, 32, create_dumb.pitch, create_dumb.handle, &fb_id)) {
        perror("Failed to create framebuffer");
        munmap(framebuffer, create_dumb.size);
        drmModeFreeCrtc(crtc);
        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(drm_fd);
        exit(EXIT_FAILURE);
    }

    drmModeFreeEncoder(encoder);
    drmModeFreeResources(resources);
}

// warning, this still doesn't work very well.
// Function to create a window with given dimensions and title
void create_window(int width, int height, const char *title) {
    init_graphics();

    // Fill the screen with a background color
    jFillScreen('K'); // 'K' for black

    // Draw a rectangle representing the window
    drawRectangle(framebuffer, 100, 100, width, height, 0xFFFFFF); // White window

    // Draw a mouse pointer and update its position based on user input
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int mouse_x = crtc->width / 2;
    int mouse_y = crtc->height / 2;

    fd_set set;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while (1) {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        int res = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
        if (res > 0 && FD_ISSET(STDIN_FILENO, &set)) {
            char c = getchar();
            if (c == 'q') {
                break;
            }
            // Example movement: WASD keys
            if (c == 'w') mouse_y -= 10;
            if (c == 'a') mouse_x -= 10;
            if (c == 's') mouse_y += 10;
            if (c == 'd') mouse_x += 10;

            // Clear previous mouse pointer
            jFillScreen('K');
            drawRectangle(framebuffer, 100, 100, width, height, 0xFFFFFF);
            drawMousePointer(framebuffer, mouse_x, mouse_y, 0xFF0000); // Red mouse pointer
        }

        // Set the CRTC to update the screen
        if (drmModeSetCrtc(drm_fd, crtc->crtc_id, fb_id, 0, 0, &connector->connector_id, 1, &crtc->mode) < 0) {
            perror("Failed to set CRTC");
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    close_graphics();
}

// this does works well... a bit too well.
// Function to fill the screen with a specified color
void jFillScreen(char color) {
    uint32_t color_val;
    switch (color) {
        case 'R':
            color_val = 0xFF0000; // Red
            break;
        case 'G':
            color_val = 0x00FF00; // Green
            break;
        case 'B':
            color_val = 0x0000FF; // Blue
            break;
        case 'K':
            color_val = 0x000000; // Black
            break;
        default:
            color_val = 0x000000; // Default to black
            break;
    }

    for (int i = 0; i < create_dumb.size / 4; ++i) {
        framebuffer[i] = color_val;
    }
}

// as well as create_window(); this doesn't work very well.
// Function to draw a rectangle
void drawRectangle(uint32_t *fb_ptr, int x, int y, int width, int height, uint32_t color) {
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            fb_ptr[(y + j) * crtc->width + (x + i)] = color;
        }
    }
}

// kinda works, but needs a bit more workaround.
// Function to draw a simple square as a mouse pointer
void drawMousePointer(uint32_t *fb_ptr, int x, int y, uint32_t color) {
    int size = 10; // Size of the mouse pointer
    drawRectangle(fb_ptr, x, y, size, size, color);
}

// still on development.
// Function to clean up and restore the original display settings
void close_graphics() {
    if (drm_fd >= 0) {
        if (saved_crtc) {
            drmModeSetCrtc(drm_fd, saved_crtc->crtc_id, saved_crtc->buffer_id, saved_crtc->x, saved_crtc->y, &connector->connector_id, 1, &saved_crtc->mode);
            drmModeFreeCrtc(saved_crtc);
        }

        drmModeRmFB(drm_fd, fb_id);
        struct drm_mode_destroy_dumb destroy_dumb = {0};
        destroy_dumb.handle = create_dumb.handle;
        drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb);

        close(drm_fd);
        drm_fd = -1;
    }
}
// notes:
// pressing: CTRL+ALT+F4 will stop J-DGP and kick you to the Terminal or last Graphical Environment if possible.
// some parameters need adjusting since this was made within a Celeron PC and it works kinda crappy, as well, some code might not work with YOUR GPU.
// be careful when copying this code.
// i don't make responsible if you break your GPU because you didn't checked the code before running it.
