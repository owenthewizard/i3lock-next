/*
 * image-size.c
 * This program prints the width and height of an image
 * Example 1080p image output: 1920x1080
 * NOTE: should not be run directly by user
 *
 */

/* Standard */
#include <stdlib.h>
#include <stdio.h>

/* Imlib2 */
#include <Imlib2.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, const char **argv)
{
    if (argv[1] == NULL)
        return 1;

    Imlib_Image image = imlib_load_image(argv[1]);
    if (image)
    {
        imlib_context_set_image(image);
        printf("%dx%d", imlib_image_get_width(), imlib_image_get_height());
        imlib_free_image_and_decache();
    }
    else
        return 2;
}
