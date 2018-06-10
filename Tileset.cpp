#include "ConsoleGraphics.h"

Tileset::Tileset() {
}

void Tileset::addTile(ref<Tile> tile) {
	tileData.push_back(tile);
}

Tile::Tile(ref<Image> source, unsigned offsetX, unsigned offsetY, unsigned width, unsigned height)
	: width(width), height(height) {
	pixelData = new Pixel[width * height];
	if (pixelData) {
		Pixel *ptr = pixelData;
		for (unsigned j = offsetY; j < offsetY + height; j++)
			for (unsigned i = offsetX; i < offsetX + width; i++)
				*ptr++ = source->getPixel(i, j);
	} else {
		width = height = 0;
	}
}

bool Tile::compareTo(ref<Tile> otherTile) {
	if (otherTile->width == width && otherTile->height == height &&
		!memcmp(otherTile->pixelData, pixelData, sizeof(Pixel) * width * height))
		return true;
	return false;
}

Tile::~Tile() {
	if (pixelData)
		delete[] pixelData;
}
