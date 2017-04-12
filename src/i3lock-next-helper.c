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
	const char *prompt = NULL;
	if (argc != 4)
		prompt  =  "";
	else
		prompt  = argv[3];


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

	// declare a variable for offsetting the text
	int offset_w = 0;

	// dummy varibles for information we don't need
	int dum = 0;
	float dum2 = 0;

	// setup some array of values to figure out whether we need to draw
	// light or dark icons and text
	float values[n];

	// draw the text on our empty image and find out how many pixels we
	// need to offset it by
	imlib_text_draw_with_return_metrics(0, 0, prompt, &offset_w, &dum, &dum, &dum);

	// discard old image and change context to screenshot
	imlib_free_image_and_decache();
	imlib_context_set_image(im);

	// define variables for and calculate top left and right corner
	// coordinates of the crop region
	int tl = widths[0] / 2 - 150;
	int tr = heights[0] / 2 - 150;

	// crop to 300x300 square at the centre of the monitor
	// and change context
	Imlib_Image *c = imlib_create_cropped_image(tl, tr, 300, 300);
	imlib_context_set_image(c);

	// crop the image to 300x300 (around center)
	imlib_context_set_image(imlib_create_cropped_image(tl, tr, 300, 300));

	// rescale to 3x3 and change context
	c = imlib_create_cropped_scaled_image(0, 0, 300, 300, 3, 3);
	imlib_context_set_image(c);

	// grab the centre pixel value (doesn't work with 1x1 for some reason)
	imlib_image_query_pixel_hsva(2, 2, &dum2, &dum2, &values[0], &dum);

	// discard modifications and change context back to whole screen
	imlib_free_image_and_decache();
	imlib_context_set_image(im);

    // repeat the above for each monitor
	for (int i = 1; i < n; i++)
	{
		// calculate top left and right corner coordinates
		tl = widths[i] / 2 - 150 + widths[i - 1];
		tr = heights[i] / 2 - 150 + heights[i - 1];

		// crop to 300x300 square at the centre of monitor i
		// and change context
		Imlib_Image *c = imlib_create_cropped_image(tl, tr, 300, 300);
		imlib_context_set_image(c);

		// rescale to 3x3 and change context
		c = imlib_create_cropped_scaled_image(0, 0, 300, 300, 3, 3);
		imlib_context_set_image(c);

		// grab centre pixel value
		imlib_image_query_pixel_hsva(2, 2, &dum2, &dum2, &values[i], &dum);

		// discard modifications and chage context back to whole screen
		imlib_free_image_and_decache();
		imlib_context_set_image(im);
    }

	// set up a color modifier
	imlib_context_set_color_modifier(imlib_create_color_modifier());

	// darken the image to 60% brightness
	imlib_modify_color_modifier_gamma(0.6);
	imlib_apply_color_modifier();
	imlib_free_color_modifier();

	// scale the image down to 20% and change context
	Imlib_Image *s = imlib_create_cropped_scaled_image(0, 0, dw, dh, dw/5, dh/5);
	imlib_context_set_image(s);

    // blur it a few times
	imlib_image_blur(1);
	imlib_image_blur(1);
	imlib_image_blur(1);

	// scale it back up and change context
	s = imlib_create_cropped_scaled_image(0, 0, dw/5, dh/5, dw, dh);
	imlib_context_set_image(s);

	// prepare to load the lock image
	Imlib_Image *lock = NULL;

	// determine which lock image to load for the first monitor
	if (values[0] * 100 >= 50)
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
		if (values[i]*100 >= 50)
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
