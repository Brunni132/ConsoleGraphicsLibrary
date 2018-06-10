#include <Foundation/Foundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <AppKit/AppKit.h>
#include "ConsoleGraphics.h"

Image::Image(const char *file, Pixel transparentColor) : width(0), height(0), pixelData(0), associatedPalette(0) {
	NSData *data = [NSData dataWithContentsOfFile:[NSString stringWithCString:file encoding:NSUTF8StringEncoding]];
	if (!data) {
		fprintf(stderr, "Image %s could not be loaded\n", file);
		throw "Image could not be loaded";
	}
	
	CGImageRef myCGImage = [[[NSBitmapImageRep imageRepWithData: data] autorelease] CGImage];
	width = CGImageGetWidth(myCGImage);
	height = CGImageGetHeight(myCGImage);
	pixelData = new Pixel[height * width];

	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	void *imageData = malloc(height * width * 4);
	CGContextRef context = CGBitmapContextCreate(imageData, width, height, 8, 4 * width, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
	CGColorSpaceRelease(colorSpace);
	CGContextClearRect(context, CGRectMake(0, 0, width, height));
	CGContextDrawImage(context, CGRectMake(0, 0, width, height), myCGImage);
	uint32_t *ptr = (uint32_t*) imageData;
	for (unsigned j = 0; j < height; j++) {
		for (unsigned i = 0; i < width; i++) {
			if (ptr[i] == transparentColor.value)
				pixelData[j * width + i] = 0;
			else
				pixelData[j * width + i] = ptr[j * width + i];
		}
	}
	free(imageData);
}

void Image::writeToFile(const char *filename) const {
/*	CLSID pngClsid;
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
	bmp.Save(wfilename, &pngClsid, NULL);*/
}
