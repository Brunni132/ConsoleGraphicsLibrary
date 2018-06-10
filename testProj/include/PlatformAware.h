#ifndef PLATFORMAWARE_H
#define PLATFORMAWARE_H

struct PixelFormat {
	// Defines the format of the pixel, or palette entry if indexed = true
	uint8_t rmax, gmax, bmax, amax;
	// Palette stuff
	bool firstColorTransparent;
	unsigned maxColors;		// Max colors in one palette (zero means no palette to be used, not indexed)
	// Should be set to 1. If set to >1, generates maxPalettes * maxColors total colors, associating one of the small palettes (of maxColors) to each tile
	// Can be use for wonderswan, GBA in 4-bit mode (where you have 16 palettes of 16 colors)
	// Works only for tilesets and when generating a map, extremely slow
	unsigned maxPalettes;
	// If maxPalettes > 1, the map will be filled with the palette number too, starting from this bit number
	uint8_t mapBitsPaletteOffset;
	// Used if maxPalettes > 1. -1: unfavors creating new colors (usually bad), 0: normal render (neutral), -2: computes for the whole image (usually gives the best result, but ultra slow), 1-2500: gives a more colorful result but can appear blocky
	unsigned unfavorNewColorCreation;
	// Used if maxPalettes > 1, useful if unfavorNewColorCreation > 0 (should be set to true to fill the palettes as much as possible; slower though)
	bool postProcessingCompletePalettes;
	
	PixelFormat() : rmax(255), gmax(255), bmax(255), amax(255),
		firstColorTransparent(false), maxColors(0), maxPalettes(1),
		mapBitsPaletteOffset(24), unfavorNewColorCreation(1000), postProcessingCompletePalettes(true) {
		// Call prepareForUse before!
	}

	bool isIndexed() { return maxColors != 0; }
	uint8_t opaqueAlphaValue() { return amax; }
	void prepareForUse() {}

	// Posterize a pixel from the framebuffer (at destination of the framebuffer)
	Pixel deflate(Pixel p) {
		p.r = (uint8_t) (p.r * (rmax + 0.99f) / 255.0f);
		p.g = (uint8_t) (p.g * (gmax + 0.99f) / 255.0f);
		p.b = (uint8_t) (p.b * (bmax + 0.99f) / 255.0f);
		p.a = (uint8_t) (p.a * (amax + 0.99f) / 255.0f);
		return p;
	}
	// Referses the effect of deflate, for a pixel of the framebuffer at destination of the framebuffer
	Pixel inflate(Pixel p) {
		p.r = (uint8_t) (p.r * 255.0f / rmax);
		p.g = (uint8_t) (p.g * 255.0f / gmax);
		p.b = (uint8_t) (p.b * 255.0f / bmax);
		p.a = (uint8_t) (p.a * 255.0f / amax);
		return p;
	}
	// Transform a pixel from the framebuffer into the target machine format
	Pixel pack(Pixel p) {
		return p;
	}
	// Read a pixel from the target machine format to the framebuffer
	Pixel unpack(Pixel p) {
		return p;
	}

private:
};

// Takes in account transparent color and also quantizes
void deflateSourceImage(ref<Image> img, Pixel transparentColor, PixelFormat &destPixelFormat);
// To be called only if the result image is to be written for reference
void inflateSourceImage(ref<Image> img, PixelFormat &destPixelFormat);

#endif
