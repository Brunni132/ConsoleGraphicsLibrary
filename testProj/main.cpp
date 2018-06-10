#include "ConsoleGraphics.h"

void writeTilemapForSnes(const char *destMapFile, const char *destTilesetFile, const char *destPaletteFile, Map *m, Tileset *t, Palette *p, PixelFormat *pf) {
	FILE *f = fopen(destPaletteFile, "wb");
	for (unsigned i = 0; i < 256; i++) {
		Pixel pix = p->getColor(i);
		unsigned short color = pix.r | pix.g << 5 | pix.b << 10;
		fwrite(&color, 2, 1, f);
	}
	fclose(f);

	// 4-bit format
	f = fopen(destTilesetFile, "wb");
	for (unsigned i = 0; i < t->numberOfTiles(); i++) {
		Tile *tile = t->getTile(i);
		// 2 first planes (16 bytes)
		for (unsigned k = 0; k < 8; k++) {
			unsigned char write1 = 0, write2 = 0;
			for (unsigned j = 0; j < 8; j++) {
				write1 |= (tile->getPixel(j, k).index & 1) << (7 - j);
				write2 |= ((tile->getPixel(j, k).index & 2) >> 1) << (7 - j);
			}
			fwrite(&write1, 1, 1, f);
			fwrite(&write2, 1, 1, f);
		}
		// 2 last planes (16 bytes)
		for (unsigned k = 0; k < 8; k++) {
			unsigned char write1 = 0, write2 = 0;
			for (int j = 0; j < 8; j++) {
				write1 |= ((tile->getPixel(j, k).index & 4) >> 2) << (7 - j);
				write2 |= ((tile->getPixel(j, k).index & 8) >> 3) << (7 - j);
			}
			fwrite(&write1, 1, 1, f);
			fwrite(&write2, 1, 1, f);
		}
	}
	fclose(f);
	
	f = fopen(destMapFile, "wb");
	for (unsigned j = 0; j < m->height; j++)
		for (unsigned i = 0; i < 256 / 8; i++) {
			unsigned short write = m->getBlock(i, j);
			fwrite(&write, 2, 1, f);
		}
	fclose(f);
}

int main(int argc, char *argv[]) {
	ref<Image> img = newref Image("img1.png");
	TilesetConversionData conversionParams(img);
	conversionParams.optimizeTileset = true;
	conversionParams.tileWidth = conversionParams.tileHeight = 8;
	conversionParams.pixelFormat.firstColorTransparent = true;
	conversionParams.pixelFormat.rmax = 31;
	conversionParams.pixelFormat.gmax = 31;
	conversionParams.pixelFormat.bmax = 31;
	conversionParams.pixelFormat.amax = 1;
	conversionParams.pixelFormat.mapBitsPaletteOffset = 10;
	conversionParams.pixelFormat.maxColors = 16;
	conversionParams.pixelFormat.maxPalettes = 4;
	conversionParams.pixelFormat.unfavorNewColorCreation = 50;
	doTilesetConversion(conversionParams);
	writeTilemapForSnes("snesmap.bin", "snestil.bin", "snespal.bin", conversionParams.outMap, conversionParams.outTileset, conversionParams.outPalette, &conversionParams.pixelFormat);
	mapWithMultipaletteAsImage(conversionParams.outMap, conversionParams.outTileset, conversionParams.outPalette, conversionParams.pixelFormat)->writeToFile("preview.png");
	return 0;
}
