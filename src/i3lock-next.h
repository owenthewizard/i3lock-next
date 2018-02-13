#ifndef _I3LOCK_NEXT_H
#define _I3LOCK_NEXT_H

/* #warning is not standard
#if __STDC_VERSION__ < 199901L && !defined IGNORE_C99
  #error "C standards prior to C99 might not have <stdint.h>"
  #error "Pass -DIGNORE_C99 to ignore this"
#endif
*/

#include <fontconfig/fontconfig.h>
#include <Imlib2.h>
#include <X11/Xlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef DEBUG
  #include <assert.h>
  #define D_PRINTF(fmt, ...) fprintf(stderr, "DEBUG: %s: %d: %s(): " fmt, \
                                     __FILE__, __LINE__, __func__,        \
                                     __VA_ARGS__);
#else
  #define D_PRINTF(fmt, ...) do{ } while(0)
  #define assert(x) do{ } while(0)
#endif

#define FREE(ptr) if (ptr) { free(ptr); ptr = NULL; }

typedef enum {BLUR, PIXELATE, NONE} Method;

inline void die(const char*, uint8_t);

inline int get_monitor_count(Display*, const Window);

inline void get_distort(const char*, Method*);

inline uint8_t get_scale(const char*);

inline void get_blur_details(const char*, const char*, uint8_t*, uint8_t*);

void get_monitor_offsets(Display*, const Window, const int,
                         int16_t*, int16_t*);

inline void add_fonts_to_imlib_context(FcConfig*);

inline char *get_font_file(FcConfig*, const char*);

inline float get_gamma(const char*);

inline int8_t get_threshold(const char*);

void warn_errno(const char*);

inline void draw_lock_icons(const char*, Imlib_Image*,
                            const int, Display*, const Window,
                            const int, const int,
                            Imlib_Image*,
                            Imlib_Image*, int16_t*,
                            int16_t*);

#endif
