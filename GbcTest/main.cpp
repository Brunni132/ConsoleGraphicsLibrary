#include "ConsoleGraphics.h"

#define RGB(r, g, b) ((((uint16_t)(b) & 0x1f) << 10) | (((uint16_t)(g) & 0x1f) << 5) | (((uint16_t)(r) & 0x1f) << 0))

FILE *outData, *outHeader;

void setConversionParams(TilesetConversionData& conversionParams, bool isBg, bool use8x16, unsigned maxPalettes = 1) {
	conversionParams.optimizeTileset = isBg;
	conversionParams.tileWidth = 8;
	conversionParams.tileHeight = use8x16 ? 16 : 8;
	conversionParams.pixelFormat.mapBitsPaletteOffset = 9;
	conversionParams.pixelFormat.firstColorTransparent = !isBg;
	conversionParams.pixelFormat.rmax = 31;
	conversionParams.pixelFormat.gmax = 31;
	conversionParams.pixelFormat.bmax = 31;
	conversionParams.pixelFormat.amax = 1;
	conversionParams.pixelFormat.maxColors = 4;
	conversionParams.pixelFormat.maxPalettes = maxPalettes;
}

unsigned smallestInMap(ref<Map> m) {
	unsigned smallest = ~0;
	for (unsigned j = 0; j < m->height; j++) {
		for (unsigned i = 0; i < m->width; i++) {
			MapBlock block = m->getBlock(i, j);
			if (block >= 0) smallest = MIN(smallest, block);
		}
	}
	return smallest;
}

void writeTilesetForGbc(const char* fileName, FILE* outData, FILE* outHeader, TilesetConversionData conversionParams, ref<Tileset> t) {
	fprintf(outData, "#define %s_chr_tiles %d\n", fileName, t->numberOfTiles());
	fprintf(outData, "const uint16_t %s_chr[%d * 8] = {\n", fileName, t->numberOfTiles());
	fprintf(outHeader, "extern const uint16_t %s_chr[%d * 8];\n", fileName, t->numberOfTiles());
	for (unsigned i = 0; i < t->numberOfTiles(); i++) {
		Tile* tile = t->getTile(i);
		for (unsigned k = 0; k < tile->height; k++) {
			uint16_t data = 0;
			for (unsigned j = 0; j < tile->width; j++) {
				int pixel = tile->getPixel(tile->width - j - 1, k).index;
				data |= (pixel & 1) << j;
				data |= (pixel & 2) << (7 + j);
			}
			fprintf(outData, "0x%x, ", data);
		}
		fprintf(outData, "\n");
	}
	fprintf(outData, "};\n\n");
}

void writePaletteForGbc(const char *fileName, FILE *outData, FILE *outHeader, TilesetConversionData conversionParams, ref<Palette> p) {
	fprintf(outData, "#define %s_palette_count %d\n", fileName, (int)ceil(double(p->currentColors) / conversionParams.pixelFormat.maxColors));
	fprintf(outData, "const uint16_t %s_palette[%d] = {\n", fileName, p->currentColors);
	fprintf(outHeader, "extern const uint16_t %s_palette[%d];\n", fileName, p->currentColors);
	for (unsigned i = 0; i < p->currentColors; i++) {
		Pixel pix = p->getColor(i);
		fprintf(outData, "0x%x, ", RGB(pix.r, pix.g, pix.b));
	}
	fprintf(outData, "\n};\n\n");
}

