#include "ConsoleGraphics.h"

int startAddr = 0x4000;
FILE *outDefs, *outDefsC, *outBin;

class Writer {
	const char *prefix, *type;
	uint8_t data[256 << 10];
	int bytes;

public:
	Writer() : prefix(0), type(0), bytes(0) {}
	~Writer() { writePending(); }
	void prepareFor(const char *prefix, const char *type) {
		writePending();
		this->prefix = prefix;
		this->type = type;
	}

	void writeByte(int byte) {
		if (bytes < sizeof(data))
			data[bytes++] = bytes;
	}

	void writePending() {
		if (!bytes)
			return;
		// Align to 16k for SMS
		if ((startAddr & ~16383) != ((startAddr + bytes) & ~16383))
			startAddr = (startAddr & ~16383) + 16384;
		fprintf(outDefs, "DEFC %s_%s = %d\n", prefix, type, startAddr);
		fprintf(outDefs, "DEFC %s_%s_size = %d\n", prefix, type, startAddr + bytes);
		fprintf(outDefsC, "#define %s_%s %d\n", prefix, type, startAddr);
		fprintf(outDefsC, "#define %s_%s_size %d\n", prefix, type, startAddr + bytes);
		fwrite(data, 1, bytes, outBin);
	}
};

// In the case of the BG, use 9 bits colors and discard transparent color
// In the case of sprites, use 6 bits colors plus the transparent color
void setConversionParams(TilesetConversionData &conversionParams, bool isBg, bool use9bit, bool use8x16) {
	conversionParams.optimizeTileset = isBg;
	conversionParams.tileWidth = 8;
	conversionParams.tileHeight = use8x16 ? 16 : 8;
	conversionParams.pixelFormat.firstColorTransparent = true /*!isBg*/;
	conversionParams.pixelFormat.rmax = use9bit ? 6 : 3;
	conversionParams.pixelFormat.gmax = use9bit ? 6 : 3;
	conversionParams.pixelFormat.bmax = use9bit ? 6 : 3;
	conversionParams.pixelFormat.amax = 1;
	conversionParams.pixelFormat.maxColors = 16;
	conversionParams.pixelFormat.maxPalettes = 1;
}

