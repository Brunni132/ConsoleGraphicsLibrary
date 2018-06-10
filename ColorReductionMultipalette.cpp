#include "ConsoleGraphics.h"
#include "ColorReduction.h"
#include <math.h>

// Magic
#define xPrecisionMax 25000

#define xMaxTilesPerPalette 10000
#define xReduceColors 1
#define xMaxColorsPerPalette 256
#define xMaxNbPalettes 256
// Must correspond to the Pixel data type
#define xPixelType uint32_t

struct XPALETTE {
	xPixelType data[xMaxColorsPerPalette];
	unsigned associatedTiles[xMaxTilesPerPalette];
	unsigned nbAssocTiles;
	unsigned usedColors;

	XPALETTE() : nbAssocTiles(0), usedColors(0) {
		memset(data, 0, sizeof(data));
		memset(associatedTiles, 0, sizeof(associatedTiles));
	}
};

static XPALETTE *xPalettes;
static unsigned premCouleur;
static unsigned nbPalettes;
static unsigned colorsPerPalette;
static unsigned xUsedPalettes;
// Settable -- optimisation vitesse (should be set to 1)
static bool xUseOctree;
static int64_t xGblScore;
// xUnfavorNewColorCreation: -1: défavorise la création de nouvelles couleurs (pourri), 0: rien, >0: calcule les diffs entre les couleurs * facteur, -2: calcule pour toute l'image (meilleur résultat, ultra lent)
// Settable -- colorization (values: -2 - 2500)
static int xUnfavorNewColorCreation;
//xPostProcessingCompletePalette: post processing qui remplit les palettes inutilisées avec les tiles qui le méritent. Utile si unfavorcolorcreation est >0
// Settable -- post processing (should be set to 1)
static bool xPostProcessingCompletePalette;

static void xApplyOptimalTilesetPalData(ref<Tile> t, xPixelType *indexedTile, xPixelType *destPalette, unsigned precisionMax, unsigned nCoul);
static void xApplyOptimalTilesetPalDataMultiple(XPALETTE *p, ref<Tileset> tileset, xPixelType *indexedTile, xPixelType *indexedPalette, unsigned precisionMax, unsigned nCoul);

#include "xReduc.hpp"

static void xDisplayPalettes() {
}

//Calcule et renvoie le nombre de couleurs et stocke une palette ainsi qu'un tableau
//représentant l'image 8 bits en utilisant "nbMax" couleurs et une tolérance de
//"différence" unités RVB.
static int xCalcPalette(ref<Tile> tile, xPixelType *destTile, xPixelType *destPalette, unsigned difference, unsigned nCoulInitiales, unsigned nbBase) {
	unsigned nbCouleurs=nCoulInitiales, coulTransparente = 0;

	xGblScore = 0;

	unsigned l = tile->width, h = tile->height;
	for (unsigned y = 0; y < h; y++) {
		for (unsigned x = 0; x < l; x++) {
			Pixel p = tile->getPixel(x, y);
			if (p.a == 0) {
				destTile[l * y + x] = coulTransparente;
				continue;
			}

			unsigned bestDifference=difference;
			int bestDiffNum=-1;
			for (unsigned i=nbBase;i<nbCouleurs;i++) {
				unsigned delta = p.colorDeltaWith(destPalette[i]);
				if (delta <= bestDifference) {
					bestDiffNum = i;
					bestDifference = delta;
				}
			}
			if (bestDiffNum == -1) {			//La couleur n'existe pas
				if (nbCouleurs < colorsPerPalette)
					destPalette[nbCouleurs++] = p.value;
				else
					return -1;
			}
			// CHANGED from original
			xGblScore += (int64_t) sqrt(bestDifference);
			destTile[l*y+x]=bestDiffNum;
		}
	}
	return nbCouleurs;
}

