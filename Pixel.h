#ifndef PIXEL_H
#define PIXEL_H

#ifndef ABS
extern inline unsigned ABS(signed v) {
	return v >= 0 ? v : -v;
}
#endif

union Pixel {
	uint32_t index;		// (when using a palette)
	struct {
		uint8_t b, g, r, a;
	};
	uint32_t value;
	Pixel() : value(0) {}
	Pixel(uint8_t r, uint8_t g, uint8_t b)
		: r(r), g(g), b(b), a(255) {}
	Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: r(r), g(g), b(b), a(a) {}
	Pixel(uint32_t value) : value(value) {}

	unsigned colorDeltaWith(Pixel p) {
		unsigned dr = ABS((signed) r - (signed) p.r),
			dg = ABS((signed) g - (signed) p.g),
			db = ABS((signed) b - (signed) p.b),
			da = ABS((signed) a - (signed) p.a);
		return dr * dr + dg * dg + db * db + da * da;
	}
};

#endif
