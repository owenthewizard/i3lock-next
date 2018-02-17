/*
 * sanitizers.c
 *
 * This file contains functions for parsing command-line
 * arguments.
 *
 * This file is a part of i3lock-next.
 * See LICENSE for license details.
 *
 */

#include "sanitizers.h"
#include "config.h"
#include "helpers.h"
#include "i3lock-next.h"

#include <errno.h>
#include <string.h>

float get_gamma(const char *gamma_arg)
{
    errno = 0;
    D_PRINTF("Gamma arg: %s\n", gamma_arg);
    return (gamma_arg)? strtof(gamma_arg, NULL) : DEFAULT_GAMMA;
}

int8_t get_threshold(const char *threshold_arg)
{
    errno = 0;
    D_PRINTF("Threshold arg: %s\n", threshold_arg);
    return (threshold_arg)? strtol(threshold_arg, NULL, 10) : DEFAULT_THRESH;
}

Method get_distort(const char *distort_arg)
{
    D_PRINTF("Distort arg: %s\n", distort_arg);
    if (!distort_arg)
        return DEFAULT_METHOD;
    if (strcasecmp(distort_arg, "blur") == 0)
        return BLUR;
    else if (strcasecmp(distort_arg, "pixelate") == 0)
        return PIXELATE;
    else if (strcasecmp(distort_arg, "none") == 0)
        return NONE;
    else
        die("METHOD must be any-of {blur, pixelate, none}", 20);
    /* Unreachable code to satisfy compiler warning*/
    return BLUR;
}

uint8_t get_blur_iter(const char *blur_arg)
{
    errno = 0;
    D_PRINTF("Blur iter arg: %s\n", blur_arg);
    return (blur_arg)? strtol(blur_arg, NULL, 10) : DEFAULT_ITER;
}

uint8_t get_blur_strength(const char *blur_arg)
{
    errno = 0;
    D_PRINTF("Blur strength arg: %s\n", blur_arg);
    return (blur_arg)? strtol(blur_arg, NULL, 10) : DEFAULT_STRENGTH;
}

uint8_t get_font_size(const char *font_size)
{
    errno = 0;
    D_PRINTF("Font size arg: %s\n", font_size);
    return (font_size)? strtol(font_size, NULL, 10) : DEFAULT_FONT_SIZE;
}

uint8_t get_text_index(const char *index_arg)
{
    errno = 0;
    D_PRINTF("Text index arg: %s\n", index_arg);
    return (index_arg)? strtol(index_arg, NULL, 10) : 0;
}

uint8_t get_scale(const char *scale_arg)
{
    errno = 0;
    uint8_t scale;
    D_PRINTF("Scale factor arg: %s\n", scale_arg);
    scale = (scale_arg)? strtol(scale_arg, NULL, 10) : DEFAULT_SCALE;
    return (scale > 0)? scale : 1;
}