static int xCalcPaletteMultiple(XPALETTE *pal, ref<Tileset> tileset, xPixelType *destPalette, unsigned difference, unsigned nCoulInitiales, unsigned nbBase) {
	unsigned nbCouleurs=nCoulInitiales, coulTransparente = 0;
	unsigned l = tileset->getTile(0)->width, h = tileset->getTile(0)->height;
	unsigned i;

	xGblScore = 0;

	for (unsigned p=0;p<pal->nbAssocTiles;p++)		{
		ref<Tile> t = tileset->getTile(pal->associatedTiles[p]);
		for (unsigned y=0;y<h;y++)		{
			for (unsigned x=0;x<l;x++)		{
				Pixel p = t->getPixel(x, y);
				if (p.a == 0)
					continue;

				unsigned bestDifference=difference;
				int bestDiffNum=-1;
				for (i=nbBase;i<nbCouleurs;i++) {
					unsigned delta = p.colorDeltaWith(destPalette[i]);
					if (delta <= bestDifference) {
						bestDiffNum = i;
						bestDifference = delta;
					}
				}
				if (bestDiffNum == -1) {			//La couleur n'existe pas
					if (nbCouleurs < colorsPerPalette)
						destPalette[nbCouleurs++] = p.value;
					else
						return -1;
				}
				// CHANGED from original
				xGblScore += (int64_t) sqrt(bestDifference);
			}
		}
	}
	return nbCouleurs;
}

// Assigned to xGblUsedColors
static unsigned xTestOptimalTilesetOnTile(unsigned palnb, ref<Tile> tile, xPixelType *destTile, xPixelType *destPalette, unsigned precisionMax)			{
	int nbCouleurs, prec, i, nCMax;
	int nCoulInitiales, nCoulBase = premCouleur;

	//Crée la palette
	memcpy(destPalette, xPalettes[palnb].data, sizeof(xPixelType) * colorsPerPalette);
	nCoulInitiales = xPalettes[palnb].usedColors;

	if (xUseOctree) {
		nbCouleurs = xReductionOctree(tile, destTile, destPalette, nCoulInitiales, colorsPerPalette);
		if (nbCouleurs == 0)
			goto quandmeme;
	}
	else			{
quandmeme:
		nbCouleurs = xCalcPalette(tile, destTile, destPalette, 0, nCoulInitiales, nCoulBase);
		prec=0;
		i=precisionMax;
		nCMax=-1;

		if (nbCouleurs==-1)		{
			//Petit algo codé à part pour trouver rapidement la meilleure précision
			int plagemin=-1, plagemax=precisionMax;
			int numeroactuel, max;

			numeroactuel = plagemax;
			max = plagemax;
			while(plagemin+1<plagemax)		{
				nbCouleurs = xCalcPalette(tile, destTile, destPalette, numeroactuel, nCoulInitiales, nCoulBase);
				if (nbCouleurs!=-1)			{
					plagemax = numeroactuel;
					if (numeroactuel == plagemin + 1)
						numeroactuel--;
					else
						numeroactuel = (numeroactuel + plagemin)/2;
				}
				else	{
					plagemin = numeroactuel;
					if (numeroactuel == plagemax - 1)
						numeroactuel++;
					else
						numeroactuel = (numeroactuel + plagemax)/2;
				}
			}
			prec = plagemax;

			nbCouleurs = xCalcPalette(tile, destTile, destPalette, prec, nCoulInitiales, nCoulBase);
			if (nbCouleurs<1)		{
				fputs("Unable to convert the bitmap because the required number of colors cannot be reached.\n", stderr);
				return 0;
			}
		}
	}
	return nbCouleurs;
}

