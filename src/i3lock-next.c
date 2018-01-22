/*
 * i3lock-next.c
 * This program takes a screenshot and applies a distortion
 * effect specified by the user.
 *
 */

/* Standard */
#include <stdlib.h>
#include <stdio.h>

/* POSIX */
#include <unistd.h>

/* Data Types */
#include <stdbool.h>
#include <string.h>

/* X11 */
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

/* MagickWand */
#include <MagickWand/MagickWand.h>

/* Argument Parsing */
#include "i3lock-next.yucc"

/* Config */
#include "config.h"

/* Debug */
#ifdef DEBUG
    #define D_PRINTF(fmt, ...) fprintf(stderr, "DEBUG: %s: %d: %s(): " fmt, \
                                        __FILE__, __LINE__, __func__,       \
                                        __VA_ARGS__);
#else
    #define D_PRINTF(fmt, ...) do{ } while (0)
#endif

void die(const char *message, const int error)
{
    fprintf(stderr, "i3lock-next: error: %s\n", message);
    exit(error);
}

/*
void get_distort(char *distort, Method *m)
{

}
*/

int get_monitor_count(Display *d)
{
    int n;
    XRRMonitorInfo *m = XRRGetMonitors(d, DefaultRootWindow(d), true, &n);
    XRRFreeMonitors(m);
    return n;
}

void get_monitor_offsets(Display *d, const int monitors,
                         ssize_t *offsets_x, ssize_t *offsets_y,
                         const size_t lock_w, const size_t lock_h)
{
    XRRScreenResources *screens =
        XRRGetScreenResources(d, DefaultRootWindow(d));

    XRRCrtcInfo *screen;
    for (signed int i = 0; i < monitors; i++)
    {
        screen = XRRGetCrtcInfo(d, screens, screens->crtcs[i]);

        D_PRINTF("Monitor %d: (x,y): (%d,%d)\n", i, screen->x, screen->y);
        D_PRINTF("Monitor %d: %ux%u\n", i, screen->width, screen->height);

        *(offsets_x + i) = screen->width / 2 - lock_w / 2 + screen->x;
        *(offsets_y + i) = screen->height / 2 - lock_h / 2 + screen->y;

        XRRFreeCrtcInfo(screen);
    }
    XRRFreeScreenResources(screens);
}

void blur(MagickWand *wand, const char *radius,
          const char *sigma, const char *scale)
{
    //establish values
    D_PRINTF("Blur radius: %s\n", radius);
    if (!radius)
        D_PRINTF("Using default radius: %f\n", DEFAULT_RADIUS);
    double blur_radius = (radius)? strtod(radius, NULL) : DEFAULT_RADIUS;
    D_PRINTF("Blur sigma: %s\n", sigma);
    if (!sigma)
        D_PRINTF("Using default sigma: %f\n", DEFAULT_SIGMA);
    double blur_sigma = (sigma)? strtod(sigma, NULL) : DEFAULT_SIGMA;
    D_PRINTF("Scale factor: %s\n", scale);
    if (!scale)
        D_PRINTF("Using default scale: %d\n", DEFAULT_SCALE);
    long int blur_scale = (scale)? strtol(scale, NULL, 10) : DEFAULT_SCALE;
    size_t width_large = MagickGetImageWidth(wand);
    D_PRINTF("Screenshot width: %zu\n", width_large);
    size_t height_large = MagickGetImageHeight(wand);
    D_PRINTF("Screenshot height: %zu\n", height_large);
    size_t width_small = width_large / blur_scale;
    D_PRINTF("Scaled width: %zu\n", width_small);
    size_t height_small = height_large / blur_scale;
    D_PRINTF("Scaled height: %zu\n", height_small);

    //scale down
    MagickResizeImage(wand, width_small, height_small, LanczosFilter);
    //blur
    MagickGaussianBlurImage(wand, blur_radius, blur_sigma);
    //scale up
    MagickResizeImage(wand, width_large, height_large, MitchellFilter);
}

