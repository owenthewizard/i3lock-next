/*
 * i3lock-next-helper.c
 * This program takes a screenshot and blurs it for use with the
 * i3lock-next script.
 * NOTE: should not be run directly by user
 *
 */

/* Standard */
#include <stdlib.h>
#include <stdio.h>

/* X11 */
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

/* Imlib2 */
#include <Imlib2.h>

/* Config */
#include "config.h"

/* Debugging Messages */
#ifdef DEBUG
    #define D_PRINTF(fmt, ...) fprintf(stderr, "DEBUG: %s: %d: %s(): " fmt, \
                                            __FILE__, __LINE__, __func__,   \
                                            __VA_ARGS__);
#else
    #define D_PRINTF(fmt, ...) do{ } while (0)
#endif


int main(int argc, const char **argv)
{
    // NOTE: argv[1] is filename, argv[2] is font path, argv[3] is prompt
    //       (prompt is optional and should be set in the script)

    // error messages
    const char i3lockerr[]      =  "i3lock-next-helper: error:";
    const char wrongargs[]      =  "incorrect argument number";
    const char disperr[]        =  "can't open display";
    const char screenshoterr[]  =  "can't take screenshot";
    const char fonterr[]        =  "can't find font";
    const char lockimgerr[]     =  "can't load lock image";

    // only take two required arguments
    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "%s %s\n", i3lockerr, wrongargs);
        return 1;
    }

    // set main prompt (default: none)
    const char *prompt = (argc == 4)? argv[3] : "";
    D_PRINTF("Prompt: %s\n", prompt);

    // get current display or default to display :0
    const char *disp_name = getenv("DISPLAY");
    if (disp_name == NULL)
        disp_name = ":0";

    // attempt to open the display
    D_PRINTF("Attempting to get information for display %s\n", disp_name);
    Display *disp = XOpenDisplay(disp_name);

    if (disp == NULL)
    {
        // error: can't open display
        fprintf(stderr, "%s %s %s\n", i3lockerr, disperr, disp_name);
        return 2;
    }

    Window root = DefaultRootWindow(disp);
    Screen *dScreen = DefaultScreenOfDisplay(disp);
    int dScreenN = DefaultScreen(disp);

    // set Imlib2 context
    imlib_context_set_display(disp);
    imlib_context_set_visual(DefaultVisual(disp, dScreenN));
    imlib_context_set_colormap(DefaultColormap(disp, dScreenN));
    imlib_context_set_drawable(root);

    // get total width/height of default screen
    unsigned int dw = dScreen->width;
    unsigned int dh = dScreen->height;
    D_PRINTF("Full screen size: %dx%d\n", dw, dh);

    // set number of monitors
    int n = 1;
    XRRMonitorInfo *m = XRRGetMonitors(disp, root, 1, &n);
    XRRFreeMonitors(m);

    // for each monitor, determine the width, height, and position
    unsigned int widths[n];
    unsigned int heights[n];
    unsigned int xcoords[n];
    unsigned int ycoords[n];
    XRRScreenResources *screens = XRRGetScreenResources(disp, root);
    XRRCrtcInfo *screen = NULL;

    for (int i = 0; i < n; i++)
    {
        screen     = XRRGetCrtcInfo(disp, screens, screens->crtcs[i]);
        widths[i]  = screen->width;
        heights[i] = screen->height;
        xcoords[i] = screen->x;
        ycoords[i] = screen->y;
        D_PRINTF("Monitor %d: (x,y): (%d,%d)\n", i, xcoords[i], ycoords[i]);
        D_PRINTF("Monitor %d: %dx%d\n", i, widths[i], heights[i]);
    }

    XRRFreeScreenResources(screens);
    XRRFreeCrtcInfo(screen);

    // take a screenshot of the full screen
    Imlib_Image *im = imlib_create_image_from_drawable(1, 0, 0, dw, dh, 1);

    if (!im)
    {
        // coudln't take screenshot
        fprintf(stderr, "%s %s\n", i3lockerr, screenshoterr);
        XFree(disp);
        return 3;
    }

    // work on a new empty image
    imlib_context_set_image(imlib_create_image(dw, dh));

    // load the font, given like so:
    // /usr/share/fonts/open-sans/OpenSans-Regular/18
    imlib_context_set_font(imlib_load_font(argv[2]));

    if (!imlib_context_get_font())
    {
        // no font loaded
        fprintf(stderr, "%s %s %s\n", i3lockerr, fonterr, argv[2]);
        free(im);
        XFree(disp);
        return 4;
    }

    // declare a variable for offsetting the text
    int offset_w;

    // declare dummy varibles for information we don't need
    int dum;
    float dum2;

    // draw the text on our empty image and find out how many pixels we
    // need to offset it by
    imlib_text_draw_with_return_metrics(0, 0, prompt, &offset_w, &dum, &dum, &dum);
    D_PRINTF("Text offset: %d\n", offset_w);

    // discard old image and change context to screenshot
    imlib_free_image_and_decache();
    imlib_context_set_image(im);

    // setup some array of values to figure out whether we need to draw
    // light or dark icons and text
    float values[n];

    // repeat the above for each monitor
    for (int i = 0; i < n; ++i)
    {
        D_PRINTF("Attempting to find value for monitor %d\n", i);

        // calculate top left and right corner coordinates
        int tlw = widths[i] / 2 - 150 + xcoords[i];
        int tlh = heights[i] / 2 - 150 + ycoords[i];
        D_PRINTF("Monitor %d: box top left corner (x,y): (%d,%d)\n", i, tlw, tlh);

        // crop to 300x300 square at the center of monitor i
        // and change context
        Imlib_Image *c = imlib_create_cropped_image(tlw, tlh, 300, 300);
        imlib_context_set_image(c);

        // rescale to 3x3 and change context
        c = imlib_create_cropped_scaled_image(0, 0, 300, 300, 3, 3);
        imlib_context_set_image(c);

        // grab center pixel value
        imlib_image_query_pixel_hsva(2, 2, &dum2, &dum2, &values[i], &dum);
        D_PRINTF("Monitor %d: value %f\n", i, values[i]);

        // discard modifications and chage context back to whole screen
        imlib_free_image_and_decache();
        imlib_context_set_image(im);
    }

    // set up a color modifier
    imlib_context_set_color_modifier(imlib_create_color_modifier());

    // adjust gamma by GAMMA_ADJUST
    imlib_modify_color_modifier_gamma(GAMMA_ADJUST);
    imlib_apply_color_modifier();
    imlib_free_color_modifier();

    // scale the image down by 1 / SCALE_FACTOR and change context
    Imlib_Image *s = imlib_create_cropped_scaled_image(0, 0, dw, dh, dw/SCALE_FACTOR, dh/SCALE_FACTOR);
    imlib_context_set_image(s);

    // blur it a few times
    for (int i = 0; i < BLUR_ITERATIONS; i++)
        imlib_image_blur(BLUR_STRENGTH);

    // scale it back up and change context
    s = imlib_create_cropped_scaled_image(0, 0, dw/SCALE_FACTOR, dh/SCALE_FACTOR, dw, dh);
    imlib_context_set_image(s);

    // draw the lock and text on monitor(s)
    for (int i = 0; i < n; ++i)
    {
        D_PRINTF("Attempting to draw lock on monitor %d\n", i);

        // prepare to load lock image for this monitor
        Imlib_Image *lock;

        // determine which lock image to load based on monitor colour
        if (values[i] * 100 >= THRESHOLD)
        {
            lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-dark.png");
            imlib_context_set_color(0, 0, 0, 255);
        }
        else
        {
            lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-light.png");
            imlib_context_set_color(255, 255, 255, 255);
        }

        if (!lock)
        {
            // couldn't load lock image
            fprintf(stderr, "%s %s\n", i3lockerr, lockimgerr);
            free(im);
            XFree(disp);
            imlib_free_font();
            imlib_free_image_and_decache();
            return 3;
        }

        // set variables for prompt string location on monitor
        int promptx = widths[i] / 2 + xcoords[i] - offset_w / 2;
        int prompty = heights[i] / 1.5 + ycoords[i];
        D_PRINTF("Monitor %d: prompt (x,y): (%d,%d)\n", i, promptx, prompty);

        // draw prompt string just below the center of the monitor
        imlib_text_draw(promptx, prompty, prompt);

        // set variables for lock image location on monitor
        int lockx = widths[i] / 2 + xcoords[i] - LOCK_SIZE/2;
        int locky = heights[i] / 2 + ycoords[i] - LOCK_SIZE/2;
        D_PRINTF("Monitor %d: lock (x,y): (%d,%d)\n", i, lockx, locky);

        // draw lock image at the center of the monitor
        imlib_blend_image_onto_image(lock, 0, 0, 0, LOCK_SIZE, LOCK_SIZE, lockx, locky, LOCK_SIZE, LOCK_SIZE);
    }

    // save the image and cleanup
    D_PRINTF("Attempting to save image: path: %s\n", argv[1]);
    imlib_save_image(argv[1]);

    free(im);
    XFree(disp);
    imlib_free_font();
    imlib_free_image_and_decache();
}