static unsigned xTestOptimalTilesetOnMultipleTiles(XPALETTE *p, ref<Tileset> tileset, xPixelType *destTile, xPixelType *destPalette, unsigned precisionMax)			{
	int nbCouleurs, prec, i, nCMax;
	int nCoulInitiales = premCouleur, nCoulBase = premCouleur;
	unsigned l = tileset->getTile(0)->width, h = tileset->getTile(0)->height;

	//Crée la palette
	memcpy(destPalette, p->data, sizeof(xPixelType) * colorsPerPalette);

	if (xUseOctree)			{
		nbCouleurs = xReductionOctreeMultiple(p, tileset, destTile, destPalette, nCoulInitiales, colorsPerPalette);
		if (nbCouleurs == 0)
			goto quandmeme;
	}
	else			{
quandmeme:
		nbCouleurs = xCalcPaletteMultiple(p, tileset, destPalette, 0, nCoulInitiales, nCoulBase);
		prec=0;
		i=precisionMax;
		nCMax=-1;

		if (nbCouleurs==-1)		{
			//Petit algo codé à part pour trouver rapidement la meilleure précision
			int plagemin=-1, plagemax=precisionMax;
			int numeroactuel, max;

			numeroactuel = plagemax;
			max = plagemax;
			while(plagemin+1<plagemax)		{
				nbCouleurs = xCalcPaletteMultiple(p, tileset, destPalette, numeroactuel, nCoulInitiales, nCoulBase);
				if (nbCouleurs!=-1)			{
					plagemax = numeroactuel;
					if (numeroactuel == plagemin + 1)
						numeroactuel--;
					else
						numeroactuel = (numeroactuel + plagemin)/2;
				}
				else	{
					plagemin = numeroactuel;
					if (numeroactuel == plagemax - 1)
						numeroactuel++;
					else
						numeroactuel = (numeroactuel + plagemax)/2;
				}
			}
			prec = plagemax;

			nbCouleurs = xCalcPaletteMultiple(p, tileset, destPalette, prec, nCoulInitiales, nCoulBase);
			if (nbCouleurs<1)		{
				fputs("Unable to convert the bitmap because the required number of colors cannot be reached.\n", stderr);
				return 0;
			}
		}
	}
	return nbCouleurs;
}

static void xAddPalette(xPixelType *palette, unsigned usedColors, unsigned tileNo)			{
	memcpy(xPalettes[xUsedPalettes].data, palette, usedColors * sizeof(xPixelType));
	xPalettes[xUsedPalettes].usedColors = usedColors;
	xPalettes[xUsedPalettes].associatedTiles[0] = tileNo;
	xPalettes[xUsedPalettes].nbAssocTiles = 1;
	xUsedPalettes++;
}

static void xReplacePalette(XPALETTE *p, xPixelType *palette, unsigned usedColors)			{
	memcpy(p->data, palette, usedColors * sizeof(xPixelType));
	p->usedColors = usedColors;
}

static int xRenderPalette(xPixelType *indexedTile, xPixelType *destPalette, ref<Tile> tile, unsigned difference, unsigned nCoulInitiales, unsigned nbBase, unsigned nbMax) {
	unsigned nbCouleurs = nbMax, coulTransparente = 0;
	unsigned i;

	xGblScore = 0;

	unsigned l = tile->width, h = tile->height;
	for (unsigned y=0;y<h;y++)		{
		for (unsigned x=0;x<l;x++)		{
			Pixel p = tile->getPixel(x, y);
			if (p.a == 0) {
				indexedTile[l * y + x]=coulTransparente;
				continue;
			}

			unsigned bestDifference = 0;
			int bestDiffNum = -1;
			for (i=nbBase;i<nbCouleurs;i++) {
				unsigned delta = p.colorDeltaWith(destPalette[i]);
				if (delta <= bestDifference || bestDiffNum == -1) {
					bestDiffNum = i;
					bestDifference = delta;
				}
			}
			// No color available at all, should be able to return from there
			if (bestDiffNum == -1)
				continue;
			// CHANGED from original
			xGblScore += (int64_t) sqrt(bestDifference);
			indexedTile[l * y + x] = bestDiffNum;
		}
	}
	return nbCouleurs;
}

