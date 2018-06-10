#include "ConsoleGraphics.h"

Image::Image(unsigned width, unsigned height) : width(width), height(height), associatedPalette(0) {
	pixelData = new Pixel[height * width];
	memset(pixelData, 0, width * height * sizeof(Pixel));
}

Image::Image(unsigned width, unsigned height, ref<Palette> p) : width(width), height(height), associatedPalette(p) {
	pixelData = new Pixel[height * width];
	memset(pixelData, 0, width * height * sizeof(Pixel));
}

void Image::makeIndexed(ref<Palette> pal) {
	if (associatedPalette) {
		printf("Image already has a palette, invalid call to makeIndexed\n");
		return;
	}

	// Any color as transparent or more transparent than that will use this index
	bool hasTransparency;
	unsigned indexOfTransparentColor = pal->indexOfMostTransparentColor(hasTransparency);
	unsigned minAlpha = pal->getColor(indexOfTransparentColor).a;
	// Avoid using transparency if none is available
	if (!hasTransparency)
		minAlpha = 0, indexOfTransparentColor = -1;

	// Apply to the image
	for (unsigned j = 0; j < height; j++)
		for (unsigned i = 0; i < width; i++) {
			Pixel p = getPixel(i, j);
			// If a pixel is transparent, any transparent color will do
			if (p.a <= minAlpha) {
				setPixel(i, j, indexOfTransparentColor);
			} else {
				unsigned bestDifference = ~0, bestDiffIndex = 0;
				// Trouve la meilleure couleur pour ce pixel
				for (unsigned k = 0; k < pal->currentColors; k++) {
					unsigned delta = p.colorDeltaWith(pal->getColor(k));
					// HACK: avoid using the transparent color for anything else than really transparent colors, this may not be adapted to some palettized images, but is fines for the GBA
					if (delta < bestDifference && k != indexOfTransparentColor) {
						bestDiffIndex = k;
						bestDifference = delta;
						// We won't find any better color
						if (delta == 0)
							break;
					}
				}
				// Then assign it
				setPixel(i, j, bestDiffIndex);
			}
		}
	associatedPalette = pal;
}

ref<Image> Image::makeTrueColorCopy() const {
	if (!associatedPalette) {
		printf("MakeTrueColorCopy can only be called on an indexed image\n");
		return 0;
	}

	ref<Image> result = newref Image(width, height);
	// Apply to the image
	for (unsigned j = 0; j < height; j++)
		for (unsigned i = 0; i < width; i++) {
			uint32_t p = getPixel(i, j).index;
			result->setPixel(i, j, associatedPalette->getColor(p));
		}
	return result;
}

void Image::drawTile(unsigned x, unsigned y, struct Tile *tile) const {
	Pixel *ptr = tile->pixelData;
	for (unsigned j = y; j < y + tile->height; j++)
		for (unsigned i = x; i < x + tile->width; i++)
			setPixel(i, j, *ptr++);
}

Image::~Image() {
	if (pixelData)
		delete[] pixelData;
}
