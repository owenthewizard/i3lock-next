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
            D_PRINTF("Applying %"PRIu8" iterations of blur with strength of %"
                    PRIu8"\n", blur_iter, blur_strength);
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
                        offsets_x, offsets_y);
    XFree(disp);

    int8_t thresh;
    if (argp->threshold_arg)
        thresh = strtol(argp->threshold_arg, NULL, 10);
    else
        thresh = DEFAULT_THRESHOLD;
    D_PRINTF("Threshold set to: %"PRId8"\n", thresh);

    Imlib_Image *cropped = NULL;
    float light, dum;
    for (size_t i = 0; i < monitors; i++)
    {
        imlib_context_set_image(screenshot_main);
        cropped =
            imlib_create_cropped_scaled_image(
                    offsets_x[i] - lock_w / 2,
                    offsets_y[i] - lock_h / 2,
                    lock_w, lock_h, 1, 1);
        imlib_context_set_image(cropped);
        imlib_image_query_pixel_hlsa(0, 0, &dum, &light, &dum, (int*) &dum);
        imlib_free_image();

        imlib_context_set_image(screenshot_main);
        if (light * 100 > thresh)
            imlib_blend_image_onto_image(lock_dark, false, 0, 0,
                                         lock_w, lock_h,
                                         offsets_x[i] - lock_w / 2,
                                         offsets_y[i] - lock_h / 2,
                                         lock_w, lock_h);
        else
            imlib_blend_image_onto_image(lock_light, false, 0, 0,
                                         lock_w, lock_h,
                                         offsets_x[i] - lock_w / 2,
                                         offsets_y[i] - lock_h / 2,
                                         lock_w, lock_h);
    }
    screenshot_main = imlib_context_get_image();

    /* Load font */
    D_PRINTF("%s\n", "Loading fontconfig config...");
    FcConfig *config = FcInitLoadConfigAndFonts();
    add_fonts_to_imlib_context(config);
    D_PRINTF("Font arg: %s\n", argp->font_arg);
    char *font;
    if (argp->font_arg)
        font = get_font_file(config, argp->font_arg);
    else
        font = get_font_file(config, DEFAULT_FONT);
    FcConfigDestroy(config);

    if (!font)
        die("Failed to find an appropriate font", 30);

    D_PRINTF("Font size arg: %s\n", argp->font_size_arg);
    uint8_t font_size;
    if (argp->font_size_arg)
    {
        errno = 0;
        font_size = strtol(argp->font_size_arg, NULL, 10);
        if (errno != 0)
            fprintf(stderr, "Warning: errno was set to %d, %s\n",
                    errno, strerror(errno));
    }
    else
        font_size = DEFAULT_FONT_SIZE;

    char *imlib_font_arg = malloc(sizeof(char) * strlen(font) + 4);
    if (imlib_font_arg)
        sprintf(imlib_font_arg, "%s/%"PRIu8, font, font_size);
    else
        die("malloc() failed!", 40);

    imlib_context_set_font(imlib_load_font(imlib_font_arg));
    D_PRINTF("Loaded font file: %s with size %"PRIu8"\n", font, font_size);
    D_PRINTF("%s\n", imlib_font_arg);
    FREE(imlib_font_arg);

    /* Calculate text offsets */
    D_PRINTF("Text index arg: %s\n", argp->text_index_arg);
    uint8_t text_index;
    if (argp->text_index_arg)
    {
        errno = 0;
        text_index = strtol(argp->text_index_arg, NULL, 10);
        if (errno != 0)
            fprintf(stderr, "Warning: errno was set to %d, %s\n",
                    errno, strerror(errno));
    }
    else
        text_index = 0;

    D_PRINTF("Prompt arg: \"%s\"\n", argp->prompt_arg);
    int text_offset_w;
    imlib_context_set_image(imlib_create_image(total_width, total_height));
    /* prompt set via argument and is not "" */
    if (argp->prompt_arg && strcmp(argp->prompt_arg, "") != 0)
        imlib_text_draw_with_return_metrics(0, 0, argp->prompt_arg,
                                            &text_offset_w, (int*) &dum,
                                            (int*) &dum, (int*) &dum);
    /* prompt argument unset and DEFAULT_PROMPT is not "" */
    else if (strcmp(DEFAULT_PROMPT, "") != 0)
        imlib_text_draw_with_return_metrics(0, 0, DEFAULT_PROMPT,
                                            &text_offset_w, (int*) &dum,
                                            (int*) &dum, (int*) &dum);
    imlib_free_image_and_decache();

    /* Draw text */
    imlib_context_set_image(screenshot_main);
    if (argp->prompt_arg && text_offset_w)
        imlib_text_draw(offsets_x[text_index] - text_offset_w / 2,
                        offsets_y[text_index] + lock_h * 3,
                        argp->prompt_arg);
    else if (text_offset_w)
        imlib_text_draw(offsets_x[text_index] - text_offset_w / 2,
                        offsets_y[text_index] + lock_h * 3,
                        DEFAULT_PROMPT);
    imlib_free_font();

    /* Write out result */
    char file_name[] = P_tmpdir"/i3lock-next.XXXXXX.png";
    mkstemps(file_name, 4);
    D_PRINTF("Writing output to %s\n", file_name);
    imlib_error = IMLIB_LOAD_ERROR_NONE;
    imlib_save_image_with_error_return(file_name, &imlib_error);
    if (imlib_error != IMLIB_LOAD_ERROR_NONE)
        D_PRINTF("Imlib_Load_Error: %d\n", imlib_error);
    imlib_free_image();
    return 0;

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
            die("METHOD must be any-of (blur, pixelate, none)", 20);
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
        int16_t *offsets_x, int16_t *offsets_y)
{
    XRRScreenResources *screens = XRRGetScreenResources(d, w);                         

    XRRCrtcInfo *screen;                                                        
    for (size_t i = 0; i < monitors; i++)                                   
    {                                                                           
        screen = XRRGetCrtcInfo(d, screens, screens->crtcs[i]);                 

        D_PRINTF("Monitor %zu: (x,y): (%d,%d)\n",
                i, screen->x, screen->y);      
        D_PRINTF("Monitor %zu: %ux%u\n",
                i, screen->width, screen->height);      

        *(offsets_x + i) = screen->width / 2 + screen->x;          
        *(offsets_y + i) = screen->height / 2 + screen->y;         

        XRRFreeCrtcInfo(screen);                                                
    }                                                                           
    XRRFreeScreenResources(screens);
}

