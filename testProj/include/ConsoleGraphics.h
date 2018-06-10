#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Ref.h"
#include "Pixel.h"
#include "Palette.h"
#include "Image.h"
#include "Map.h"
#include "Tileset.h"
#include "PlatformAware.h"
#include "Conversion.h"

#if !defined(MAX) && !defined(MIN)
	#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
	#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#endif