int main(const int argc, char *argv[])
{
    //parse arguments
    yuck_t argp[1];
    yuck_parse(argp, argc, argv);

    //init wand
    MagickWandGenesis();
    MagickWand *wand = NewMagickWand();
    MagickSetFormat(wand, "png");

    //take screenshot
    MagickReadImage(wand, "x:root");

    //find out what we want to do
    D_PRINTF("Distort set to: %s\n", argp->method_arg);
    if (!argp->method_arg)
        D_PRINTF("%s\n", "Using default distortion");
    typedef enum {BLUR, PIXELATE, NONE} Method;
    Method distort;
    //get_distort(argp->method_arg &distort);
    if (argp->method_arg)
    {
        if (strcasecmp(argp->method_arg, "blur") == 0)
            distort = BLUR;
        else if (strcasecmp(argp->method_arg, "pixelate") == 0)
            distort = PIXELATE;
        else if (strcasecmp(argp->method_arg, "none") == 0)
            distort = NONE;
        else
        {
            wand = DestroyMagickWand(wand);
            MagickWandTerminus();
            yuck_free(argp);
            die("METHOD must be any-of (blur, pixelate, none)", 10);
        }
    }
    else
        distort = DEFAULT_METHOD;

    //do it
    switch(distort)
    {
        case BLUR:
            D_PRINTF("%s\n", "Applying blur");
            blur(wand, argp->radius_arg,
                 argp->sigma_arg,
                 argp->scale_factor_arg);
            break;

        case PIXELATE:
            D_PRINTF("%s\n", "Applying pixelation");
            //TODO: pixelate
            break;

        case NONE:
            break;
    }

    //apply gamma adjust
    D_PRINTF("Gamma: %s\n", argp->gamma_arg);
    if (!argp->gamma_arg)
        D_PRINTF("Using default gamma: %f\n", DEFAULT_GAMMA);
    double gamma = (argp->gamma_arg)?
        strtod(argp->gamma_arg, NULL) : DEFAULT_GAMMA;
    MagickGammaImage(wand, gamma);


    //add lock images
    char *lock_image_l, *lock_image_d;

    if (argp->lock_light_arg)
    {
        lock_image_l = malloc(sizeof(char) * strlen(argp->lock_light_arg)
                       + sizeof(char));
        if (lock_image_l)
            strcpy(lock_image_l, argp->lock_light_arg);
        else
        {
            wand = DestroyMagickWand(wand);
            MagickWandTerminus();
            yuck_free(argp);
            die("malloc failed", 20);
        }
    }
    else
    {
        lock_image_l = malloc(sizeof(char) * strlen(DEFAULT_LOCK_LIGHT)
                       + sizeof(char));
        if (lock_image_l)
            strcpy(lock_image_l, DEFAULT_LOCK_LIGHT);
        else
        {
            wand = DestroyMagickWand(wand);
            MagickWandTerminus();
            yuck_free(argp);
            die("malloc failed", 20);
        }
    }

    if (argp->lock_dark_arg)
    {
        lock_image_d = malloc(sizeof(char) * strlen(argp->lock_dark_arg)
                       + sizeof(char));
        if (lock_image_d)
            strcpy(lock_image_d, argp->lock_dark_arg);
        else
        {
            wand = DestroyMagickWand(wand);
            MagickWandTerminus();
            yuck_free(argp);
            die("malloc failed", 20);
        }
    }
    else
    {
        lock_image_d = malloc(sizeof(char) * strlen(DEFAULT_LOCK_DARK)
                       + sizeof(char));
        if (lock_image_d)
            strcpy(lock_image_d, DEFAULT_LOCK_DARK);
        else
        {
            wand = DestroyMagickWand(wand);
            MagickWandTerminus();
            yuck_free(argp);
            die("malloc failed", 20);
        }
    }

    MagickWand *wand_lock_l = NewMagickWand();
    MagickWand *wand_lock_d = NewMagickWand();
    MagickReadImage(wand_lock_l, lock_image_l);
    MagickReadImage(wand_lock_d, lock_image_d);

    MagickWand *wand_cropped = NULL;
    PixelWand *center = NewPixelWand();

    double threshold = (argp->threshold_arg)?
        strtod(argp->threshold_arg, NULL) : DEFAULT_THRESH;

    size_t lock_w = MagickGetImageWidth(wand_lock_l);
    size_t lock_h = MagickGetImageHeight(wand_lock_l);

    //open display
    //if you want to use xcb instead of Xlib then write some decent
    //documentation or submit a PR
    Display *disp = XOpenDisplay(NULL);
    if (!disp)
        die("can't open display", 30);

    signed int monitors = get_monitor_count(disp);

    ssize_t offsets_x[monitors], offsets_y[monitors];
    get_monitor_offsets(disp, monitors, offsets_x, offsets_y, lock_w, lock_h);

    XCloseDisplay(disp);

    for (signed int i = 0; i < monitors; i++)
    {
        wand_cropped = CloneMagickWand(wand);
        //light/dark calculation a bit messed up because we use the offsets
        //for the lock drawing instead of using our own
        MagickCropImage(wand_cropped, lock_w * 2, lock_h * 2,
                        offsets_x[i], offsets_y[i]);
        MagickResizeImage(wand_cropped, lock_w, lock_h, LanczosFilter);
        MagickGetImagePixelColor(wand_cropped, lock_w / 2, lock_h / 2, center);
        double h, s, l;
        PixelGetHSL(center, &h, &s, &l);

        ClearMagickWand(wand_cropped);
        ClearPixelWand(center);

        if (l * 100 >= threshold)
            MagickCompositeImage(wand, wand_lock_d, OverCompositeOp, MagickFalse,
                                 offsets_x[i], offsets_y[i]);
        else
            MagickCompositeImage(wand, wand_lock_l, OverCompositeOp, MagickFalse,
                                 offsets_x[i], offsets_y[i]);
    }
    if (wand_cropped != NULL)
        wand_cropped = DestroyMagickWand(wand_cropped);
    center = DestroyPixelWand(center);
    wand_lock_l = DestroyMagickWand(wand_lock_l);
    wand_lock_d = DestroyMagickWand(wand_lock_d);

    free(lock_image_l);
    free(lock_image_d);

    //call i3lock
    //TODO

    //write out result
    char file_name[] = P_tmpdir"/i3lock-next.XXXXXX.png";
    D_PRINTF("Opening %s for writing\n", file_name);
    FILE *output = fdopen(mkstemps(file_name, 4), "w");
    D_PRINTF("Writing output to: %s\n", file_name);
    MagickWriteImageFile(wand, output);
    D_PRINTF("Closing %s\n", file_name);
    fclose(output);
    puts(file_name);

    //cleanup
    D_PRINTF("%s\n", "Cleaning up");
    wand = DestroyMagickWand(wand);
    MagickWandTerminus();
    yuck_free(argp);
}

// vim: set colorcolumn=80 :
