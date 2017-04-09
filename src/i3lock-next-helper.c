/*
 * i3lock-next-helper.c
 * This program takes a screenshot and blurs it for use with the
 * i3lock-next script.
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

int main(int argc, const char **argv)
{
	// NOTE: argv[1] is filename and argv[2] is font path

    // main prompt (TODO: GNU gettext here?)
    char prompt[]         =  "Type password to unlock";

    // error messages
    char i3lockerr[]      =  "i3lock-next-helper: error:";
    char wrongargs[]      =  "incorrect argnument number";
    char disperr[]        =  "can't open display";
    char screenshoterr[]  =  "can't take screenshot";
    char fonterr[]        =  "can't find font";
    char lockimgerr[]     =  "can't load lock image";

    // only take two arguments
    if (argc != 3)
    {
        fprintf(stderr, "%s %s\n", i3lockerr, wrongargs);
        return 1;
    }

    // get current display or default to display :0
	const char *disp_name = getenv("DISPLAY");

    if (disp_name == NULL)
        disp_name = ":0";

    // attempt to open the display
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

	// set number of monitors
    int n = 1;
    XRRMonitorInfo *m = XRRGetMonitors(disp, root, 1, &n);
    XRRFreeMonitors(m);

	// for each monitor, determine the width and height
    unsigned int widths[n];
    unsigned int heights[n];
    XRRScreenResources *screens = XRRGetScreenResources(disp, root);
    XRRCrtcInfo *screen = NULL;

    for (int i = 0; i < n; i++)
    {
        screen = XRRGetCrtcInfo(disp, screens, screens->crtcs[i]);
        widths[i] = screen->width;
        heights[i] = screen->height;
    }

    XRRFreeScreenResources(screens);
	XRRFreeCrtcInfo(screen);

    // take a screenshot of the full screen
    Imlib_Image *im = imlib_create_image_from_drawable(1, 0, 0, dw, dh, 1);

    if (!im)
    {
		// coudln't take screenshot
        fprintf(stderr, "%s %s\n", i3lockerr, screenshoterr);
		free(disp);
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
		free(disp);
        return 4;
    }

    // declare a  variable for offsetting the text
    int offset_w = 0;

	// dummy varibles for information we don't need
	int dummy = 0;
	float dummy2 = 0;

    // draw the text on our empty image and find out how many pixels we
    // need to offset it by
    imlib_text_draw_with_return_metrics(0, 0, prompt, &offset_w, &dummy, &dummy, &dummy);

    //Work on the screenshot, discarding the old image
    imlib_free_image_and_decache();
    imlib_context_set_image(im);

    // setup some value to figure out wether we need to draw light or
	// dark icons and text
    float values[n];

    // crop the image to 300/300 (from center)
    imlib_context_set_image(imlib_create_cropped_image(widths[0]/2-150, heights[0]/2-150, 300, 300));

    // scale the image to 3x3
    imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, 300, 300, 3, 3));

    // grab the center pixel's value (doesn't work with 1x1 for some reason...)
    imlib_image_query_pixel_hsva(2, 2, &dummy2, &dummy2, &values[0], &dummy);

    // discard modifications, back to the screenshot
    imlib_free_image_and_decache();
    imlib_context_set_image(im);

    // repeat the above for each monitor
    for (int i = 1; i < n; i++)
    {
        imlib_context_set_image(imlib_create_cropped_image(widths[i]/2-150+widths[i-1], heights[i]/2-150+heights[i-1], 300, 300));
        imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, 300, 300, 3, 3));
        imlib_image_query_pixel_hsva(2, 2, &dummy2, &dummy2, &values[i], &dummy);
        imlib_context_set_image(im);
    }

    // set up a color modifier
    imlib_context_set_color_modifier(imlib_create_color_modifier());

    // darken the image to 45% brightness
    imlib_modify_color_modifier_gamma(0.45);
    imlib_apply_color_modifier();
    imlib_free_color_modifier();

    // scale the image down to 20%
    imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, dw, dh, dw/5, dh/5));

    // blur it a few times
    imlib_image_blur(1);
    imlib_image_blur(1);
    imlib_image_blur(1);

    // scale it back up
    imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, dw/5, dh/5, dw, dh));

    // save these changes
    //im = imlib_context_get_image();

    // prepare to load the lock image
    Imlib_Image *lock;

    // determine which lock image to load for the first monitor
    if (values[0]*100 >= 60)
    {
		// monitor is bright
        lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-dark.png");
        imlib_context_set_color(0, 0, 0, 255);
    }
    else
    {
		// monitor is dark
        lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-light.png");
        imlib_context_set_color(255, 255, 255, 255);
    }

	// couldn't load lock
    if (!lock)
    {
        fprintf(stderr, "%s %s\n", i3lockerr, lockimgerr);
		free(im);
		free(disp);
		imlib_free_font();
		imlib_free_image_and_decache();
        return 3;
    }

	// define variables for prompt string location on monitor
	int promptx = widths[0] / 2 - offset_w / 2;
	int prompty = heights[0] / 1.5;

	// draw prompt just below the centre of the monitor
    imlib_text_draw(promptx, prompty, prompt);

	// define variables for location of lock image on monitor
	int lockx = widths[0] / 2 - 40;
	int locky = heights[0] / 2 - 40;

	// draw lock image at centre on monitor
    imlib_blend_image_onto_image(lock, 0, 0, 0, 80, 80, lockx, locky, 80, 80);

    // draw the lock and text on the other monitors (if they exist)
    for (int i = 1; i < n; i++)
    {
		// determine which lock image to load based on monitor colour
        if (values[i]*100 >= 60)
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
			free(disp);
			imlib_free_font();
			imlib_free_image_and_decache();
            return 3;
        }

		// set variables for prompt string location on monitor
		promptx = widths[i] / 2 - offset_w / 2 + widths[i - 1];
		prompty = heights[i] / 1.5;

		// draw prompt string just below the centre of the monitor
        imlib_text_draw(promptx, prompty, prompt);

		// set variables for lock image location on monitor
		lockx = widths[i] / 2 - 40 + widths[i - 1];
		locky = heights[i] / 2 - 40;

		// draw lock image at the centre of the monitor
        imlib_blend_image_onto_image(lock, 0, 0, 0, 80, 80, lockx, locky, 80, 80);
    }

    // save the image and cleanup
    imlib_save_image(argv[1]);

	free(im);
	free(lock);
	free(disp);
	imlib_free_font();
	imlib_free_image_and_decache();

    return 0;
}
