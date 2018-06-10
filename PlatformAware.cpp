#include "ConsoleGraphics.h"

void deflateSourceImage(ref<Image> img, Pixel transparentColor, PixelFormat &destPixelFormat) {
	for (unsigned j = 0; j < img->height; j++)
		for (unsigned i = 0; i < img->width; i++) {
			// Check for full transparency
			Pixel p = img->getPixel(i, j);
			if (p.value == transparentColor.value) {
				img->setPixel(i, j, 0);
				continue;
			}
			// Else quantize to the appropriate number of bits
			// We'll use the proper bit order when writing the palette/bitmap
			img->setPixel(i, j, destPixelFormat.deflate(p));
		}
}

void inflateSourceImage(ref<Image> img, PixelFormat &destPixelFormat) {
	for (unsigned j = 0; j < img->height; j++)
		for (unsigned i = 0; i < img->width; i++) {
			Pixel p = img->getPixel(i, j);
			img->setPixel(i, j, destPixelFormat.inflate(p));
		}
}
