#ifndef MAP_H
#define MAP_H

typedef int32_t MapBlock;

class Map : public RefClass {
public:
	unsigned width, height;
	MapBlock *mapData;

	Map(unsigned width, unsigned height);
	~Map();

	MapBlock getBlock(unsigned x, unsigned y) const {
		if (x < width && y < height)
			return mapData[y * width + x];
		return -1;
	}
	
	void setBlock(unsigned x, unsigned y, MapBlock block) const {
		if (x < width && y < height)
			mapData[y * width + x] = block;
	}
};

#endif