static int xRenderPaletteMultiple(XPALETTE *pal, ref<Tileset> tileset, xPixelType *indexedTile, xPixelType *destPalette, unsigned difference, unsigned nCoulInitiales, unsigned nbBase, unsigned nbMax) {
	unsigned nbCouleurs = nbMax, coulTransparente;
	unsigned l = tileset->getTile(0)->width, h = tileset->getTile(0)->height;
	unsigned i;

	coulTransparente=0;
	xGblScore = 0;

	for (unsigned p=0;p<pal->nbAssocTiles;p++)		{
		ref<Tile> t = tileset->getTile(pal->associatedTiles[p]);
		for (unsigned y=0; y<h; y++)		{
			for (unsigned x=0; x<l; x++)		{
				Pixel p = t->getPixel(x, y);
				if (p.a == 0) {
					indexedTile[l * y + x]=coulTransparente;
					continue;
				}

				unsigned bestDifference = 0;
				int bestDiffNum = -1;
				for (i=nbBase;i<nbCouleurs;i++) {
					unsigned delta = p.colorDeltaWith(destPalette[i]);
					if (delta <= bestDifference || bestDiffNum == -1) {
						bestDiffNum = i;
						bestDifference = delta;
					}
				}
				// No color available at all, should be able to return from there
				if (bestDiffNum == -1)
					continue;
				// CHANGED from original
				xGblScore += (int64_t) sqrt(bestDifference);
				indexedTile[l * y + x] = bestDiffNum;
			}
		}
	}
	return nbCouleurs;
}
static void xApplyOptimalTileset(int palnb, xPixelType *indexedTile, xPixelType *destPalette, ref<Tile> tile, unsigned precisionMax) {
	//Crée la palette
	memcpy(destPalette, xPalettes[palnb].data, sizeof(xPixelType) * colorsPerPalette);
	xRenderPalette(indexedTile, destPalette, tile, 0, premCouleur, premCouleur, xPalettes[palnb].usedColors);
}

static int xGetBestPaletteForTile(xPixelType *indexedTile, xPixelType *destPalette, ref<Tile> tile, unsigned precisionMax)		{
	int bestPalette=-1;
	int64_t minScore;
	minScore = 0;
	for (unsigned i=0;i<xUsedPalettes;i++)		{
		xApplyOptimalTileset(i, indexedTile, destPalette, tile, precisionMax);
		if (xGblScore < minScore || bestPalette == -1)			{
			bestPalette = i;
			minScore = xGblScore;
		}
	}
	return bestPalette;
}

static void xAddTileToPalette(XPALETTE *p, unsigned tileNo) {
	if (p->nbAssocTiles >= xMaxTilesPerPalette) {
		fputs("FATAL ERROR: TOO MUCH TILES PER PALETTE!\n", stderr);
		return;
	}
	p->associatedTiles[p->nbAssocTiles++] = tileNo;
}

static int64_t xCalcWholeImageScore(ref<Tileset> initialTileset) {
	unsigned tileX = initialTileset->getTile(0)->width, tileY = initialTileset->getTile(0)->height;
	xPixelType palette[xMaxColorsPerPalette];
	xPixelType *tempIndexedTile = new xPixelType[tileX * tileY];
	unsigned pal;
	int64_t score;
	score=0;
	for (unsigned i = 0; i < initialTileset->numberOfTiles(); i++) {
		pal = xGetBestPaletteForTile(tempIndexedTile, palette, initialTileset->getTile(i), 250000);
		xApplyOptimalTileset(pal, tempIndexedTile, palette, initialTileset->getTile(i), 250000);
		score+=xGblScore;
	}
	delete[] tempIndexedTile;
	return score;
}

void xApplyOptimalTilesetPalData(ref<Tile> t, xPixelType *indexedTile, xPixelType *destPalette, unsigned precisionMax, unsigned nCoul) {
	//Crée la palette
//	memcpy(palette, pal, sizeof(*palette)*xColorsPerPalette);
	xRenderPalette(indexedTile, destPalette, t, 0, premCouleur, premCouleur, nCoul);
}

void xApplyOptimalTilesetPalDataMultiple(XPALETTE *p, ref<Tileset> tileset, xPixelType *indexedTile, xPixelType *destPalette, unsigned precisionMax, unsigned nCoul) {
	//Crée la palette
//	memcpy(palette, pal, sizeof(*palette)*xColorsPerPalette);
	xRenderPaletteMultiple(p, tileset, indexedTile, destPalette, 0, premCouleur, premCouleur, nCoul);
}