inline void add_fonts_to_imlib_context(FcConfig *config)
{
    FcStrList *font_dirs = FcConfigGetFontDirs(config);
    FcChar8 *path;
    while ((path = FcStrListNext(font_dirs)))
    {
        D_PRINTF("Adding font path: %s\n", path);
        imlib_add_path_to_font_path((const char*) path);
    }
    FcStrFree(path);
    FcStrListDone(font_dirs);

    /*
    int total_paths;
    char **paths = imlib_list_font_path(&total_paths);
    for (int8_t i = 0; i < total_paths; i++)
        D_PRINTF("Imlib2 font path (%"PRId8"/%d): %s\n",
                i + 1, total_paths, *(paths+i));
    imlib_free_font_list(paths, total_paths);
    */
}

inline char *get_font_file(FcConfig *config, const char *font_name)
{
    D_PRINTF("Searching for closest match to: %s\n", font_name);
    FcPattern *pat = FcNameParse((const FcChar8*) font_name);
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult dum;
    FcPattern *font = FcFontMatch(config, pat, &dum);

    char *font_file = NULL;
    if (!font
        || FcPatternGetString(font, FC_FILE, 0, (FcChar8**) &font_file) != FcResultMatch)
        fprintf(stderr, "Could not find a font file for %s\n", font_name);

    /* TODO: free this later
    FcPatternDestroy(font);
    */
    FcPatternDestroy(pat);
    return font_file;
}

inline void die(const char *message, uint8_t code)
{
    fprintf(stderr, "i3lock-next: error: %s\n", message);
    exit(code);
}
