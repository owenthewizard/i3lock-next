/*
 * config.h
 * Various settings for i3lock-next-helper.c
 *
 */

/* GAMMA_ADJUST
 * The amount to brighten or darken the image
 * Min 0.0
 * Greater than 1.0 is lighter, less than 1.0 is darker
 * Default 0.5
 *
 */
#define GAMMA_ADJUST 0.5

/* SCALE_FACTOR
 * Factor to scale the image by for faux-blur
 * Divides 1 to create a percentage
 * Default 5 (20%)
 *
 */
#define SCALE_FACTOR 5

/* BLUR_ITERATIONS
 * How many times to blur the image
 * Higher values take longer
 * Default 3
 *
 */
#define BLUR_ITERATIONS 3

/* BLUR_STRENGTH
 * How much blur is applied each iteration
 * Higher values take longer
 * Default 1
 *
 */
#define BLUR_STRENGTH 1

/* THRESHOLD
 * Threshold between light and dark backgrounds
 * If the average image value is greater than this, the dark lock image is used
 * If the average image value is less than this, the light lock image is used
 * Default 50
 *
 */
#define THRESHOLD 50
