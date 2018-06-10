/***********************************************
			XREDUCTION
************************************************/

struct Node {
    bool bIsLeaf;               // true if Node has no children
    unsigned nPixelCount;           // Number of pixels represented by this leaf
    unsigned nRedSum;               // Sum of red components
    unsigned nGreenSum;             // Sum of green components
    unsigned nBlueSum;              // Sum of blue components
    Node *pChild[8];    // Pointers to child Nodes
    Node *pNext;        // Pointer to next reducible Node
};

unsigned nColorBits = 8;
unsigned xCreateOctreePalette (ref<Tile> tile, xPixelType *destPalette, unsigned nMaxColors, unsigned nColorBits);
void xAddColor(Node**, uint8_t, uint8_t, uint8_t, unsigned, unsigned, unsigned*, Node**);
Node* xCreateNode (unsigned, unsigned, unsigned*, Node**);
void xReduceTree (unsigned, unsigned*, Node**);
void xDeleteTree (Node**);
void xGetPaletteColors (Node*, xPixelType*, unsigned*);

unsigned xCreateOctreePalette (ref<Tile> tile, xPixelType *destPalette, unsigned nMaxColors) {
	unsigned l = tile->width, h = tile->height;
    Node* pTree;
    unsigned nLeafCount, nIndex;
    Node* pReducibleNodes[9];

    // Initialize octree variables
    pTree = NULL;
    nLeafCount = 0;
    for (unsigned i=0; i<=(int) nColorBits; i++)
        pReducibleNodes[i] = NULL;

    for (unsigned j=0; j<h; j++) {
	    for (unsigned i=0; i<l; i++) {
			Pixel p = tile->getPixel(i, j);
			if (p.a == 0)
				continue;
			xAddColor (&pTree, p.r, p.g, p.b, nColorBits, 0, &nLeafCount,
					  pReducibleNodes);
			while (nLeafCount > nMaxColors)
				xReduceTree (nColorBits, &nLeafCount, pReducibleNodes);
        }
    }

    if (nLeafCount > nMaxColors) { // Sanity check
        xDeleteTree (&pTree);
        return 0;
    }

    nIndex = 0;
	if (!pTree)
		return 0;
    xGetPaletteColors (pTree, destPalette, &nIndex);
    xDeleteTree (&pTree);
    return nLeafCount;
}

unsigned xCreateOctreePaletteMultiple (XPALETTE *pal, ref<Tileset> tileset, xPixelType *destPalette, unsigned nMaxColors) {
	unsigned l = tileset->getTile(0)->width, h = tileset->getTile(0)->height;
    Node* pTree;
    unsigned nLeafCount, nIndex;
    Node* pReducibleNodes[9];

    // Initialize octree variables
    pTree = NULL;
    nLeafCount = 0;
    for (unsigned i=0; i<=(int) nColorBits; i++)
        pReducibleNodes[i] = NULL;

	for (unsigned p=0;p<pal->nbAssocTiles;p++)		{
		ref<Tile> t = tileset->getTile(pal->associatedTiles[p]);
		for (unsigned j=0; j<h; j++) {
			for (unsigned i=0; i<l; i++) {
				Pixel p = t->getPixel(i, j);
				if (p.a == 0)
					continue;
				xAddColor (&pTree, p.r, p.g, p.b, nColorBits, 0, &nLeafCount,
						  pReducibleNodes);
				while (nLeafCount > nMaxColors)
					xReduceTree (nColorBits, &nLeafCount, pReducibleNodes);
			}
		}
	}

    if (nLeafCount > nMaxColors) { // Sanity check
        xDeleteTree (&pTree);
        return 0;
    }

	if (!pTree)
		return 0;
    nIndex = 0;
    xGetPaletteColors (pTree, destPalette, &nIndex);
    xDeleteTree (&pTree);
    return nLeafCount;
}


void xAddColor (Node** ppNode, uint8_t r, uint8_t g, uint8_t b, unsigned nColorBits,
    unsigned nLevel, unsigned* pLeafCount, Node** pReducibleNodes)
{
    int nIndex, shift;
    static uint8_t mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

    // If the node doesn't exist, create it
    if (*ppNode == NULL)
        *ppNode = xCreateNode (nLevel, nColorBits, pLeafCount,
                              pReducibleNodes);

    // Update color information if it's a leaf node
    if ((*ppNode)->bIsLeaf) {
        (*ppNode)->nPixelCount++;
        (*ppNode)->nRedSum += r;
        (*ppNode)->nGreenSum += g;
        (*ppNode)->nBlueSum += b;
    }

    // Recurse a level deeper if the node is not a leaf
    else {
        shift = 7 - nLevel;
        nIndex = (((r & mask[nLevel]) >> shift) << 2) |
            (((g & mask[nLevel]) >> shift) << 1) |
            ((b & mask[nLevel]) >> shift);
        xAddColor (&((*ppNode)->pChild[nIndex]), r, g, b, nColorBits,
                  nLevel + 1, pLeafCount, pReducibleNodes);
    }
}

