#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <Imlib2.h>
#include "config.h"

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

    //Get screen width/height
    unsigned int width = DefaultScreenOfDisplay(disp)->width;
    unsigned int height = DefaultScreenOfDisplay(disp)->height;

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
    int offset_w, offset_h, ignore_me;
    //Draw the text on our empty image and find out how many pixels we need to offset it by
    imlib_text_draw_with_return_metrics(0, 0, "Type password to unlock.", &offset_w, &offset_h, &ignore_me, &ignore_me);

    //Set out screenshot as the working image
    imlib_context_set_image(im);

    //Crop it down to 100x100 from center
    imlib_context_set_image(imlib_create_cropped_image(width/2, height/2, 100, 100));

    //Scale it to 3x3, for some reason 1x1 will always have HSL of 0,0,0
    imlib_context_set_image(imlib_create_cropped_scaled_image(0, 0, 100, 100, 3, 3));

    //Setup some variables for the value of the center pixel
    float value, ignore_me_2;

    //Grab the center pixel's value
    imlib_image_query_pixel_hsva(2, 2, &ignore_me_2, &ignore_me_2, &value, &ignore_me);

    //Revert the image back to before we did any modifications
    imlib_context_set_image(im);

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

    //Prepare to load the lock image
    Imlib_Image *lock;

    if (value*100 >= 60) //We have a mostly light background
    {
        //Use dark lock image and black text
        lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-dark.png");
        imlib_context_set_color(0, 0, 0, 255);
    }
    else //We have a mostly dark background
    {
        //Use light lock image and white text
        lock = imlib_load_image(PREFIX"/share/i3lock-next/lock-light.png");
        imlib_context_set_color(255, 255, 255, 255);
    }

    if (!lock)
    {
        fputs("i3lock-next-helper: error: couldn't load lock image\n", stderr);
        return 1;
    }

    //Draw the text
    //TODO: GNU gettext
    imlib_text_draw(width/2-offset_w/2, height/1.5, "Type password to unlock.");

    //Slap on the lock image
    imlib_blend_image_onto_image(lock, 0, 0, 0, 80, 80, width/2-40, height/2-40, 80, 80);

    //Save the image
    imlib_save_image(argv[1]);
}
// vim: set tabstop=4 softtabstop=4 shiftwidth=4 expandtab:
