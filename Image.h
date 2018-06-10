#ifndef IMAGE_H
#define IMAGE_H

class Image : public RefClass {
public:
	unsigned width, height;
	Pixel *pixelData;
	ref<Palette> associatedPalette;		// Means that the pixels are indexes actually; weak reference

	Image(const char *filename, Pixel transparentColor = 0);
	Image(unsigned width, unsigned height);
	// Constructs an indexed image, with a given palette
	Image(unsigned width, unsigned height, ref<Palette> p);
	Image(const Image &img);
	Image &operator = (const Image &img);
	~Image();

	bool isValid() const { return pixelData != 0; }
	bool isIndexed() const { return associatedPalette != 0; }

	Pixel getPixel(unsigned x, unsigned y) const {
		if (x < width && y < height)
			return pixelData[y * width + x];
		return 0;
	}
	
	void setPixel(unsigned x, unsigned y, Pixel color) const {
		if (x < width && y < height)
			pixelData[y * width + x] = color;
	}

	void drawTile(unsigned x, unsigned y, struct Tile *tile) const;
	// Applies a palette to this image, by trying to match the closest color to any palette entry for each pixel (slow, of course)
	// Works only once, then keeps the palette as associatedPalette
	void makeIndexed(ref<Palette> palette);
	// Works only if a palette has been set (makeIndexed), makes a copy of this image by using the same pixel format as the palette
	// Resulting image must be released by you
	ref<Image> makeTrueColorCopy() const;

	void writeToFile(const char *filename) const;
};

#endif
