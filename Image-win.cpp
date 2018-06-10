#include "ConsoleGraphics.h"
#include <windows.h>
#include <gdiplus.h>

static bool bGdiplusStarted = false;

static void InitGDIPlus() {
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	// Initialize GDI+.
	if (!bGdiplusStarted)
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	bGdiplusStarted = true;
}

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

   Gdiplus::GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for (UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

static void toW(const char *string, WCHAR *dest, int destSize) {
	MultiByteToWideChar(CP_ACP, 0, string, -1, dest, destSize);
}

Image::Image(const char *file, Pixel transparentColor) : width(0), height(0), pixelData(0), associatedPalette(0) {
	WCHAR wfilename[1024];
	// Load an image via GDI+
	InitGDIPlus();
	toW(file, wfilename, sizeof(wfilename));
	Gdiplus::Bitmap image(wfilename);
	if (image.GetWidth() == 0) {
		fprintf(stderr, "Image %s could not be loaded\n", file);
		throw "Image could not be loaded";
	}

	width = image.GetWidth();
	height = image.GetHeight();
	pixelData = new Pixel[height * width];

	Gdiplus::Rect rt(0, 0, width, height);
	Gdiplus::BitmapData data;
	image.LockBits(&rt, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data);
	uint32_t *ptr = (uint32_t*) data.Scan0;
	for (unsigned j = 0; j < height; j++) {
		for (unsigned i = 0; i < width; i++) {
			if (ptr[i] == transparentColor.value)
				pixelData[j * width + i] = 0;
			else
				pixelData[j * width + i] = ptr[i];
		}
		ptr = (uint32_t*) ((uint8_t*) ptr + data.Stride);
	}
	image.UnlockBits(&data);
}

void Image::writeToFile(const char *filename) const {
	CLSID pngClsid;
	WCHAR wfilename[1024];
	Gdiplus::Bitmap bmp(width, height);
	Gdiplus::Rect rt(0, 0, width, height);
	Gdiplus::BitmapData data;
	// Copy pixels
	bmp.LockBits(&rt, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data);
	uint32_t *ptr = (uint32_t*) data.Scan0;
	for (unsigned j = 0; j < height; j++) {
		for (unsigned i = 0; i < width; i++) {
			uint32_t pixel = pixelData[j * width + i].value;
			if (associatedPalette) {
				// Indexed
				ptr[i] = associatedPalette->getColor(pixel).value;
			} else {
				// Normal
				ptr[i] = pixel;
			}
		}
		ptr = (uint32_t*) ((uint8_t*) ptr + data.Stride);
	}
	bmp.UnlockBits(&data);
	// Save
	toW(filename, wfilename, sizeof(wfilename));
	GetEncoderClsid(L"image/png", &pngClsid);
	bmp.Save(wfilename, &pngClsid, NULL);
}
