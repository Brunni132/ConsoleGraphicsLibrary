#ifndef CONVERSION_H
#define CONVERSION_H

struct TilesetConversionData {
	// Source image (required)
	ref<Image> source;
	// Output tileset (this one may be specified to add to an existing tileset)
	ref<Tileset> outTileset;
	// Output palette (must be null)
	ref<Palette> outPalette;
	// Output map (must be null)
	ref<Map> outMap;

	// Conversion parameters
	unsigned tileWidth, tileHeight;
	bool optimizeTileset;
	Pixel transparentColor;		// if not 0; not applied on palette but source image only
	PixelFormat pixelFormat;

	TilesetConversionData(ref<Image> source) : source(source), outTileset(0), outPalette(0), outMap(0),
		tileWidth(8), tileHeight(8), optimizeTileset(true), transparentColor(0) {}
};

// Process with the conversion. The TilesetConversionData must be carefully configured before
void doTilesetConversion(TilesetConversionData &data);
// Writes a converted tileset as an image, containing only the relevant tiles
ref<Image> tilesetAsImage(ref<Tileset> tileset, ref<Palette> paletteIfAny, PixelFormat &originalPixelFormat, unsigned widthPixels);
// Writes an image on the disc representing the visual output of the tilemap conversion, with multipalette support (useful to check without the hardware)
ref<Image> mapWithMultipaletteAsImage(ref<Map> m, ref<Tileset> t, ref<Palette> p, PixelFormat &pf);
// Utility to write a tiled project from a conversion (should be used initially, will generate an image too)
void createTiledProjectFile(const char *tmxFileName, TilesetConversionData &conversionParams);
// Utility to read a map from a CSV typed Tiled project file
ref<Map> readMapFromTiledCsvProjectFile(const char *tmxFileName);


#endif
