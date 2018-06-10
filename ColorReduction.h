#ifndef COLORREDUCTION_H
#define COLORREDUCTION_H

// The image is not modified, you should apply the resulting palette to the image if you are ok with it
ref<Palette> CreateOctreePalette(ref<Image> img, unsigned maxColors, PixelFormat &pixelFormat);

// ^ defined in ColorReduction.h || v defined in ColorReductionMultipalette.h

// This one does much much more. Before calling, you should have a tileset with true color (already deflated though) and a map constituting the original image.
// When done, you will have a palette of maxColors * maxPalettes and the map will be updated with the palette number flags.
// After that, you should remove double tiles (which may still exist) and adapt the map accordingly. Do not forget to release the palette.
void DoMultipaletteConversion(ref<Tileset> initialTileset, ref<Map> initialMap, ref<Palette> *outPalette, PixelFormat &pixelFormat, bool useOctree = true);

#endif
