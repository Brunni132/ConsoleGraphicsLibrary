#include "ConsoleGraphics.h"
#include "ColorReduction.h"
#include "pugixml.hpp"

void doTilesetConversion(TilesetConversionData &data) {
	unsigned tilesWidth = (data.source->width + data.tileWidth - 1) / data.tileWidth;
	unsigned tilesHeight = (data.source->height + data.tileHeight - 1) / data.tileHeight;
	unsigned tileCount = 0;
	ref<Map> map = newref Map(tilesWidth, tilesHeight);
	ref<Tileset> tileset = data.outTileset ? data.outTileset : newref Tileset();
	ref<Palette> palette = null;

	data.pixelFormat.prepareForUse();
	deflateSourceImage(data.source, data.transparentColor, data.pixelFormat);
	// Indexed: simple case (1 palette)
	if (data.pixelFormat.isIndexed() && data.pixelFormat.maxPalettes <= 1) {
		palette = CreateOctreePalette(data.source, data.pixelFormat.maxColors, data.pixelFormat);
		printf("Using %d colors\n", palette->currentColors);
		data.source->makeIndexed(palette);
	}
	
	for (unsigned tileY = 0; tileY < tilesHeight; tileY++) {
		for (unsigned tileX = 0; tileX < tilesWidth; tileX++) {
			ref<Tile> newTile = newref Tile(data.source, tileX * data.tileWidth, tileY * data.tileHeight, data.tileWidth, data.tileHeight);
			unsigned similarTile = tileCount;
			if (data.optimizeTileset) {
				// Find another similar tile
				for (similarTile = 0; similarTile < tileCount; similarTile++) {
					if (tileset->getTile(similarTile)->compareTo(newTile))
						break;
				}
			}
			// Set map block and/or add tile if new
			if (similarTile < tileCount)
				map->setBlock(tileX, tileY, similarTile);
			else {
				tileset->addTile(newTile);
				map->setBlock(tileX, tileY, tileCount++);
			}
		}
	}

	if (data.pixelFormat.isIndexed() && data.pixelFormat.maxPalettes > 1) {
		DoMultipaletteConversion(tileset, map, &palette, data.pixelFormat);
	}

	data.outMap = map;
	data.outTileset = tileset;
	data.outPalette = palette;
}

ref<Image> tilesetAsImage(ref<Tileset> tileset, ref<Palette> paletteIfAny, PixelFormat &originalPixelFormat, unsigned widthPixels) {
	unsigned tileWidth = tileset->getTile(0)->width, tileHeight = tileset->getTile(0)->height;
	unsigned tilesPerLine = widthPixels / tileset->getTile(0)->width;
	unsigned tilesTall = (tileset->numberOfTiles() + tilesPerLine - 1) / tilesPerLine;
	unsigned tileNo = 0;
	ref<Image> result = newref Image(widthPixels, tilesTall * tileset->getTile(0)->height, paletteIfAny);
	for (unsigned j = 0; j < tilesTall; j++)
		for (unsigned i = 0; i < tilesPerLine; i++) {
			if (tileNo < tileset->numberOfTiles())
				result->drawTile(i * tileWidth, j * tileHeight, tileset->getTile(tileNo++));
		}

	// If the image was indexed, we need to get it back to normal
	if (result->isIndexed()) {
		result = result->makeTrueColorCopy();
	}

	inflateSourceImage(result, originalPixelFormat);
	return result;
}

ref<Image> mapWithMultipaletteAsImage(ref<Map> m, ref<Tileset> t, ref<Palette> p, PixelFormat &pf) {
	unsigned tw = t->getTile(0)->width, th = t->getTile(0)->height;
	ref<Image> result = newref Image(m->width * tw, m->height * th + 1);
	for (unsigned j = 0; j < m->height; j++)
		for (unsigned i = 0; i < m->width; i++) {
			MapBlock b = m->getBlock(i, j);
			unsigned palOffset = (b >> pf.mapBitsPaletteOffset) * pf.maxColors;
			ref<Tile> tile = t->getTile(b & ((1 << pf.mapBitsPaletteOffset) - 1));
			for (unsigned y = 0; y < th; y++)
				for (unsigned x = 0; x < tw; x++) {
					result->setPixel(i * tw + x, j * th + y, p->getColor(palOffset + tile->getPixel(x, y).index));
				}
		}
	for (unsigned i = 0; i < p->currentColors; i++)
		result->setPixel(i, m->height * th, p->getColor(i));

	inflateSourceImage(result, pf);
	return result;
}

void createTiledProjectFile(const char *tmxFileName, TilesetConversionData &conversionParams) {
	Map *map = conversionParams.outMap;
	Tileset *til = conversionParams.outTileset;
	Palette *pal = conversionParams.outPalette;
	PixelFormat &pf = conversionParams.pixelFormat;

	char imageName[512];
	sprintf(imageName, "%s.png", tmxFileName);
	tilesetAsImage(til, pal, pf, 256)->writeToFile(imageName);

	char name[512];
	char *imageNameToWrite = MAX(MAX(imageName, strrchr(imageName, '\\')+1), strrchr(imageName, '/')+1);
	Image img(imageName);
	sprintf(name, "%s", tmxFileName);
	FILE *f = fopen(name, "wt");
	fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(f, "	<map version=\"1.0\" orientation=\"orthogonal\" width=\"%d\" height=\"%d\" tilewidth=\"%d\" tileheight=\"%d\">\n", map->width, map->height, conversionParams.tileWidth, conversionParams.tileHeight);
	fprintf(f, "	<tileset firstgid=\"1\" name=\"tileset\" tilewidth=\"8\" tileheight=\"8\">\n");
	fprintf(f, "	<image source=\"%s\" width=\"%d\" height=\"%d\"/>\n", imageNameToWrite, img.width, img.height);
	fprintf(f, "	</tileset>\n");
	fprintf(f, "	<layer name=\"Calque de Tile 1\" width=\"%d\" height=\"%d\">\n", map->width, map->height);
	fprintf(f, "	<data encoding=\"csv\">\n");
	// Write map data from here
	for (int j = 0; j < map->height; j++) {
		for (int i = 0; i < map->width; i++) {
			// Tiled considers tile 0 as transparent, not included in the tileset; we want to use our first tile as transparent
			fprintf(f, "%d,", map->getBlock(i, j) + 1);
		}
		if (j == map->height - 1)
			fseek(f, -1, SEEK_CUR);
		fputc('\n', f);
	}
	fprintf(f, "	</data>\n");
	fprintf(f, "	</layer>\n");
	fprintf(f, "	</map>\n");
	fclose(f);
}

ref<Map> readMapFromTiledCsvProjectFile(const char *tmxFileName) {
	using namespace pugi;
	xml_document doc;
	xml_parse_result parseResult = doc.load_file(tmxFileName);
	if (parseResult.status == status_file_not_found)
		return null;
	// XML: map -> layer -> data
	xml_node layer = doc.child("map").child("layer");
	const char_t *data = layer.child_value("data");
	// Get size and create map for that
	int width = atoi(layer.attribute("width").value()), height = atoi(layer.attribute("height").value());
	ref<Map> result = newref Map(width, height);
	// Fetch data
	for (int i = 0; i < width * height; i++) {
		// Skip anything unrelated to map indexes
		while (*data && (*data < '0' || *data > '9'))
			data++;
		if (!*data)
			break;
		// 1 = first tile in Tiled
		MapBlock value;
		sscanf(data, "%d", &value);
		result->setBlock(i % width, i / width, value - 1);
		// Advance to next number
		while (*data >= '0' && *data <= '9')
			data++;
	}
	return result;
}