void DoMultipaletteConversion(ref<Tileset> initialTileset, ref<Map> initialMap, ref<Palette> *outPalette, PixelFormat &pixelFormat, bool useOctree) {
	unsigned tileX = initialTileset->getTile(0)->width, tileY = initialTileset->getTile(0)->height;
	xPixelType palette[xMaxColorsPerPalette];
	xPixelType *indexedTile;
	unsigned usedColors;

	// Sanity check
	if (colorsPerPalette > xMaxColorsPerPalette || nbPalettes > xMaxNbPalettes) {
		fputs("Nothing to do: too much colors/palettes\n", stderr);
		return;
	}

	xPalettes = new XPALETTE[xMaxNbPalettes];
	indexedTile = new xPixelType[tileX * tileY];
	xUsedPalettes = 0;
	premCouleur = pixelFormat.firstColorTransparent ? 1 : 0;
	nbPalettes = pixelFormat.maxPalettes;
	colorsPerPalette = pixelFormat.maxColors;
	xUnfavorNewColorCreation = pixelFormat.unfavorNewColorCreation;
	xPostProcessingCompletePalette = pixelFormat.postProcessingCompletePalettes;
	xUseOctree = useOctree;

	// Initialize for conversion
	for (unsigned i = 0; i < nbPalettes; i++)
		xPalettes[i].usedColors = premCouleur;

	// nCoul = colorsPerPalette, motifs = (n/a), map = (dest map), syspal = (dest palette)
	//Première étape: trouver les meilleures palettes possibles
	//1-1: première palette: la tile la plus gourmande
	int64_t maxScore = -1;
	unsigned bestTileNo;
	for (unsigned i = 0; i < initialTileset->numberOfTiles(); i++) {
		unsigned score = xTestOptimalTilesetOnTile(0, initialTileset->getTile(i), indexedTile, palette, 250000);
		if (score > maxScore || maxScore == -1)
			maxScore = score, bestTileNo = i;
	}

	usedColors = xTestOptimalTilesetOnTile(0, initialTileset->getTile(bestTileNo), indexedTile, palette, 250000);
	xAddPalette(palette, usedColors, bestTileNo);
	printf("%d %lld %d\n", xUsedPalettes, maxScore, bestTileNo);

	// Remplit les autres palettes
	while (xUsedPalettes < nbPalettes)			{
		maxScore = -1;
		for (unsigned i = 0; i < initialTileset->numberOfTiles(); i++) {
			unsigned pal = xGetBestPaletteForTile(indexedTile, palette, initialTileset->getTile(i), 250000);
			xApplyOptimalTileset(pal, indexedTile, palette, initialTileset->getTile(i), 250000);
			if (xGblScore > maxScore || maxScore == -1)
				maxScore = xGblScore, bestTileNo = i;
		}

		usedColors = xTestOptimalTilesetOnTile(xUsedPalettes, initialTileset->getTile(bestTileNo), indexedTile, palette, 250000);
		xAddPalette(palette, usedColors, bestTileNo);
		if (usedColors == 0)			{
			printf("ERROR %d %d\n", xUsedPalettes, bestTileNo);
		}
		printf("%d %lld %d\n", xUsedPalettes, maxScore, bestTileNo);
	}
	xDisplayPalettes();

	//Seconde étape: trouver quelle tile va avec quelle palette
	XPALETTE temp;
	unsigned bestPal;
	for (unsigned tileNo = 0; tileNo < initialTileset->numberOfTiles(); tileNo++) {
		maxScore = -1;
		for (unsigned i=0;i<xUsedPalettes;i++)		{
			memcpy(&temp, &xPalettes[i], sizeof(temp));
			xAddTileToPalette(&temp, tileNo);
			usedColors = xTestOptimalTilesetOnMultipleTiles(&temp, initialTileset, indexedTile, palette, 250000);
			if (xUnfavorNewColorCreation > 0)			{
				for (unsigned tmp=0;tmp<usedColors;tmp++)		{
					if (palette[tmp] != xPalettes[i].data[tmp])
						// CHANGED from original
						xGblScore += xUnfavorNewColorCreation * Pixel(palette[tmp]).colorDeltaWith(xPalettes[i].data[tmp]);
				}
			}
			//Désactivé pour test
			if (xUnfavorNewColorCreation == -1)
				xGblScore += (usedColors - xPalettes[i].usedColors)*100;
			else if (xUnfavorNewColorCreation == -2)			{
				XPALETTE temp2;
				memcpy(&temp2, &xPalettes[i], sizeof(temp2));
				xGblScore = xCalcWholeImageScore(initialTileset);
				memcpy(&xPalettes[i], &temp2, sizeof(temp2));
			}
			if (xGblScore < maxScore || maxScore == -1)
				maxScore = xGblScore, bestTileNo = tileNo, bestPal = i;
		}
		xAddTileToPalette(&xPalettes[bestPal], tileNo);
		usedColors = xTestOptimalTilesetOnMultipleTiles(&xPalettes[bestPal], initialTileset, indexedTile, palette, 250000);
		xReplacePalette(&xPalettes[bestPal], palette, usedColors);
//		test_tablo(x,y, xTileX, xTileY, tableau, palette);
	}

	xDisplayPalettes();

	/*
		POST PROCESSING DE TEST!!!!!
		Algo possible: pour chaque palette dans laquelle il manque des couleurs, on
		essaie de caser une tile qui remplira cette palette. Celle qui améliore le plus
		le score final est prise.
	*/

	if (xPostProcessingCompletePalette) {
		unsigned tiles = initialTileset->numberOfTiles();
		bool *used = new bool[tiles];
		memset(used, 0, sizeof(bool) * tiles);
		while(1) {
			int neededPalette = -1;
			unsigned j = 0;
			for (unsigned i=0;i<xUsedPalettes;i++)			{
				if (xPalettes[i].usedColors < colorsPerPalette)			{
					neededPalette = i;
					j += colorsPerPalette - xPalettes[i].usedColors;
				}
			}
			//Plus de palette libre
			if (neededPalette == -1)
				break;
			printf("Free colors: %d\n", j);

			maxScore = -1;
			//Seconde étape: trouver quelle tile va avec quelle palette
			for (unsigned tileNo = 0; tileNo < initialTileset->numberOfTiles(); tileNo++) {
				for (unsigned i=0;i<xUsedPalettes;i++)		{
					if (xPalettes[i].usedColors >= colorsPerPalette)
						continue;
					int pal = xGetBestPaletteForTile(indexedTile, palette, initialTileset->getTile(tileNo), 250000);
					xApplyOptimalTileset(pal, indexedTile, palette, initialTileset->getTile(tileNo), 250000);
					if ((xGblScore > maxScore || maxScore == -1) && !used[tileNo])
						maxScore = xGblScore, bestTileNo = tileNo, bestPal = i;
				}
			}
			if (maxScore == -1)
				break;
			usedColors = xTestOptimalTilesetOnTile(bestPal, initialTileset->getTile(bestTileNo), indexedTile, palette, 250000);
			xReplacePalette(&xPalettes[bestPal], palette, usedColors);
			used[bestTileNo] = true;
//			test_tablo(curTileX,curTileY, xTileX, xTileY, tableau, palette);
		}

		delete[] used;
		xDisplayPalettes();
	}

	unsigned tileNb = 0;
	unsigned *paletteForTileNb = new unsigned[initialTileset->numberOfTiles()];
	for (unsigned tileNo = 0; tileNo < initialTileset->numberOfTiles(); tileNo++) {
		ref<Tile> t = initialTileset->getTile(tileNo);
		unsigned palNo = xGetBestPaletteForTile(indexedTile, palette, t, 250000);
		xPixelType *indexedTilePtr = indexedTile;
		xApplyOptimalTileset(palNo, indexedTile, palette, t, 250000);
		for (unsigned j = 0; j < tileY; j++)
			for (unsigned i = 0; i < tileX; i++)
				t->setPixel(i, j, *indexedTilePtr++);
		paletteForTileNb[tileNb++] = palNo;
//		test_tablo(x,y, xTileX, xTileY, tableau, palette);
	}

	for (unsigned j = 0; j < initialMap->height; j++)
		for (unsigned i = 0; i < initialMap->width; i++) {
			MapBlock b = initialMap->getBlock(i, j);
			initialMap->setBlock(i, j, b | paletteForTileNb[b] << pixelFormat.mapBitsPaletteOffset);
		}
	delete[] paletteForTileNb;

	*outPalette = newref Palette(xUsedPalettes * colorsPerPalette, false);
	for (unsigned j=0;j<xUsedPalettes;j++)
		for (unsigned i=0;i<colorsPerPalette;i++) {
			Pixel p = xPalettes[j].data[i];
			p.a = pixelFormat.opaqueAlphaValue();
			(*outPalette)->addColor(p);
		}

	/*
		END POST PROCESSING DE TEST!!!!!
	 */
	delete[] indexedTile;
	delete[] xPalettes;
}
