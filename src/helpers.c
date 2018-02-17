/*
 * helpers.c
 *
 * This file contains a few helper functions used
 * elsewhere.
 *
 * This file is part of i3lock-next.
 * See LICENSE for license details.
 *
 */

#include "helpers.h"
#include "config.h"
#include "i3lock-next.h"

#include <X11/extensions/Xrandr.h>

#include <errno.h>
#include <string.h>

char *get_font_file(FcConfig *config, const char *font_name, FcPattern **font)
{
    D_PRINTF("Searching for closest match to: %s\n", font_name);
    FcPattern *pat = FcNameParse((const FcChar8*) font_name);
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult dum;
    *font = FcFontMatch(config, pat, &dum);

    char *font_file;
    FcPatternGetString(*font, FC_FILE, 0, (FcChar8**) &font_file);

    FcPatternDestroy(pat);
    return font_file;
}

Imlib_Image *get_lock(const char *lock_arg, const bool dark)
{
    char *default_lock = (dark)?
        malloc(sizeof(char) * (strlen(DEFAULT_LOCK_DARK) + 1)) :
        malloc(sizeof(char) * (strlen(DEFAULT_LOCK_LIGHT) + 1));

    if (dark)
        strcpy(default_lock, DEFAULT_LOCK_DARK);
    else
        strcpy(default_lock, DEFAULT_LOCK_LIGHT);

    Imlib_Load_Error error = IMLIB_LOAD_ERROR_NONE;
    Imlib_Image *lock = (lock_arg)?
        imlib_load_image_with_error_return(lock_arg, &error) :
        imlib_load_image_with_error_return(default_lock, &error);
    if (error != IMLIB_LOAD_ERROR_NONE)
        fprintf(stderr, "i3lock-next: warning: Imlib_Load_Error %d set while "
                "loading lock image\n", error);
    FREE(default_lock);
    return lock;
}

int get_monitor_count(Display *d, const Window w)
{
    int n;
    XRRMonitorInfo *m = XRRGetMonitors(d, w, true, &n);                         
    XRRFreeMonitors(m);                                                         
    return n;
}


void die(const char *message, uint8_t code)
{
    fprintf(stderr, "i3lock-next: error: %s\n", message);
    exit(code);
}

void get_monitor_centers(Display *d, const Window w, const int monitors,
                         int *centers_x, int *centers_y)
{
    XRRScreenResources *screens = XRRGetScreenResources(d, w);

    XRRCrtcInfo *screen;
    for (size_t i = 0; i < monitors; i++)
    {
        screen = XRRGetCrtcInfo(d, screens, screens->crtcs[i]);

        D_PRINTF("Monitor %zu: (x,y): (%d,%d)\n", i, screen->x, screen->y);
        D_PRINTF("Monitor %zu: %ux%u\n", i, screen->width, screen->height);

        *(centers_x + i) = screen->width / 2 + screen->x;
        *(centers_y + i) = screen->height / 2 + screen->y;

        XRRFreeCrtcInfo(screen);
    }
    XRRFreeScreenResources(screens);
}

void warn_if_errno(const char *property)
{
    if (errno != 0)
        fprintf(stderr, "i3lock-next: warning: errno set to %d (%s) while "
                "parsing %s\n", errno, strerror(errno), property);
}
