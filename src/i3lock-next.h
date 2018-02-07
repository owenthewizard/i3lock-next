#ifndef _I3LOCK_NEXT_H
#define _I3LOCK_NEXT_H

/* #warning is not standard
#if __STDC_VERSION__ < 199901L && !defined IGNORE_C99
  #error "C standards prior to C99 might not have <stdint.h>"
  #error "Pass -DIGNORE_C99 to ignore this"
#endif
*/

#include <Imlib2.h>
#include <X11/Xlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef DEBUG
  #define D_PRINTF(fmt, ...) fprintf(stderr, "DEBUG: %s: %d: %s(): " fmt, \
                                     __FILE__, __LINE__, __func__,        \
                                     __VA_ARGS__);
#else
  #define D_PRINTF(fmt, ...) do{ } while(0)
#endif

#define FREE(ptr) if (ptr) { free(ptr); ptr = NULL; }

typedef enum {BLUR, PIXELATE, NONE} Method;

inline void die(const char *message, uint8_t code);
inline int get_monitor_count(Display *d, const Window w);
inline void get_distort(const char *distort, Method *m);
inline uint8_t get_scale(const char *scale_arg);
inline void get_blur_details(const char *blur_arg, const char *iter_arg, \
                             uint8_t *radius, uint8_t *iter);
void get_monitor_offsets(Display *d, const Window w, const int monitors, \
                         int16_t *offsets_x, int16_t *offsets_y,         \
                         const int lock_w, const int lock_h);

#endif