Node* xCreateNode (unsigned nLevel, unsigned nColorBits, unsigned* pLeafCount,
                  Node** pReducibleNodes)
{
    Node* pNode = new Node;
   	memset(pNode, 0, sizeof(Node));
    pNode->bIsLeaf = (nLevel == nColorBits) ? true : false;
    if (pNode->bIsLeaf)
        (*pLeafCount)++;
    else { // Add the node to the reducible list for this level
        pNode->pNext = pReducibleNodes[nLevel];
        pReducibleNodes[nLevel] = pNode;
    }
    return pNode;
}

void xReduceTree (unsigned nColorBits, unsigned* pLeafCount, Node** pReducibleNodes)
{
    int i;
    Node* pNode;
    unsigned nRedSum, nGreenSum, nBlueSum, nChildren;

    // Find the deepest level containing at least one reducible node
    for (i=nColorBits - 1; (i>0) && (pReducibleNodes[i] == NULL); i--);

    // Reduce the node most recently added to the list at level i
    pNode = pReducibleNodes[i];
    pReducibleNodes[i] = pNode->pNext;

    nRedSum = nGreenSum = nBlueSum = nChildren = 0;
    for (i=0; i<8; i++) {
        if (pNode->pChild[i] != NULL) {
            nRedSum += pNode->pChild[i]->nRedSum;
            nGreenSum += pNode->pChild[i]->nGreenSum;
            nBlueSum += pNode->pChild[i]->nBlueSum;
            pNode->nPixelCount += pNode->pChild[i]->nPixelCount;
            delete pNode->pChild[i];
            pNode->pChild[i] = NULL;
            nChildren++;
        }
    }

    pNode->bIsLeaf = true;
    pNode->nRedSum = nRedSum;
    pNode->nGreenSum = nGreenSum;
    pNode->nBlueSum = nBlueSum;
    *pLeafCount -= (nChildren - 1);
}

void xDeleteTree (Node** ppNode)
{
    int i;

    for (i=0; i<8; i++) {
        if ((*ppNode)->pChild[i] != NULL)
            xDeleteTree (&((*ppNode)->pChild[i]));
    }
    delete *ppNode;
    *ppNode = NULL;
}

void xGetPaletteColors (Node* pTree, xPixelType* pPalEntries, unsigned* pIndex)
{
    int i;

    if (pTree->bIsLeaf) {
        pPalEntries[*pIndex] = Pixel(
			(pTree->nRedSum) / (pTree->nPixelCount),
			(pTree->nGreenSum) / (pTree->nPixelCount),
			(pTree->nBlueSum) / (pTree->nPixelCount), 0).value;
        (*pIndex)++;
    }
    else {
        for (i=0; i<8; i++) {
            if (pTree->pChild[i] != NULL)
                xGetPaletteColors (pTree->pChild[i], pPalEntries, pIndex);
        }
    }
}

int xReductionOctree(ref<Tile> tile, xPixelType *indexedTile, xPixelType *palette, unsigned nCoulInitiales, unsigned nCoul)		{
	if (nCoul-nCoulInitiales<=1)
		return 0;

	unsigned nbreCouleursPalette = xCreateOctreePalette(tile, palette+nCoulInitiales, nCoul-nCoulInitiales);
	if (nbreCouleursPalette <= 1)
		return 0;

	xApplyOptimalTilesetPalData(tile, indexedTile, palette, 250000, nbreCouleursPalette);

	return nbreCouleursPalette+nCoulInitiales;
}

int xReductionOctreeMultiple(XPALETTE *p, ref<Tileset> tileset, xPixelType *indexedTile, xPixelType *palette, int nCoulInitiales, int nCoul)		{
	if (nCoul-nCoulInitiales<=1)
		return 0;

	unsigned nbreCouleursPalette = xCreateOctreePaletteMultiple(p, tileset, palette+nCoulInitiales, nCoul-nCoulInitiales);

	if (nbreCouleursPalette<=1)
		return 0;

	xApplyOptimalTilesetPalDataMultiple(p, tileset, indexedTile, palette, 250000, nbreCouleursPalette);
	return nbreCouleursPalette+nCoulInitiales;
}