void convertLevelMap(const char *fileName, const char *outputImageForDebug = nullptr) {
	char imageName[256], tilesetName[256];

	sprintf(imageName, "%s.png", fileName);
	sprintf(tilesetName, "%s.tmx", fileName);

	TilesetConversionData conversionParams(newref Image(imageName, Pixel(255, 0, 255, 255)));
	setConversionParams(conversionParams, true, false, 6);
	doTilesetConversion(conversionParams);

	ref<Palette> p = conversionParams.outPalette;
	ref<Tileset> t = conversionParams.outTileset;
	ref<Map> m = conversionParams.outMap;

	// Writing map
	ref<Map> existingMap = readMapFromTiledCsvProjectFile(tilesetName, "bg");
	if (!existingMap) {
		createTiledProjectFile(tilesetName, conversionParams);
	}
	else {
		m = existingMap;
	}

	ref<Map> collisionMap = readMapFromTiledCsvProjectFile(tilesetName, "collisions");
	if (collisionMap) {
		unsigned smallest = smallestInMap(collisionMap);
		fprintf(outData, "const uint8_t %s_collision[%d * %d] = {\n", fileName, collisionMap->width, collisionMap->height);
		fprintf(outHeader, "extern const uint8_t %s_collision[%d * %d];\n", fileName, collisionMap->width, collisionMap->height);
		for (unsigned j = 0; j < collisionMap->height; j++) {
			for (unsigned i = 0; i < collisionMap->width; i++) {
				MapBlock tiledBlock = MAX(0, collisionMap->getBlock(i, j) - smallest);
				fprintf(outData, "%d, ", tiledBlock & 0xff);
			}
			fprintf(outData, "\n");
		}
		fprintf(outData, "};\n\n");
	}

	fprintf(outData, "#define %s_map_width %d\n", fileName, m->width);
	fprintf(outData, "#define %s_map_height %d\n", fileName, m->height);
	fprintf(outData, "const uint8_t %s_tiles[%d * %d] = {\n", fileName, m->width, m->height);
	fprintf(outHeader, "extern const uint8_t %s_tiles[%d * %d];\n", fileName, m->width, m->height);
	for (unsigned j = 0; j < m->height; j++) {
		for (unsigned i = 0; i < m->width; i++) {
			MapBlock tiledBlock = m->getBlock(i, j);
			fprintf(outData, "%d, ", tiledBlock & 0xff);
		}
		fprintf(outData, "\n");
	}
	fprintf(outData, "};\n\n");

	fprintf(outData, "const uint8_t %s_attr[%d * %d] = {\n", fileName, m->width, m->height);
	fprintf(outHeader, "extern const uint8_t %s_attr[%d * %d];\n", fileName, m->width, m->height);
	for (unsigned j = 0; j < m->height; j++) {
		for (unsigned i = 0; i < m->width; i++) {
			MapBlock tiledBlock = m->getBlock(i, j);
			unsigned short write = (unsigned short)tiledBlock & 0xffff;
			fprintf(outData, "%d, ", write >> 9);
		}
		fprintf(outData, "\n");
	}
	fprintf(outData, "};\n\n");

	// Writing tileset
	writeTilesetForGbc(fileName, outData, outHeader, conversionParams, t);

	// Writing the palette
	writePaletteForGbc(fileName, outData, outHeader, conversionParams, p);

	if (outputImageForDebug) mapWithMultipaletteAsImage(m, t, p, conversionParams.pixelFormat)->writeToFile(outputImageForDebug);
}

class SpriteBuilder {
	ref<Tileset> tileset;
public:
	SpriteBuilder() : tileset(null) {}

	SpriteBuilder& convertSprite(const char* imageName, unsigned tileHeight = 8) {
		TilesetConversionData conversionParams(newref Image(imageName, Pixel(255, 0, 255, 255)));
		conversionParams.outTileset = tileset;
		conversionParams.tileHeight = tileHeight;
		doTilesetConversion(conversionParams);
		tileset = conversionParams.outTileset;
		return *this;
	}

	void writeSprites(const char* destBinFilePrefix) {
		ref<Image> imageToConvert = tilesetAsImage(tileset, null, PixelFormat(), 8);
		TilesetConversionData conversionParams(imageToConvert);
		setConversionParams(conversionParams, false, false, 1);
		doTilesetConversion(conversionParams);
		// Do not write a map
		conversionParams.outMap = nullptr;
		writeTilesetForGbc(destBinFilePrefix, outData, outHeader, conversionParams, conversionParams.outTileset);
		writePaletteForGbc(destBinFilePrefix, outData, outHeader, conversionParams, conversionParams.outPalette);
	}
};

