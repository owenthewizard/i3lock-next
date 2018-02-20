/*                                                                              
 * processing.c                                                                 
 *                                                                              
 * This file contains functions for applying distortions                        
 * and drawing on the screenshot.                                               
 *                                                                              
 * This file is part of i3lock-next.                                            
 * See LICENSE for license details.                                             
 *                                                                              
 */

#include "processing.h"
#include "config.h"
#include "helpers.h"
#include "i3lock-next.h"
#include "sanitizers.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

void adjust_gamma(Imlib_Image *i, const char *gamma_arg)
{
    imlib_context_set_image(i);

    float gamma = get_gamma(gamma_arg);
    warn_if_errno("gamma", __FILE__, __LINE__);
    D_PRINTF("Gamma set to %.2f\n", gamma);

    imlib_context_set_color_modifier(imlib_create_color_modifier());
    imlib_modify_color_modifier_gamma(gamma);
    imlib_apply_color_modifier();
    imlib_free_color_modifier();
}

void distort(Imlib_Image *im, const Method m, const yuck_t *argp)
{
    imlib_context_set_image(im);
    
    uint8_t blur_strength, blur_iter;
    switch(m)
    {
        case BLUR:
            blur_strength = get_blur_strength(argp->strength_arg);
            D_PRINTF("Blur strength set to %"PRIu8"\n", blur_strength);
            blur_iter = get_blur_iter(argp->iter_arg);
            D_PRINTF("Blur iterations set to %"PRIu8"\n", blur_iter);
            for (int8_t i = 0; i < blur_iter; i++)
                imlib_image_blur(blur_strength);
            break;

        case PIXELATE:
            /* Done when capturing screenshot */
            break;

        case NONE:
            break;
    }
}

void draw_lock_icons(Imlib_Image *im, Imlib_Image *lock_d, Imlib_Image *lock_l,
                     const int *centers_x, const int *centers_y,
                     const int8_t threshold, const size_t monitors,
                     int *lock_w, int *lock_h)
{
    imlib_context_set_image(lock_d);
    *lock_w = imlib_image_get_width();
    *lock_h = imlib_image_get_height();
    imlib_free_image();

    Imlib_Image *cropped = NULL;
    float light, dum;
    for (size_t i = 0; i < monitors; i++)
    {
        imlib_context_set_image(im);
        cropped =
            imlib_create_cropped_scaled_image(*(centers_x + i) - *lock_w / 2,
                                              *(centers_y + i) - *lock_h / 2,
                                              *lock_w, *lock_h, 1, 1);
        imlib_context_set_image(cropped);
        imlib_image_query_pixel_hlsa(0, 0, &dum, &light, &dum, (int*) &dum);
        imlib_free_image();

        imlib_context_set_image(im);
        if (light * 100 > threshold)
            imlib_blend_image_onto_image(lock_d, false, 0, 0, *lock_w, *lock_h,
                                         *(centers_x + i) - *lock_w / 2,
                                         *(centers_y + i) - *lock_h / 2,
                                         *lock_w, *lock_h);
        else
            imlib_blend_image_onto_image(lock_l, false, 0, 0, *lock_w, *lock_h,
                                         *(centers_x + i) - *lock_w / 2,
                                         *(centers_y + i) - *lock_h / 2,
                                         *lock_w, *lock_h);
    }
}

