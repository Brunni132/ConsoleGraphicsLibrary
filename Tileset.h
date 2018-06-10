#ifndef TILESET_H
#define TILESET_H

#include <vector>

struct Tile : public RefClass {
	unsigned width, height;
	Pixel *pixelData;

	Tile(ref<Image> source, unsigned offsetX, unsigned offsetY, unsigned width, unsigned height);
	~Tile();
	bool compareTo(ref<Tile> otherTile);
	Pixel getPixel(unsigned x, unsigned y) {
		if (x < width && y < height)
			return pixelData[width * y + x];
		return 0;
	}
	void setPixel(unsigned x, unsigned y, Pixel value) {
		if (x < width && y < height)
			pixelData[width * y + x] = value;
	}
};

class Tileset : public RefClass {
public:
	std::vector< ref<Tile> > tileData;

	Tileset();

	void addTile(ref<Tile> tile);
	ref<Tile> getTile(unsigned tileNo) const {
		if (tileNo < numberOfTiles())
			return tileData[tileNo];
		return 0;
	}
	unsigned numberOfTiles() const { return tileData.size(); }
};

#endif
