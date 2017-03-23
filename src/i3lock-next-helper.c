#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <Imlib2.h>
#include <X11/extensions/Xrandr.h>

int main(int argc, char **argv)
{
    //We only take 2 arguments
    if (argc != 3)
    {
        fputs("i3lock-next-helper: error: this program takes exactly two arguments\n",stderr);
        return 1;
    }

    //Get $DISPLAY
    const char *display_name = getenv("DISPLAY");

    //Default to :0
    if (!display_name)
        display_name = ":0";

    //Attempt to open the display
    Display *disp = XOpenDisplay(display_name);
    if (!disp)
    {
        fprintf(stderr, "i3lock-next-helper: error: can't open display %s\n", display_name);
        return 1;
    }

    //Set Imlib2's context
    imlib_context_set_display(disp);
    imlib_context_set_visual(DefaultVisual(disp, DefaultScreen(disp)));
    imlib_context_set_colormap(DefaultColormap(disp, DefaultScreen(disp)));
    imlib_context_set_drawable(DefaultRootWindow(disp));

    //Get total width/height
    unsigned int width = DefaultScreenOfDisplay(disp)->width;
    unsigned int height = DefaultScreenOfDisplay(disp)->height;

    //Get width of each screen
    XRRScreenResources *screens = XRRGetScreenResources(disp, DefaultRootWindow(disp));
    XRRCrtcInfo *screen;
    unsigned int widths[screens->ncrtc];
    unsigned int heights[screens->ncrtc];
    for (int i = 0; i < screens->ncrtc; i++)
    {
        screen = XRRGetCrtcInfo(disp, screens, screens->crtcs[i]);
        //Check for rotation
        if (screen->rotation == RR_Rotate_90 || screen->rotation == RR_Rotate_270)
        {
            widths[i] = screen->height;
            heights[i] = screen-> width;
        }
        else
        {
            widths[i] = screen->width;
            heights[i] = screen->height;
        }
    }

    //Take a screenshot
    Imlib_Image *im = imlib_create_image_from_drawable(1, 0, 0, width, height, 1);
    if (!im)
    {
        fputs("i3lock-next-helper: error: couldn't grab image\n", stderr);
        return 1;
    }

    //Work on a new empty image
    imlib_context_set_image(imlib_create_image(width, height));

    //Load the font, given like so: /usr/share/fonts/open-sans/OpenSans-Regular/18
    imlib_context_set_font(imlib_load_font(argv[2]));
    if (!imlib_context_get_font())
    {
        fprintf(stderr, "i3lock-next-helper: error: couldn't find font %s\n", argv[2]);
        return 1;
    }

    //Setup some variables for offsetting the text (it needs to be centered)
    int offset_w, ignore_me;

    //Draw the text on our empty image and find out how many pixels we need to offset it by
    imlib_text_draw_with_return_metrics(0, 0, "Type password to unlock.", &offset_w, &ignore_me, &ignore_me, &ignore_me);

    //Work on the screenshot, discarding the old image
    imlib_context_set_image(im);

    //Setup some value to figure out wether we need to draw light or dark icons and text
    float values[screens->ncrtc];
    float ignore_me_2;

    //Crop the image to 300/300 (from center)
    imlib_context_set_image(imlib_create_cropped_image(widths[0]/2-150, heights[0]/2-150, 300, 300));

    //Scale the image to 3x3
    imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, 300, 300, 3, 3));

    //Grab the center pixel's value (doesn't work with 1x1 for some reason...)
    imlib_image_query_pixel_hsva(2, 2, &ignore_me_2, &ignore_me_2, &values[0], &ignore_me);

    //Discard modifications, back to the screenshot
    imlib_context_set_image(im);

    //Repeat the above for each monitor
    for (int i = 1; i < screens->ncrtc; i++)
    {
        imlib_context_set_image(imlib_create_cropped_image(widths[i]/2-150+widths[i-1], heights[i]/2-150+heights[i-1], 300, 300));
        imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, 300, 300, 3, 3));
        imlib_image_query_pixel_hsva(2, 2, &ignore_me_2, &ignore_me_2, &values[i], &ignore_me);
        imlib_context_set_image(im);
    }

    //Set up a color modifier
    imlib_context_set_color_modifier(imlib_create_color_modifier());

    //Darken the image to 60% brightness
    imlib_modify_color_modifier_gamma(0.6);
    imlib_apply_color_modifier();

    //Scale the image down to 20%
    imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, width, height, width/5, height/5));

    //Blur it a few times
    imlib_image_blur(1);
    imlib_image_blur(1);
    imlib_image_blur(1);

    //Scale it back up
    imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, width/5, height/5, width, height));

    //Save these changes
    im = imlib_context_get_image();

    //Prepare to load the lock image
    Imlib_Image *lock;

    //Draw the lock and text on the first monitor
    //TODO: GNU gettext
    if (values[0]*100 >= 60) //Rather light background
    {
        lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-dark.png");
        imlib_context_set_color(0, 0, 0, 255);
    }
    else //Rather dark background
    {
        lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-light.png");
        imlib_context_set_color(255, 255, 255, 255);
    }
    if (!lock)
    {
        fputs("i3lock-next-helper: error: couldn't load lock image\n", stderr);
        return 1;
    }
    imlib_text_draw(widths[0]/2-offset_w/2, heights[0]/1.5, "Type password to unlock.");
    imlib_blend_image_onto_image(lock, 0, 0, 0, 80, 80, widths[0]/2-40, heights[0]/2-40, 80, 80);

    //Draw the lock and text on the other monitors
    for (int i = 1; i < screens->ncrtc; i++)
    {
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
            fputs("i3lock-next-helper: error: couldn't load lock image\n", stderr);
            return 1;
        }
        if (widths[i] != 0) //0x0 monitors aren't real, so don't draw on them
        {
            imlib_text_draw(widths[i]/2-offset_w/2+widths[i-1], heights[i]/1.5, "Type password to unlock.");
            imlib_blend_image_onto_image(lock, 0, 0, 0, 80, 80, widths[i]/2-40+widths[i-1], heights[i]/2-40, 80, 80);
        }
    }

    //Save the image
    imlib_save_image(argv[1]);
}
// vim: set tabstop=4 softtabstop=4 shiftwidth=4 expandtab:
