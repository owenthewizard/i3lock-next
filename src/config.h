/*
 * config.h
 * Various settings for i3lock-next.c
 *
 */

/* DEFAULT_GAMMA
 * The amount to brighten or darken the image
 * Min 0.0
 * Greater than 1.0 is lighter, less than 1.0 is darker
 * Default 0.5
 *
 */
#define DEFAULT_GAMMA 0.5

/* DEFAULT_SCALE
 * Factor to scale the image by for faux-blur
 * Divides 1 to create a percentage
 * Default 5 (20%)
 *
 */
#define DEFAULT_SCALE 5

/* DEFAULT_RADIUS
 * Radius for gaussian blur
 * Higher values take longer
 * Default 3.0
 *
 */
#define DEFAULT_RADIUS 3.0

/* DEFAULT_SIGMA
 * Sigma for guassian blur
 * Higher values take longer
 * Default 1.5
 *
 */
#define DEFAULT_SIGMA 1.5

/* DEFAULT_THRESH
 * Threshold between light and dark backgrounds
 * If the average image lightness is greater than this, the dark lock image is
 * used
 * If the average image lightness is less than this, the light lock image is
 * used
 * Default 50
 *
 */
#define DEFAULT_THRESH 50

/* DEFAULT_METHOD
 * Method for distortion of image
 * Must be BLUR, PIXELATE, or NONE
 * Default BLUR
 *
 */
#define DEFAULT_METHOD BLUR

/* DEFAULT_LOCK_LIGHT
 * Path to lock image for dark backgrounds
 * Default PREFIX/share/i3lock-next/lock-light.png
 *
 */
#define DEFAULT_LOCK_LIGHT PREFIX"/share/i3lock-next/lock-light.png"

/* DEFAULT_LOCK_DARK
 * Path to lock image for light backgrounds
 * Default PREFIX/share/i3lock-next/lock-dark.png
 *
 */
#define DEFAULT_LOCK_DARK PREFIX"/share/i3lock-next/lock-dark.png"
