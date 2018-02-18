#ifndef _I3LOCK_NEXT_H
#define _I3LOCK_NEXT_H

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
  #define D_PRINTF(fmt, ...) fprintf(stderr, "DEBUG:%s:%d: %s(): " fmt, \
                                     __FILE__, __LINE__, __func__,        \
                                     __VA_ARGS__);
  #define DEBUG_SYSTEM(x) system("true");
#else
  #define D_PRINTF(fmt, ...) do{ } while(0)
  #define DEBUG_SYSTEM(x) system(x);
#endif

#define FREE(ptr) if (ptr) { free(ptr); ptr = NULL; }

typedef enum {BLUR, PIXELATE, NONE} Method;

#endif
