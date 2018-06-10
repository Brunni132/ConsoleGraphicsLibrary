#include "ConsoleGraphics.h"
#include "ColorReduction.h"

struct Node {
    bool bIsLeaf;               // true if Node has no children
    unsigned nPixelCount;           // Number of pixels represented by this leaf
    unsigned nRedSum;               // Sum of red components
    unsigned nGreenSum;             // Sum of green components
    unsigned nBlueSum;              // Sum of blue components
    unsigned nAlphaSum;             // Sum of alpha components
    Node *pChild[16];   // Pointers to child Nodes
    Node *pNext;        // Pointer to next reducible Node
};
const unsigned nColorBits = 8;
void AddColor (Node**, Pixel p, unsigned, unsigned, unsigned*, Node**);
Node* CreateNode (unsigned, unsigned, unsigned*, Node**);
void ReduceTree (unsigned, unsigned*, Node**);
void DeleteTree (Node**);
void GetPaletteColors (Node*, ref<Palette>, PixelFormat &, unsigned*);

ref<Palette> CreateOctreePalette(ref<Image> img, unsigned maxColors, PixelFormat &pixelFormat) {
    Node* pTree;
    unsigned nLeafCount, nIndex;
    Node* pReducibleNodes[9];

    // Initialize octree variables
    pTree = NULL;
    nLeafCount = 0;
    if (nColorBits > 8) // Just in case
        return NULL;

	for (unsigned i = 0; i <= nColorBits; i++)
        pReducibleNodes[i] = NULL;

	// Create palette
	ref<Palette> result = newref Palette(maxColors, pixelFormat.firstColorTransparent);
	maxColors = result->remaningColors();

	for (unsigned j = 0; j < img->height; j++) {
		for (unsigned i = 0; i < img->width; i++) {
			Pixel p = img->getPixel(i, j);
			// Ignore fully transparent, others will become fully opaque
			if (p.a == 0)
				continue;
			// Add other colors
			AddColor(&pTree, p, nColorBits, 0, &nLeafCount, pReducibleNodes);
			while (nLeafCount > maxColors)
				ReduceTree (nColorBits, &nLeafCount, pReducibleNodes);
		}
	}

	unsigned colorCount = nLeafCount - 1;
    GetPaletteColors (pTree, result, pixelFormat, &nIndex);
    DeleteTree (&pTree);
    return result;
}

void AddColor (Node** ppNode, Pixel p, unsigned nColorBits,
    unsigned nLevel, unsigned* pLeafCount, Node** pReducibleNodes)
{
    int nIndex, shift;
    static uint8_t mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

    // If the Node doesn't exist, create it
    if (*ppNode == NULL)
        *ppNode = CreateNode (nLevel, nColorBits, pLeafCount,
                              pReducibleNodes);

    // Update color information if it's a leaf Node
    if ((*ppNode)->bIsLeaf) {
        (*ppNode)->nPixelCount++;
        (*ppNode)->nRedSum += p.r;
        (*ppNode)->nGreenSum += p.g;
        (*ppNode)->nBlueSum += p.b;
        (*ppNode)->nAlphaSum += p.a;
    }

    // Recurse a level deeper if the Node is not a leaf
    else {
        shift = 7 - nLevel;
        nIndex = (((p.r & mask[nLevel]) >> shift) << 3) |
			(((p.g & mask[nLevel]) >> shift) << 2) |
            (((p.b & mask[nLevel]) >> shift) << 1) |
            ((p.a & mask[nLevel]) >> shift);
        AddColor (&((*ppNode)->pChild[nIndex]), p, nColorBits,
                  nLevel + 1, pLeafCount, pReducibleNodes);
    }
}

Node* CreateNode (unsigned nLevel, unsigned nColorBits, unsigned* pLeafCount,
                  Node** pReducibleNodes)
{
    Node* pNode = new Node;
   	memset(pNode, 0, sizeof(Node));
    pNode->bIsLeaf = (nLevel == nColorBits) ? true : false;
    if (pNode->bIsLeaf)
        (*pLeafCount)++;
    else { // Add the Node to the reducible list for this level
        pNode->pNext = pReducibleNodes[nLevel];
        pReducibleNodes[nLevel] = pNode;
    }
    return pNode;
}

void ReduceTree (unsigned nColorBits, unsigned* pLeafCount, Node** pReducibleNodes) {
	int i;
    Node* pNode;
    unsigned nRedSum, nGreenSum, nBlueSum, nAlphaSum, nChildren;

    // Find the deepest level containing at least one reducible Node
    for (i=nColorBits - 1; (i>0) && (pReducibleNodes[i] == NULL); i--);

    // Reduce the Node most recently added to the list at level i
    pNode = pReducibleNodes[i];
    pReducibleNodes[i] = pNode->pNext;

    nRedSum = nGreenSum = nBlueSum = nAlphaSum = nChildren = 0;
    for (i = 0; i < 16; i++) {
        if (pNode->pChild[i] != NULL) {
            nRedSum += pNode->pChild[i]->nRedSum;
            nGreenSum += pNode->pChild[i]->nGreenSum;
            nBlueSum += pNode->pChild[i]->nBlueSum;
            nAlphaSum += pNode->pChild[i]->nAlphaSum;
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
    pNode->nAlphaSum = nAlphaSum;
    *pLeafCount -= (nChildren - 1);
}

void DeleteTree (Node** ppNode)
{
    for (unsigned i = 0; i < 16; i++) {
        if ((*ppNode)->pChild[i] != NULL)
            DeleteTree (&((*ppNode)->pChild[i]));
    }
    delete *ppNode;
    *ppNode = NULL;
}

void GetPaletteColors (Node* pTree, ref<Palette> destPalette, PixelFormat &pixelFormat, unsigned* pIndex) {
    if (pTree->bIsLeaf) {
		destPalette->addColor(Pixel(
			(uint8_t) ((pTree->nRedSum) / (pTree->nPixelCount)),
			(uint8_t) ((pTree->nGreenSum) / (pTree->nPixelCount)),
			(uint8_t) ((pTree->nBlueSum) / (pTree->nPixelCount)),
			(uint8_t) ((pTree->nAlphaSum) / (pTree->nPixelCount))));
        (*pIndex)++;
    }
    else {
        for (int i = 0; i < 16; i++) {
            if (pTree->pChild[i] != NULL)
                GetPaletteColors (pTree->pChild[i], destPalette, pixelFormat, pIndex);
        }
    }
}