int main(int argc, char* argv[]) {
	outData = fopen("out/gfx.c", "w");
	outHeader = fopen("out/gfx.h", "w");

	convertLevelMap("level1");
	SpriteBuilder()
		.convertSprite("mainchar.png", 16)
		.writeSprites("sprites");

	SpriteBuilder()
		.convertSprite("bar.png")
		.writeSprites("window");

	fclose(outData);
	fclose(outHeader);

	return 0;
}


//int startAddr = 0x4000;
//FILE *outDefs, *outDefsC, *outBin;
//
//class Writer {
//	const char *prefix, *type;
//	uint8_t data[256 << 10];
//	int bytes;
//
//public:
//	Writer() : prefix(0), type(0), bytes(0) {}
//	~Writer() { writePending(); }
//	void prepareFor(const char *prefix, const char *type) {
//		writePending();
//		this->prefix = prefix;
//		this->type = type;
//	}
//
//	void writeByte(int byte) {
//		if (bytes < sizeof(data))
//			data[bytes++] = bytes;
//	}
//
//	void writePending() {
//		if (!bytes)
//			return;
//		// Align to 16k for SMS
//		if ((startAddr & ~16383) != ((startAddr + bytes) & ~16383))
//			startAddr = (startAddr & ~16383) + 16384;
//		fprintf(outDefs, "DEFC %s_%s = %d\n", prefix, type, startAddr);
//		fprintf(outDefs, "DEFC %s_%s_size = %d\n", prefix, type, startAddr + bytes);
//		fprintf(outDefsC, "#define %s_%s %d\n", prefix, type, startAddr);
//		fprintf(outDefsC, "#define %s_%s_size %d\n", prefix, type, startAddr + bytes);
//		fwrite(data, 1, bytes, outBin);
//	}
//};
//
//// In the case of the BG, use 9 bits colors and discard transparent color
//// In the case of sprites, use 6 bits colors plus the transparent color
//void setConversionParams(TilesetConversionData &conversionParams, bool isBg, bool use9bit, bool use8x16) {
//	conversionParams.optimizeTileset = isBg;
//	conversionParams.tileWidth = 8;
//	conversionParams.tileHeight = use8x16 ? 16 : 8;
//	conversionParams.pixelFormat.firstColorTransparent = true /*!isBg*/;
//	conversionParams.pixelFormat.rmax = use9bit ? 6 : 3;
//	conversionParams.pixelFormat.gmax = use9bit ? 6 : 3;
//	conversionParams.pixelFormat.bmax = use9bit ? 6 : 3;
//	conversionParams.pixelFormat.amax = 1;
//	conversionParams.pixelFormat.maxColors = 16;
//	conversionParams.pixelFormat.maxPalettes = 1;
//}
//
//void writeTilemapForSms(const char *destBinFilePrefix, TilesetConversionData &data, bool generateTwoPalettes, bool writeMapVertically = false) {
//	ref<Palette> p = data.outPalette;
//	ref<Tileset> t = data.outTileset;
//	ref<Map> m = data.outMap;
//	Writer writer;
//
//	if (!generateTwoPalettes) {
//		writer.prepareFor(destBinFilePrefix, "pal");
//		for (unsigned i = 0; i < p->currentColors; i++) {
//			Pixel pix = p->getColor(i);
//			uint8_t value = pix.r | pix.g << 2 | pix.b << 4;
//			writer.writeByte(value);
//			startAddr++;
//		}
//	}
//	else {
//		// Output 2 palettes to use by swapping quickly (to give a "blink" effect)
//		writer.prepareFor(destBinFilePrefix, "pal_lo");
//		for (unsigned i = 0; i < p->currentColors; i++) {
//			Pixel pix = p->getColor(i);
//			uint8_t value = (pix.r/2) | (pix.g/2) << 2 | (pix.b/2) << 4;
//			writer.writeByte(value);
//			startAddr++;
//		}
//
//		writer.prepareFor(destBinFilePrefix, "pal_hi");
//		for (unsigned i = 0; i < p->currentColors; i++) {
//			Pixel pix = p->getColor(i);
//			uint8_t value = ((pix.r+1)/2) | ((pix.g+1)/2) << 2 | ((pix.b+1)/2) << 4;
//			writer.writeByte(value);
//			startAddr++;
//		}
//	}
//
//	// Write the tileset
//	writer.prepareFor(destBinFilePrefix, "til");
//	for (unsigned i = 0; i < t->numberOfTiles(); i++) {
//		Tile *tile = t->getTile(i);
//		for (unsigned k = 0; k < tile->height; k++) {
//			int pix0 = 0, pix1 = 0, pix2 = 0, pix3 =0;
//			for (unsigned j = 0; j < tile->width; j++) {
//				int pixel = tile->getPixel(j, k).index;
//				pix0 |= (pixel & 1) << (7 - j);
//				pix1 |= (pixel >> 1 & 1) << (7 - j);
//				pix2 |= (pixel >> 2 & 1) << (7 - j);
//				pix3 |= (pixel >> 3 & 1) << (7 - j);
//			}
//			writer.writeByte(pix0);
//			writer.writeByte(pix1);
//			writer.writeByte(pix2);
//			writer.writeByte(pix3);
//			startAddr += 4;
//		}
//	}
//
//	if (m) {
//		// Here, we want two things: either we generate an associated TMX if it doesn't exist, or we convert it to a map
//		char tmxFileName[512];
//		sprintf(tmxFileName, "out/%s_tiled.tmx", destBinFilePrefix);
//		ref<Map> existingMap = readMapFromTiledCsvProjectFile(tmxFileName);
//		if (!existingMap) {
//			createTiledProjectFile(tmxFileName, data);
//		} else {
//			m = existingMap;
//		}
//
//		// Write the map anyway
//		writer.prepareFor(destBinFilePrefix, "map");
//		if (!writeMapVertically) {
//			for (unsigned j = 0; j < m->height; j++) {
//				for (unsigned i = 0; i < 256 / 8; i++) {
//					MapBlock tiledBlock = m->getBlock(i, j);
//					unsigned short write = (unsigned short) tiledBlock & 0xffff;
//					// Tiled supports tile flipping, so convert it to SMS values
//					if (tiledBlock & 1 << 31)
//						write |= 1 << 9;
//					if (tiledBlock & 1 << 30)
//						write |= 1 << 10;
//					uint8_t b1 = write & 0xff, b2 = write >> 8;
//					writer.writeByte(b1);
//					writer.writeByte(b2);
//					startAddr += 2;
//				}
//			}
//		} else {
//			for (unsigned i = 0; i < m->width; i++) {
//				for (unsigned j = 0; j < m->height; j++) {
//					MapBlock tiledBlock = m->getBlock(i, j);
//					unsigned short write = (unsigned short) tiledBlock & 0xffff;
//					// Tiled supports tile flipping, so convert it to SMS values
//					if (tiledBlock & 1 << 31)
//						write |= 1 << 9;
//					if (tiledBlock & 1 << 30)
//						write |= 1 << 10;
//					uint8_t b1 = write & 0xff, b2 = write >> 8;
//					writer.writeByte(b1);
//					writer.writeByte(b2);
//					startAddr += 2;
//				}
//			}
//		}
//	}
//}
//
//
//void convertBg(const char *imageName, const char *destBinFilePrefix, bool use9bit = false) {
//	TilesetConversionData conversionParams(newref Image(imageName, Pixel(255, 0, 255, 255)));
//	setConversionParams(conversionParams, true, use9bit, false);
//	doTilesetConversion(conversionParams);
//	writeTilemapForSms(destBinFilePrefix, conversionParams, use9bit, true);
//}
//
//int main(int argc, char *argv[]) {
//	outDefs = fopen("out/gfx.inc", "w");
//	outDefsC = fopen("out/gfx.h", "w");
//	outBin = fopen("out/gfx.bin", "wb");
//	if (!outDefs) {
//		fprintf(stderr, "Could not write to output directory, check your rights.\n");
//		return -1;
//	}
//	convertBg("level.png", "levelbg", false);
//	SpriteBuilder()
//		.convertSprite("player8x8.png")
//		.convertSprite("player16x16.png")
//		.convertSprite("player1_32x32.png")
//		.convertSprite("player2_32x32.png")
//		.writeSprites("sprites");
//	fclose(outDefs);
//	fclose(outBin);
//	return 0;
//}
