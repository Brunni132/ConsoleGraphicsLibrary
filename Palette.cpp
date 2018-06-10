#include "ConsoleGraphics.h"

Palette::Palette(unsigned maxColors, bool firstColorTransparent) : maxColors(maxColors), currentColors(0) {
	colorData = new Pixel[maxColors];
	if (!colorData) {
		maxColors = 0;
	} else if (firstColorTransparent) {
		addColor(Pixel(0, 0, 0, 0));
	}
}

Palette::~Palette() {
	if (colorData)
		delete[] colorData;
}
