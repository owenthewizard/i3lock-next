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

/* Argument Parsing (and config) */
#include "i3lock-next.yucc"

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
    FilterType resize_filter = DEFAULT_FILTER;
    if (filter)
    {
        if (strcmp(filter, "Jinc") == 0)
        resize_filter = JincFilter;
        else if (strcmp(filter, "Blackman") == 0)
            resize_filter = BlackmanFilter;
        else if (strcmp(filter, "Box") == 0)
            resize_filter = BoxFilter;
        else if (strcmp(filter, "Catrom") == 0)
            resize_filter = CatromFilter;
        else if (strcmp(filter, "Hanning") == 0)
            resize_filter = HanningFilter;
        else if (strcmp(filter, "Hermite") == 0)
            resize_filter = HermiteFilter;
        else if (strcmp(filter, "Lanczos") == 0)
            resize_filter = LanczosFilter;
        else if (strcmp(filter, "Mitchell") == 0)
            resize_filter = MitchellFilter;
        else if (strcmp(filter, "Sinc") == 0)
            resize_filter = SincFilter;
        else if (strcmp(filter, "Triangle") == 0)
            resize_filter = TriangleFilter;
        else
        {
            fprintf(stderr, "%s\n", "error: invalid filter");
            status = 30;
        }
    }

    //establish other values
    double blur_radius = (radius)? strtod(radius, NULL) : DEFAULT_RADIUS;
    double blur_sigma = (sigma)? strtod(sigma, NULL) : DEFAULT_SIGMA;
    long int blur_scale = (scale)? strtol(scale, NULL, 10) : DEFAULT_SCALE;
    size_t width_large = MagickGetImageWidth(wand);
    size_t height_large = MagickGetImageHeight(wand);
    size_t width_small = width_large / blur_scale;
    size_t height_small = height_large / blur_scale;

    printf("radius:%f sigma:%f\n", blur_radius, blur_sigma);
    printf("width_small:%d, height_small:%d\n", width_small, height_small);
    printf("width_large:%d, height_large:%d\n", width_large, height_large);
    printf("scale: %d\n", blur_scale);

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
    typedef enum {BLUR, PIXELATE, NONE} Method;
    Method distort = DEFAULT_METHOD;
    if (argp->method_arg)
    {
        if (strcmp(argp->method_arg, "blur") == 0)
            distort = BLUR;
        else if (strcmp(argp->method_arg, "pixelate") == 0)
            distort = PIXELATE;
        else if (strcmp(argp->method_arg, "none") == 0)
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
            blur(wand, argp->radius_arg,
                 argp->sigma_arg,
                 argp->scale_factor_arg,
                 argp->filter_arg);
            break;
        case PIXELATE:
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
        output = fdopen(fd, "w");
        MagickWriteImageFile(wand, output);
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
    wand = DestroyMagickWand(wand);
    MagickWandTerminus();
    yuck_free(argp);
    return status;
}
