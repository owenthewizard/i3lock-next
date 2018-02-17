#ifndef _SANITIZERS_H
#define _SANITIZERS_H

#include "i3lock-next.h"

#include <stdint.h>
#include <stdlib.h>

float get_gamma(const char*);

int8_t get_threshold(const char*);

Method get_distort(const char*);

uint8_t get_blur_iter(const char*);
uint8_t get_blur_strength(const char*);
uint8_t get_font_size(const char*);
uint8_t get_text_index(const char*);
uint8_t get_scale(const char*);

#endif
