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

static int status;

void blur(MagickWand *wand, const char *radius, const char *sigma,
          const char *scale, const char *filter)
{
    //establish filter
    D_PRINTF("Filter: %s", filter);
    if (!filter)
        D_PRINTF("Using default filter");
    FilterType resize_filter = DEFAULT_FILTER;
    if (filter)
    {
        if (strcasecmp(filter, "Jinc") == 0)
        resize_filter = JincFilter;
        else if (strcasecmp(filter, "Blackman") == 0)
            resize_filter = BlackmanFilter;
        else if (strcasecmp(filter, "Box") == 0)
            resize_filter = BoxFilter;
        else if (strcasecmp(filter, "Catrom") == 0)
            resize_filter = CatromFilter;
        else if (strcasecmp(filter, "Hanning") == 0)
            resize_filter = HanningFilter;
        else if (strcasecmp(filter, "Hermite") == 0)
            resize_filter = HermiteFilter;
        else if (strcasecmp(filter, "Lanczos") == 0)
            resize_filter = LanczosFilter;
        else if (strcasecmp(filter, "Mitchell") == 0)
            resize_filter = MitchellFilter;
        else if (strcasecmp(filter, "Sinc") == 0)
            resize_filter = SincFilter;
        else if (strcasecmp(filter, "Triangle") == 0)
            resize_filter = TriangleFilter;
        else
        {
            fprintf(stderr, "%s\n", "error: invalid filter");
            status = 30;
        }
    }

    //establish other values
    D_PRINTF("Blur radius: %s", radius);
    if (!radius)
        D_PRINTF("Using default radius");
    double blur_radius = (radius)? strtod(radius, NULL) : DEFAULT_RADIUS;
    D_PRINTF("Blur sigma: %s", sigma);
    if (!sigma)
        D_PRINTF("Using default sigma");
    double blur_sigma = (sigma)? strtod(sigma, NULL) : DEFAULT_SIGMA;
    D_PRINTF("Scale factor: %s", scale);
    if (!scale)
        D_PRINTF("Using default scale");
    long int blur_scale = (scale)? strtol(scale, NULL, 10) : DEFAULT_SCALE;
    size_t width_large = MagickGetImageWidth(wand);
    D_PRINTF("Screenshot width: %zu", width_large);
    size_t height_large = MagickGetImageHeight(wand);
    D_PRINTF("Screenshot height: %zu", height_large);
    size_t width_small = width_large / blur_scale;
    D_PRINTF("Scaled width: %zu", width_small);
    size_t height_small = height_large / blur_scale;
    D_PRINTF("Scaled height: %zu", height_small);

    //scale down
    MagickResizeImage(wand, width_small, height_small, resize_filter);
    //blur
    MagickGaussianBlurImage(wand, blur_radius, blur_sigma);
    //scale up
    MagickResizeImage(wand, width_large, height_large, resize_filter);
}

int main(int argc, char **argv)
{
    status = 0;

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
    D_PRINTF("Distort set to: %s", argp->method_arg);
    if (!argp->method_arg)
        D_PRINTF("Using default distortion");
    typedef enum {BLUR, PIXELATE, NONE} Method;
    Method distort = DEFAULT_METHOD;
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
            fprintf(stderr, "%s\n",
                    "error: METHOD must be one of blur, pixelate, or none.");
            status = 10;
        }
    }

    //do it
    switch(distort)
    {
        case BLUR:
            D_PRINTF("Applying blur");
            blur(wand, argp->radius_arg,
                 argp->sigma_arg,
                 argp->scale_factor_arg,
                 argp->filter_arg);
            break;
        case PIXELATE:
            D_PRINTF("Applying pixelation");
            //TODO: pixelate
            break;
        case NONE:
            break;
    }

    //apply gamma adjust
    //TODO

    //add lock images
    //TODO
    //XXX multiple monitors

    //call i3lock
    //TODO

    //write out result
    ///: sizeof(char)
    //i3lock-next.XXXXXX.png: sizeof(char) * 22
    //\0: sizeof(char)
    char *file_name = malloc(sizeof(char) * 24
                      + strlen(P_tmpdir) * sizeof(char));
    FILE *output;
    int fd;
    if (file_name)
    {
        strcpy(file_name, P_tmpdir);
        strcat(file_name, "/i3lock-next.XXXXXX.png");
        fd = mkstemps(file_name, 4);
        D_PRINTF("Opening %s for writing", file_name);
        output = fdopen(fd, "w");
        D_PRINTF("Writing output to: %s", file_name);
        MagickWriteImageFile(wand, output);
        D_PRINTF("Closing %s", file_name);
        fclose(output);
        puts(file_name);
        free(file_name);
    }
    else
    {
        fprintf(stderr, "%s\n", "error: malloc failed!");
        status = 20;
    }

    //cleanup
    D_PRINTF("Cleaning up");
    wand = DestroyMagickWand(wand);
    MagickWandTerminus();
    yuck_free(argp);
    return status;
}
