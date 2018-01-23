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

signed int get_args_to_pass(const int argc, char *argv[], signed int *eop)
{
    signed int value = 0;
    bool flag = false;
    for (signed int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--") == 0 && !flag)
        {
            flag = true;
            *eop = i;
            continue;
        }
        if (flag)
            value++;
    }
    if (!*eop)
        *eop = -1;
    return value;
}

signed int get_args_bytes(const int argc, char *argv[],
                          const signed int eop, const signed int i3lock_args)
{
    signed int value = 0;
    if (i3lock_args > 0 && eop > 0)
    {
        for (signed int i = eop + 1; i < argc; i++)
            value += strlen(argv[i]) * sizeof(char); //arguments
        value += sizeof(char) * (i3lock_args - 1);   //spaces between arguments
        value += sizeof(char);                       //space (after i3lock)
    }
    value += sizeof(char) * 7; //i3lock + null
    return value;
}

void construct_i3lock_args(const int argc, char *argv[], const int eop,
                           const int i3lock_args_n, char *i3lock_args)
{
    if (i3lock_args)
    {
        strcpy(i3lock_args, "i3lock");
        if (i3lock_args_n > 0 && eop > 0)
        {
            strcat(i3lock_args, " ");
            for (signed int i = eop + 1, spaces = 0; i < argc; i++)
            {
                strcat(i3lock_args, argv[i]);
                if (spaces < i3lock_args_n - 1)
                {
                    strcat(i3lock_args, " ");
                    spaces++;
                }
            }
        }
    }
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

    /*
    printf("%s", "args:");
    for (signed int i = 0; i < argc; i++)
        printf(" %s", argv[i]);
    printf("\n");
    */

    //printf("argc: %d, argp->nargs: %zu\n", argc, argp->nargs);

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
            die("malloc() failed", 20);
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
            die("malloc() failed", 20);
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
            die("malloc() failed", 20);
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
            die("malloc() failed", 20);
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

    yuck_free(argp);

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

    //write out result
    char file_name[] = P_tmpdir"/i3lock-next.XXXXXX.png";
    D_PRINTF("Opening %s for writing\n", file_name);
    FILE *output = fdopen(mkstemps(file_name, 4), "w");
    D_PRINTF("Writing output to %s\n", file_name);
    MagickWriteImageFile(wand, output);
    D_PRINTF("Closing %s\n", file_name);
    fclose(output);

    wand = DestroyMagickWand(wand);
    MagickWandTerminus();

    //construct args for i3lock
    signed int end_of_parameter;
    signed int args_after = get_args_to_pass(argc, argv, &end_of_parameter);
    D_PRINTF("Detected %d arguments for i3lock\n", args_after);
    if (end_of_parameter > 0)
        D_PRINTF("Detected '--' at position %d\n", end_of_parameter);

    char *i3lock_args = malloc(get_args_bytes(argc, argv,
                               end_of_parameter, args_after)
                               + strlen(file_name) * sizeof(char)
                               + sizeof(char) * 4); //" -i "
    construct_i3lock_args(argc, argv, end_of_parameter,
                          args_after, i3lock_args);
    int i3lock_status;
    if (i3lock_args)
    {
        D_PRINTF("%s\n", "Adding image argument to i3lock");
        strcat(i3lock_args, " -i ");
        strcat(i3lock_args, file_name);
        D_PRINTF("calling i3lock like so: \"%s\"\n", i3lock_args);
        //call i3lock
        //yep, system() isn't secure
        i3lock_status = system(i3lock_args);
        D_PRINTF("deleting %s\n", file_name);
        unlink(file_name);
        D_PRINTF("i3lock: exit %d\n", i3lock_status);
        free(i3lock_args);
    }
    else
    {
        D_PRINTF("%s\n", "error: malloc() failed");
        die("malloc() failed", 20);
    }
}

// vim: set colorcolumn=80 :
