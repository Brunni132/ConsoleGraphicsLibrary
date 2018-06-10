#ifndef PALETTE_H
#define PALETTE_H

struct Palette : public RefClass {
	unsigned currentColors, maxColors;
	Pixel *colorData;

	Palette(unsigned maxColors, bool firstColorTransparent);
	~Palette();

	void addColor(Pixel color) {
		if (currentColors < maxColors)
			colorData[currentColors++] = color;
	}

	Pixel getColor(unsigned i) {
		if (i < currentColors)
			return colorData[i];
		return 0;
	}

	unsigned indexOfMostTransparentColor(bool &hasTransparency) {
		// Could be optimized
		unsigned minAlpha = colorData[0].a, indexOfTransparentColor = 0;
		unsigned firstAlpha = colorData[0].a;
		hasTransparency = false;
		for (unsigned k = 0; k < currentColors; k++) {
			unsigned a = colorData[k].a;
			if (a < minAlpha) {
				indexOfTransparentColor = k;
				minAlpha = a;
			}
			// If one color has an alpha different, or one color is 0, then we have transparency
			hasTransparency |= (a == 0 || firstAlpha != a);
		}
		return indexOfTransparentColor;
	}

	unsigned remaningColors() { return maxColors - currentColors; }
};

#endif
