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
#include <inttypes.h>
#include <math.h>
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
        fprintf(stderr, "i3lock-next:%s:%d warn: Imlib_Load_Error %d set "
                "while loading lock image\n", __FILE__, __LINE__, error);
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

/* for some reason not doing this de-referencing nonsense
   on `command` results in an empty string
   
   however, add_radius_to_args() is fine */
void add_args_to_command(char **argv, const int argc, char **command)
{
    /* don't pass arguments before "--" */
    size_t i = 0;
    /* dangerous cast? */
    while (strcmp(*(argv + i), "--") != 0 && i < (unsigned) (argc - 1))
        i++;
    /* pass arguments, starting at argv[i+1] */
    while (++i < (unsigned) argc)
    {
        /* X chars for argv[i] + 1 chars for " " */
        /* space for "\0" already allocated */
        *command = realloc(*command, sizeof(char) * (strlen(*command)
                                 + strlen(*(argv + i))
                                 + 1));
        sprintf(*command + strlen(*command), "%s ", *(argv + i));
    }
}

void add_radius_to_args(char *args, const int lock_w, const int lock_h)
{
    uint8_t radius = (uint8_t)
                     (1.1 * sqrt(pow(lock_w, 2) + pow(lock_h, 2)) / 2);
    D_PRINTF("Ring radius set to: %"PRIu8"\n", radius);

    /* for loop for smaller scope of `temp` */
    uint8_t radius_len = 1;
    for (uint8_t temp = radius; temp /= 10; radius_len++)
    { }

    /* 8 chars for "--radius=", radius_len chars for radius, 2 chars for " " */
    /* space for "\0" already allocated */
    args = realloc(args, sizeof(char) * (strlen(args) + 10 + radius_len));
    sprintf(args + strlen(args), "--radius=%"PRIu8" ", radius);
}

void die(const char *message, const char *file, const int line,
         const uint8_t code)
{
    fprintf(stderr, "i3lock-next:%s:%d: error: %s\n", file, line, message);
    exit(code);
}

void get_monitor_centers(Display *d, const Window w, const size_t monitors,
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

void warn(const char *message, const char *file, const int line)
{ fprintf(stderr, "i3lock-next:%s:%d: warn: %s\n", file, line, message); }

void warn_if_errno(const char *property, const char *file, const int line)
{
    if (errno != 0)
        fprintf(stderr, "i3lock-next:%s:%d: warn: errno set to %d (%s) while "
                "parsing %s\n", file, line, errno, strerror(errno), property);
}
