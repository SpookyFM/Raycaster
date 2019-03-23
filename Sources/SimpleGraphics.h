#pragma once

namespace Kore {
	namespace Graphics4 {
		class Texture;
	}
}


void initGraphics();
void startFrame();
void endFrame();
void clear(float red, float green, float blue);
void setPixel(int x, int y, float red, float green, float blue, float alpha = 1.0f);
Kore::Graphics4::Texture* loadTexture(const char* filename);
void destroyTexture(Kore::Graphics4::Texture* image);
void drawTexture(Kore::Graphics4::Texture* image, int x, int y);
int readPixel(Kore::Graphics4::Texture* image, int x, int y);

// Watch out for resolutions that are higher than your monitor's resolution and for non-power-of-two sizes
const int width = 512;
const int height = 512;
