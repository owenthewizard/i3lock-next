#ifndef _PROCESSING_H
#define _PROCESSING_H

#include "i3lock-next.h"
#include "i3lock-next.yucc"

#include <fontconfig/fontconfig.h>
#include <Imlib2.h>
#include <X11/Xlib.h>

#include <stdint.h>
#include <stdlib.h>

void adjust_gamma(Imlib_Image*, const char*);
void distort(Imlib_Image*, const Method, const yuck_t*);
void draw_lock_icons(Imlib_Image*, Imlib_Image*, Imlib_Image*, const int*,
                     const int*, const int8_t, const int, int*, int*);
void draw_text(Imlib_Image*, const char*, const char*, const int, const int,
               const int, const int*, const int*);
void set_imlib2_context(Display*, const Window, const int, const char*,
                        const char*);

#endif