void draw_text(Imlib_Image *im, const char *prompt, const char *index_arg,
               const int8_t threshold, const int w, const int h,
               const int *centers_x, const int *centers_y)
{
    /* Check if we need to draw text */
    if ((!prompt && strcmp(DEFAULT_PROMPT, "") == 0)
        || strcmp(prompt, "") == 0)
    {
        D_PRINTF("%s\n", "Text drawing skipped");
        return;
    }

    uint8_t index = get_text_index(index_arg);
    warn_if_errno("text index", __FILE__, __LINE__);

    /* Calculate text offsets */
    D_PRINTF("Prompt arg: %s\n", prompt);
    int text_offset_w, text_offset_h, dum;
    imlib_context_set_image(imlib_create_image(w, h));
    /* prompt set via argument and is not "" */
    if (prompt && strcmp(prompt, "") != 0)
        imlib_text_draw_with_return_metrics(0, 0, prompt, &text_offset_w,
                                            &text_offset_h, &dum, &dum);
    /* prompt argument unset and default prompt is not "" */
    else
        imlib_text_draw_with_return_metrics(0, 0, DEFAULT_PROMPT,
                                            &text_offset_w, &text_offset_h,
                                            &dum, &dum);
    imlib_free_image();

    /* Get text area lightness */
    float light;
    imlib_context_set_image(im);
    Imlib_Image *cropped =
        imlib_create_cropped_scaled_image(*(centers_x + index)
                                          - text_offset_w / 2,
                                          (int) (*(centers_y + index) * 1.5),
                                          text_offset_w, text_offset_h, 1, 1);
    imlib_context_set_image(cropped);
    imlib_image_query_pixel_hlsa(0, 0, (float*) &dum, &light, (float*) &dum,
                                 &dum);
    imlib_free_image();

    if (light * 100 > threshold)
        imlib_context_set_color(0, 0, 0, 255);
    else
        imlib_context_set_color(255, 255, 255, 255);

    /* Draw text */
    imlib_context_set_image(im);
    if (prompt && text_offset_w)
        imlib_text_draw(*(centers_x + index) - text_offset_w / 2, 
                        (int) (*(centers_y + index) * 1.5), prompt);
    else if (text_offset_w)
        imlib_text_draw(*(centers_x + index) - text_offset_w / 2,
                        (int) (*(centers_y + index) * 1.5), DEFAULT_PROMPT);
    imlib_free_font();
}

void set_imlib2_context(Display *d, const Window w, const int screen,
                        const char *font_arg, const char *size)
{
    imlib_context_set_display(d);
    imlib_context_set_anti_alias(1);
    imlib_context_set_visual(DefaultVisual(d, screen));
    imlib_context_set_colormap(DefaultColormap(d, screen));
    imlib_context_set_drawable(w);

    D_PRINTF("%s\n", "Loading fontconfig config...");
    FcConfig *config = FcInitLoadConfigAndFonts();
    FcStrList *font_dirs = FcConfigGetFontDirs(config);
    FcChar8 *path;
    while ((path = FcStrListNext(font_dirs)))
    {
        D_PRINTF("Adding font path: %s\n", path);
        imlib_add_path_to_font_path((const char*) path);
    }
    FcStrFree(path);
    FcStrListDone(font_dirs);

    D_PRINTF("Font arg: %s\n", font_arg);
    FcPattern *temp;
    char *font = (font_arg)? get_font_file(config, font_arg, &temp) :
        get_font_file(config, DEFAULT_FONT, &temp);
    FcConfigDestroy(config);
    if (!font)
        die("Failed to fine an appropriate font", __FILE__, __LINE__, 30);

    D_PRINTF("Font size arg: %s\n", size);
    uint8_t font_size = get_font_size(size);
    warn_if_errno("font size", __FILE__, __LINE__);
    
    char *imlib_font_arg = malloc(sizeof(char) * (strlen(font) + 4));
    if (imlib_font_arg)
        sprintf(imlib_font_arg, "%s/%"PRIu8, font, font_size);
    else
        die("malloc() failed!", __FILE__, __LINE__, 40);

    imlib_context_set_font(imlib_load_font(imlib_font_arg));
    D_PRINTF("Loaded font file: %s with size %"PRIu8"\n", font, font_size);
    D_PRINTF("%s\n", imlib_font_arg);
    FREE(imlib_font_arg);
    FcPatternDestroy(temp);
    FREE(font);
}