void writeTilemapForSms(const char *destBinFilePrefix, TilesetConversionData &data, bool generateTwoPalettes, bool writeMapVertically = false) {
	ref<Palette> p = data.outPalette;
	ref<Tileset> t = data.outTileset;
	ref<Map> m = data.outMap;
	Writer writer;

	if (!generateTwoPalettes) {
		writer.prepareFor(destBinFilePrefix, "pal");
		for (unsigned i = 0; i < p->currentColors; i++) {
			Pixel pix = p->getColor(i);
			uint8_t value = pix.r | pix.g << 2 | pix.b << 4;
			writer.writeByte(value);
			startAddr++;
		}
	}
	else {
		// Output 2 palettes to use by swapping quickly (to give a "blink" effect)
		writer.prepareFor(destBinFilePrefix, "pal_lo");
		for (unsigned i = 0; i < p->currentColors; i++) {
			Pixel pix = p->getColor(i);
			uint8_t value = (pix.r/2) | (pix.g/2) << 2 | (pix.b/2) << 4;
			writer.writeByte(value);
			startAddr++;
		}

		writer.prepareFor(destBinFilePrefix, "pal_hi");
		for (unsigned i = 0; i < p->currentColors; i++) {
			Pixel pix = p->getColor(i);
			uint8_t value = ((pix.r+1)/2) | ((pix.g+1)/2) << 2 | ((pix.b+1)/2) << 4;
			writer.writeByte(value);
			startAddr++;
		}
	}

	// Write the tileset
	writer.prepareFor(destBinFilePrefix, "til");
	for (unsigned i = 0; i < t->numberOfTiles(); i++) {
		Tile *tile = t->getTile(i);
		for (unsigned k = 0; k < tile->height; k++) {
			int pix0 = 0, pix1 = 0, pix2 = 0, pix3 =0;
			for (unsigned j = 0; j < tile->width; j++) {
				int pixel = tile->getPixel(j, k).index;
				pix0 |= (pixel & 1) << (7 - j);
				pix1 |= (pixel >> 1 & 1) << (7 - j);
				pix2 |= (pixel >> 2 & 1) << (7 - j);
				pix3 |= (pixel >> 3 & 1) << (7 - j);
			}
			writer.writeByte(pix0);
			writer.writeByte(pix1);
			writer.writeByte(pix2);
			writer.writeByte(pix3);
			startAddr += 4;
		}
	}

	if (m) {
		// Here, we want two things: either we generate an associated TMX if it doesn't exist, or we convert it to a map
		char tmxFileName[512];
		sprintf(tmxFileName, "out/%s_tiled.tmx", destBinFilePrefix);
		ref<Map> existingMap = readMapFromTiledCsvProjectFile(tmxFileName);
		if (!existingMap) {
			createTiledProjectFile(tmxFileName, data);
		} else {
			m = existingMap;
		}

		// Write the map anyway
		writer.prepareFor(destBinFilePrefix, "map");
		if (!writeMapVertically) {
			for (unsigned j = 0; j < m->height; j++) {
				for (unsigned i = 0; i < 256 / 8; i++) {
					MapBlock tiledBlock = m->getBlock(i, j);
					unsigned short write = (unsigned short) tiledBlock & 0xffff;
					// Tiled supports tile flipping, so convert it to SMS values
					if (tiledBlock & 1 << 31)
						write |= 1 << 9;
					if (tiledBlock & 1 << 30)
						write |= 1 << 10;
					uint8_t b1 = write & 0xff, b2 = write >> 8;
					writer.writeByte(b1);
					writer.writeByte(b2);
					startAddr += 2;
				}
			}
		} else {
			for (unsigned i = 0; i < m->width; i++) {
				for (unsigned j = 0; j < m->height; j++) {
					MapBlock tiledBlock = m->getBlock(i, j);
					unsigned short write = (unsigned short) tiledBlock & 0xffff;
					// Tiled supports tile flipping, so convert it to SMS values
					if (tiledBlock & 1 << 31)
						write |= 1 << 9;
					if (tiledBlock & 1 << 30)
						write |= 1 << 10;
					uint8_t b1 = write & 0xff, b2 = write >> 8;
					writer.writeByte(b1);
					writer.writeByte(b2);
					startAddr += 2;
				}
			}
		}
	}
}

class SpriteBuilder {
	ref<Tileset> tileset;
public:
	SpriteBuilder() : tileset(null) {}

	SpriteBuilder &convertSprite(const char *imageName) {
		TilesetConversionData conversionParams(newref Image(imageName, Pixel(255, 0, 255, 255)));
		conversionParams.outTileset = tileset;
		conversionParams.tileHeight = 16;
		doTilesetConversion(conversionParams);
		tileset = conversionParams.outTileset;
		return *this;
	}

	void writeSprites(const char *destBinFilePrefix) {
		ref<Image> imageToConvert = tilesetAsImage(tileset, null, PixelFormat(), 8);
		TilesetConversionData conversionParams(imageToConvert);
		setConversionParams(conversionParams, false, false, true);
		doTilesetConversion(conversionParams);
		// Do not write a map
		ref<Map> temp = conversionParams.outMap;
		conversionParams.outMap = false;
		writeTilemapForSms(destBinFilePrefix, conversionParams, false);
	}
};

void convertBg(const char *imageName, const char *destBinFilePrefix, bool use9bit = false) {
	TilesetConversionData conversionParams(newref Image(imageName, Pixel(255, 0, 255, 255)));
	setConversionParams(conversionParams, true, use9bit, false);
	doTilesetConversion(conversionParams);
	writeTilemapForSms(destBinFilePrefix, conversionParams, use9bit, true);
}

int main(int argc, char *argv[]) {
	outDefs = fopen("out/gfx.inc", "w");
	outDefsC = fopen("out/gfx.h", "w");
	outBin = fopen("out/gfx.bin", "wb");
	if (!outDefs) {
		fprintf(stderr, "Could not write to output directory, check your rights.\n");
		return -1;
	}
	convertBg("level.png", "levelbg", false);
	SpriteBuilder()
		.convertSprite("player8x8.png")
		.convertSprite("player16x16.png")
		.convertSprite("player1_32x32.png")
		.convertSprite("player2_32x32.png")
		.writeSprites("sprites");
	fclose(outDefs);
	fclose(outBin);
	return 0;
}
