// The Tofita Kernel
// Copyright (C) 2019  Oleg Petrenko
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Performs visualization onto the screen

// Speed of rendering mostly depends on cache-locality
// Remember: top-down, left-to-right: for(y) for(x) {}, not other way!

Framebuffer *_framebuffer;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} __attribute__((packed)) PixelRGBAData;

typedef struct {
	union {
		PixelRGBAData rgba;
		uint32_t color;
	};
} __attribute__((packed)) Pixel32;

typedef struct {
	uint16_t width;
	uint16_t height;
	Pixel32 pixels[];
} Bitmap32;

// Avoid one level of pointer indirection
Pixel32 *_pixels;

Bitmap32* allocateBitmapFromBuffer(uint16_t width, uint16_t height) {
	Bitmap32* result = (Bitmap32*)allocateFromBuffer(sizeof(uint16_t) * 2 + sizeof(Pixel32) * width * height);
	result->width = width;
	result->height = height;
	return result;
}

function setFramebuffer(const Framebuffer *framebuffer) {
	_framebuffer = const_cast<Framebuffer *>(framebuffer);
	_pixels = (Pixel32 *)_framebuffer->base;
}

// Very fast, but not precise, alpha multiply
#define Mul255(a255, c255) (((uint32_t)a255 + 1) * (uint32_t)c255 >> 8)
#define Blend255(target, color, alpha) (Mul255(alpha, color) + Mul255(255 - alpha, target))

function __attribute__((fastcall)) blendPixel(uint16_t x, uint16_t y, Pixel32 pixel) {
	if ((x > _framebuffer->width - 1) || (y > _framebuffer->height - 1)) return ;
	Pixel32 p = _pixels[y * _framebuffer->width + x];

	p.rgba.r = Mul255(pixel.rgba.a, pixel.rgba.r) + Mul255(255 - pixel.rgba.a, p.rgba.r);
	p.rgba.g = Mul255(pixel.rgba.a, pixel.rgba.g) + Mul255(255 - pixel.rgba.a, p.rgba.g);
	p.rgba.b = Mul255(pixel.rgba.a, pixel.rgba.b) + Mul255(255 - pixel.rgba.a, p.rgba.b);

	_pixels[y * _framebuffer->width + x] = p;
}

function __attribute__((fastcall)) setPixel(uint16_t x, uint16_t y, Pixel32 pixel) {
	if ((x > _framebuffer->width - 1) || (y > _framebuffer->height - 1)) return ;
	_pixels[y * _framebuffer->width + x] = pixel;
}

function drawBitmap32WithAlpha(Bitmap32* bitmap, uint16_t x, uint16_t y) {
	for (int32_t yy = 0; yy < bitmap->height; yy++) {
		for (int32_t xx = 0; xx < bitmap->width; xx++) {
			blendPixel(x + xx, y + yy, bitmap->pixels[yy * bitmap->width + xx]);
		}
	}
}

function drawBitmap32(Bitmap32* bitmap, uint16_t x, uint16_t y) {
	for (int32_t yy = 0; yy < bitmap->height; yy++) {
		for (int32_t xx = 0; xx < bitmap->width; xx++) {
			setPixel(x + xx, y + yy, bitmap->pixels[yy * bitmap->width + xx]);
		}
	}
}

function drawRectangleWithAlpha(Pixel32 color, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	for (int32_t yy = 0; yy < height; yy++) {
		for (int32_t xx = 0; xx < width; xx++) {
			blendPixel(x + xx, y + yy, color);
		}
	}
}

function drawRectangle(Pixel32 color, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	for (int32_t yy = 0; yy < height; yy++) {
		for (int32_t xx = 0; xx < width; xx++) {
			setPixel(x + xx, y + yy, color);
		}
	}
}

function drawRectangleOutline(Pixel32 color, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	for (int32_t yy = 0; yy < height; yy++) {
		for (int32_t xx = 0; xx < width; xx++) {
			// Rendering left and far right points sequentally should be
			// better for cache-locality than vertical lines
			// At least this is true for small rectangles (like buttons)
			if (yy == 0 || xx == 0 || xx == width - 1 || yy == height - 1) setPixel(x + xx, y + yy, color);
		}
	}
}

function line45smooth(Pixel32 color, int32_t x, int32_t y, int32_t width, int32_t mod) {
	color.rgba.a = 98;
	int32_t xx = 0;
	for (int32_t xi = 0; xi < width - 1; xi++) {
		xx += mod;
		setPixel(xx + x, y + xi, color);
		blendPixel(xx + x, y + xi + 1, color);
		blendPixel(xx + x + mod, y + xi, color);
	}
	xx += mod;
	setPixel(xx + x, y + width - 1, color);
}

