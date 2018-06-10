#include "ConsoleGraphics.h"

Map::Map(unsigned width, unsigned height) : width(width), height(height) {
	mapData = new MapBlock[width * height];
	if (!mapData)
		width = height = 0;
}

Map::~Map() {
	if (mapData)
		delete[] mapData;
}
