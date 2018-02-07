/*
 * i3lock-next.c
 * This program takes a screenshot and applies a distortion
 * effect specified by the user, then calls i3lock.
 *
 */

#include "i3lock-next.h"

#include "config.h"
#include "i3lock-next.yucc"

#include <unistd.h>
#include <X11/extensions/Xrandr.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(const int argc, char *argv[])
{
    /* parse arguments */
    yuck_t argp[1];
    yuck_parse(argp, argc, argv);

    /* Open DISPLAY */
    Display *disp = XOpenDisplay(NULL);
    if (!disp)
        die("Can't open display", 10);

    /* Init some variables */
    Window root = DefaultRootWindow(disp);
    Screen *default_screen = DefaultScreenOfDisplay(disp);
    int screen_number = DefaultScreen(disp);

    /* Init Imlib2 context */
    imlib_context_set_display(disp);
    Visual *visual = DefaultVisual(disp, screen_number);
    imlib_context_set_visual(visual);
    imlib_context_set_colormap(DefaultColormap(disp, screen_number));
    imlib_context_set_drawable(root);

    /* Get monitor info */
    int32_t total_width = default_screen->width;
    int32_t total_height = default_screen->height;
    D_PRINTF("Total resolution: %"PRId32"x%"PRId32"\n", \
             total_width, total_height);

    int monitors = get_monitor_count(disp, root);
    D_PRINTF("Found %d monitor(s)\n", monitors);

    /* Get distortion to apply */
    Method distortion;
    get_distort(argp->method_arg, &distortion);

    /* Take the screenshot */
    uint8_t scale;
    if (distortion == NONE)
        scale = 1;
    else
        scale = get_scale(argp->scale_factor_arg);
    D_PRINTF("Scale set to: %"PRIu8"\n", scale);

    D_PRINTF("%s\n", "Taking screenshot");
    Imlib_Image *screenshot_main =
        imlib_create_scaled_image_from_drawable(0, 0, 0,
                total_width, total_height,
                total_width / scale,
                total_height / scale, 1, 0);
    imlib_context_set_image(screenshot_main);

    /* Apply distortion */
    uint8_t blur_strength, blur_iter;
    switch(distortion)
    {
        case BLUR:
            get_blur_details(argp->strength_arg, argp->iter_arg,
                    &blur_strength, &blur_iter);
            D_PRINTF("Applying %"PRIu8" iterations of blur with strength of \
                     %"PRIu8"\n", blur_iter, blur_strength);
            for (int8_t i = 0; i < blur_iter; i++)
                imlib_image_blur(blur_strength);
            break;
        case PIXELATE:
            /* nothing for now */
            D_PRINTF("%s\n", "PIXELATE not yet implemented");
            break;
        case NONE:
            /* nothing for now */
            break;
    }

    /* Scale screenshot back up */
    screenshot_main =
        imlib_create_cropped_scaled_image(0, 0,
                                          total_width / scale,
                                          total_height / scale,
                                          total_width, total_height);
    imlib_free_image();

    /* Apply gamma adjust */
    imlib_context_set_image(screenshot_main);
    float gamma;
    D_PRINTF("Gamma arg: %s\n", argp->gamma_arg);
    if (argp->gamma_arg)
    {
        errno = 0;
        gamma = strtof(argp->gamma_arg, NULL);
        if (errno != 0)
            D_PRINTF("Function strtof() set errno to: %d (%s)\n",
                     errno, strerror(errno));
    }
    else
        gamma = DEFAULT_GAMMA;

    D_PRINTF("Gamma set to: %f\n", gamma);
    imlib_context_set_color_modifier(imlib_create_color_modifier());
    imlib_modify_color_modifier_gamma(gamma);
    imlib_apply_color_modifier();
    imlib_free_color_modifier();

    screenshot_main = imlib_context_get_image();

    /* Get lock width and height */
    Imlib_Image *lock_light, *lock_dark;
    Imlib_Load_Error imlib_error;

    imlib_error = IMLIB_LOAD_ERROR_NONE;
    if (argp->lock_light_arg)
        lock_light = imlib_load_image_with_error_return(
                        argp->lock_light_arg, &imlib_error);
    else
        lock_light = imlib_load_image_with_error_return(
                        DEFAULT_LOCK_LIGHT, &imlib_error);
    if (imlib_error != IMLIB_LOAD_ERROR_NONE)
        fprintf(stderr, "Failed to load light lock image, \
                Imlib_Load_Error: %d\n", imlib_error);

    imlib_error = IMLIB_LOAD_ERROR_NONE;
    if (argp->lock_dark_arg)
        lock_dark = imlib_load_image_with_error_return(
                argp->lock_dark_arg, &imlib_error);
    else
        lock_dark = imlib_load_image_with_error_return(
                DEFAULT_LOCK_DARK, &imlib_error);
    if (imlib_error != IMLIB_LOAD_ERROR_NONE)
        fprintf(stderr, "Failed to load dark lock image, \
                Imlib_Load_Error: %d\n", imlib_error);

    int lock_w, lock_h;
    imlib_context_set_image(lock_light);
    lock_w = imlib_image_get_width();
    lock_h = imlib_image_get_height();
    imlib_free_image();

    /* Draw lock icons */
    int16_t offsets_x[monitors], offsets_y[monitors];
    get_monitor_offsets(disp, root, monitors,
                        offsets_x, offsets_y,
                        lock_w, lock_h);
    XFree(disp);

    int8_t thresh;
    if (argp->threshold_arg)
        thresh = strtol(argp->threshold_arg, NULL, 10);
    else
        thresh = DEFAULT_THRESHOLD;
    D_PRINTF("Threshold set to: %"PRId8"\n", thresh);

    Imlib_Image *cropped = NULL;
    float light, dum;
    for (int8_t i = 0; i < monitors; i++)
    {
        imlib_context_set_image(screenshot_main);
        cropped =
            imlib_create_cropped_scaled_image(
                    offsets_x[i] - lock_w / 2,
                    offsets_y[i] - lock_h / 2,
                    lock_w, lock_h, 1, 1);
        imlib_context_set_image(cropped);
        imlib_image_query_pixel_hlsa(0, 0, &dum, &light, &dum, &dum);
        imlib_free_image();

        imlib_context_set_image(screenshot_main);
        if (light * 100 > thresh)
            imlib_blend_image_onto_image(lock_dark, false, 0, 0,
                                         lock_w, lock_h,
                                         offsets_x[i], offsets_y[i],
                                         lock_w, lock_h);
        else
            imlib_blend_image_onto_image(lock_light, false, 0, 0,
                                         lock_w, lock_h,
                                         offsets_x[i], offsets_y[i],
                                         lock_w, lock_h);
    }

    /* Draw text */
    //TODO

    /* Write out result */
    char file_name[] = P_tmpdir"/i3lock-next.XXXXXX.png";
    mkstemps(file_name, 4);
    D_PRINTF("Writing output to %s\n", file_name);
    imlib_error = IMLIB_LOAD_ERROR_NONE;
    imlib_save_image_with_error_return(file_name, &imlib_error);
    if (imlib_error != IMLIB_LOAD_ERROR_NONE)
        D_PRINTF("Imlib_Load_Error: %d\n", imlib_error);

    /* Call i3lock */
    //TODO: pass arguments
    char *i3lock = malloc(sizeof(char) * 11
                          + sizeof(char) * strlen(file_name));
    strcpy(i3lock, "i3lock -i ");
    strcat(i3lock, file_name);
    system(i3lock);
    unlink(file_name);
    FREE(i3lock);

    /* Cleanup */
    yuck_free(argp);
}

