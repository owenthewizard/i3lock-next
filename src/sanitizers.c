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
#include <inttypes.h>
#include <string.h>

double get_gamma(const char *gamma_arg)
{
    errno = 0;
    D_PRINTF("Gamma arg: %s\n", gamma_arg);
    return (gamma_arg)? strtod(gamma_arg, NULL) : DEFAULT_GAMMA;
}

int8_t get_threshold(const char *threshold_arg)
{
    errno = 0;
    D_PRINTF("Threshold arg: %s\n", threshold_arg);
    int8_t thresh;
    if (threshold_arg)
        sscanf(threshold_arg, "%"SCNd8, &thresh);
    else
        thresh = DEFAULT_THRESH;
    return thresh;
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
        die("METHOD must be any-of {blur, pixelate, none}", __FILE__,
            __LINE__, 20);
    /* Unreachable code to satisfy compiler warning*/
    return BLUR;
}

uint8_t get_blur_iter(const char *blur_arg)
{
    errno = 0;
    D_PRINTF("Blur iter arg: %s\n", blur_arg);
    uint8_t iter;
    if (blur_arg)
        sscanf(blur_arg, "%"SCNu8, &iter);
    else
        iter = DEFAULT_ITER;
    return iter;
}

uint8_t get_blur_strength(const char *blur_arg)
{
    errno = 0;
    D_PRINTF("Blur strength arg: %s\n", blur_arg);
    uint8_t strength;
    if (blur_arg)
        sscanf(blur_arg, "%"SCNu8, &strength);
    else
        strength = DEFAULT_STRENGTH;
    return strength;
}

uint8_t get_font_size(const char *font_size)
{
    errno = 0;
    D_PRINTF("Font size arg: %s\n", font_size);
    uint8_t font_sz;
    if (font_size)
        sscanf(font_size, "%"SCNu8, &font_sz);
    else
        font_sz = DEFAULT_FONT_SIZE;
    return font_sz;
}

uint8_t get_text_index(const char *index_arg)
{
    errno = 0;
    uint8_t index;
    D_PRINTF("Text index arg: %s\n", index_arg);
    if (index_arg)
        sscanf(index_arg, "%"SCNu8, &index);
    else
        index = 0;
    return index;
}

uint8_t get_scale(const char *scale_arg)
{
    errno = 0;
    uint8_t scale;
    D_PRINTF("Scale factor arg: %s\n", scale_arg);
    if (scale_arg)
        sscanf(scale_arg, "%"SCNu8, &scale);
    else
        scale = DEFAULT_SCALE;
    return (scale > 0)? scale : 1;
}
