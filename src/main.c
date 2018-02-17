/*
 * main.c
 *
 * This program takes a screenshot and applies a distortion
 * effect specified by the user, then calls i3lock.
 *
 * This file is part of i3lock-next.
 * See LICENSE for license details.
 *
 */

#include "i3lock-next.h"
#include "helpers.h"
#include "processing.h"
#include "sanitizers.h"

#include <Imlib2.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include <inttypes.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
    /* Init some variables */
    Display *disp = XOpenDisplay(NULL);
    if (!disp)
        die("i3lock-next: error: failed to open display", 10);
    Window root = DefaultRootWindow(disp);
    Screen *screen = DefaultScreenOfDisplay(disp);
    //int screen_n = DefaultScreen(disp);

    /* Get some info about the monitor */
    int total_width = screen->width;
    int total_height = screen->height;
    D_PRINTF("Total resolution: %dx%d\n", total_width, total_height);
    int monitors = get_monitor_count(disp, root);
    D_PRINTF("Found %d monitor(s)\n", monitors);

    /* Read arguments */
    yuck_t argp[1];
    yuck_parse(argp, argc, argv);

    Method distortion = get_distort(argp->method_arg);

    uint8_t scale;
    if (distortion == NONE)
        scale = 1;
    else
        scale = get_scale(argp->scale_factor_arg);
    warn_if_errno("scale");
    D_PRINTF("Scale set to %"PRIu8"\n", scale);

    D_PRINTF("%s\n", "Setting Imlib2 context...");
    set_imlib2_context(disp, root, DefaultScreen(disp), argp->font_arg,
                       argp->font_size_arg);

    D_PRINTF("%s\n", "Taking screenshot");

    Imlib_Image *screenshot =
        imlib_create_scaled_image_from_drawable(false, 0, 0, total_width,
                                                total_height,
                                                total_width / scale,
                                                total_height / scale,
                                                true, false);
    distort(screenshot, distortion, argp);

    if (scale != 1)
    {
        screenshot =
            imlib_create_cropped_scaled_image(0, 0, total_width / scale,
                                              total_height / scale,
                                              total_width, total_height);
        imlib_free_image();
    }

    adjust_gamma(screenshot, argp->gamma_arg);

    int centers_x[monitors], centers_y[monitors];
    get_monitor_centers(disp, root, monitors, centers_x, centers_y);
    Imlib_Image *lock_d = get_lock(argp->lock_dark_arg, true);
    Imlib_Image *lock_l = get_lock(argp->lock_light_arg, false);
    int8_t threshold = get_threshold(argp->threshold_arg);
    warn_if_errno("threshold");

    int lock_w, lock_h;

    draw_lock_icons(screenshot, lock_d, lock_l, centers_x, centers_y,
                    threshold, monitors, &lock_w, &lock_h);
    
    XFree(screen);
    XFree(disp);

    draw_text(screenshot, argp->prompt_arg, argp->text_index_arg, total_width,
              total_height, lock_h, centers_x, centers_y);

    char file_name[] = P_tmpdir"/i3lock-next.XXXXXX.png";
    mkstemps(file_name, 4);
    D_PRINTF("Writing output to %s\n", file_name);
    Imlib_Load_Error imlib_error = IMLIB_LOAD_ERROR_NONE;
    imlib_save_image_with_error_return(file_name, &imlib_error);
    if (imlib_error != IMLIB_LOAD_ERROR_NONE)
        D_PRINTF("Imlib_Load_Error set while saving: %d\n", imlib_error);
    imlib_free_image();

    /* Call i3lock */
    //TODO: pass args
    char *i3lock = malloc(sizeof(char) * 11
            + sizeof(char) * strlen(file_name));
    strcpy(i3lock, "i3lock -i ");
    strcat(i3lock, file_name);
    //system(i3lock);
    unlink(file_name);
    FREE(i3lock);

    /* Cleanup */
    yuck_free(argp);
}