inline uint8_t get_scale(const char *scale_arg)
{
    uint8_t scale;
    D_PRINTF("Scale factor arg: %s\n", scale_arg);
    if (scale_arg)
        scale = strtol(scale_arg, NULL, 10);
    else
        scale = DEFAULT_SCALE;
    return (scale > 0)? scale : 1;
}

inline void get_blur_details(const char *blur_arg, const char *iter_arg,
        uint8_t *strength, uint8_t *iter)
{
    D_PRINTF("Blur strength arg: %s\n", blur_arg);
    if (blur_arg)
        *strength = strtol(blur_arg, NULL, 10);
    else
        *strength = DEFAULT_STRENGTH;
    D_PRINTF("strength set to: %"PRIu8"\n", *strength);

    D_PRINTF("Blur iterations arg: %s\n", iter_arg);
    if (iter_arg)
        *iter = strtol(iter_arg, NULL, 10);
    else
        *iter = DEFAULT_ITER;
    D_PRINTF("Iterations set to: %"PRIu8"\n", *iter);
}

inline void get_distort(const char *distort, Method *m)
{
    if (distort)
    {
        if (strcasecmp(distort, "blur") == 0)
            *m = BLUR;
        else if (strcasecmp(distort, "pixelate") == 0)
            *m = PIXELATE;
        else if (strcasecmp(distort, "none") == 0)
            *m = NONE;
        else
            die("METHOD must be any-of (blur, pixelate, none)", 10);
    }
    else
    {
        D_PRINTF("%s\n", "Using default distortion");
        *m = DEFAULT_METHOD;
    }
}

inline int get_monitor_count(Display *d, const Window w)
{
    int n;
    XRRMonitorInfo *m = XRRGetMonitors(d, w, true, &n);
    XRRFreeMonitors(m);
    return n;
}

void get_monitor_offsets(Display *d, const Window w, const int monitors,
                         int16_t *offsets_x, int16_t *offsets_y,
                         const int lock_w, const int lock_h)
{
    XRRScreenResources *screens =                                               
        XRRGetScreenResources(d, w);                         
                                                                                
    XRRCrtcInfo *screen;                                                        
    for (int8_t i = 0; i < monitors; i++)                                   
    {                                                                           
        screen = XRRGetCrtcInfo(d, screens, screens->crtcs[i]);                 
                                                                                
        D_PRINTF("Monitor %"PRId8": (x,y): (%d,%d)\n",
                 i, screen->x, screen->y);      
        D_PRINTF("Monitor %"PRId8": %ux%u\n",
                 i, screen->width, screen->height);      
                                                                                
        *(offsets_x + i) = screen->width / 2 - lock_w / 2 + screen->x;          
        *(offsets_y + i) = screen->height / 2 - lock_h / 2 + screen->y;         
                                                                                
        XRRFreeCrtcInfo(screen);                                                
    }                                                                           
    XRRFreeScreenResources(screens);
}

inline void die(const char *message, uint8_t code)
{
    fprintf(stderr, "i3lock-next: error: %s\n", message);
    exit(code);
}
