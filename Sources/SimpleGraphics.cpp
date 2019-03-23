#include "pch.h"
#include "SimpleGraphics.h"
#include <Kore/IO/FileReader.h>
#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics4/Shader.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/IO/FileReader.h>
#include <limits>

using namespace Kore;

namespace {
	Graphics4::Shader* vertexShader;
	Graphics4::Shader* fragmentShader;
	Graphics4::PipelineState* program;
	Graphics4::TextureUnit tex;
	Graphics4::VertexBuffer* vb;
	Graphics4::IndexBuffer* ib;
	Graphics4::Texture* texture;
	int* image;
}

void startFrame() {
	Graphics4::begin();
	Graphics4::clear(Graphics4::ClearColorFlag, 0xff000000);

	image = (int*)texture->lock();
}

#ifdef DIRECT3D
#define CONVERT_COLORS(red, green, blue) int r = (int)(blue * 255); int g = (int)(green * 255); int b = (int)(red * 255);
#else
#define CONVERT_COLORS(red, green, blue) int r = (int)(red * 255); int g = (int)(green * 255); int b = (int)(blue * 255);
#endif

void clear(float red, float green, float blue) {
	CONVERT_COLORS(red, green, blue);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			image[y * texture->width + x] = 0xff << 24 | b << 16 | g << 8 | r;
		}
	}
}


void setPixel(int x, int y, float red, float green, float blue, float alpha /* = 1.0f */) {
	if (y < 0 || y >= texture->texHeight || x < 0 || x >= texture->texWidth) {
		return;
	}
	
	int col = image[y * texture->texWidth + x];

// #ifdef OPENGL
	float bi = ((col >> 16) & 0xff) / 255.0f;
	float gi = ((col >> 8)  & 0xff) / 255.0f;
	float ri = (col & 0xff) / 255.0f;
//#else
	/* float ri = ((col >> 16) & 0xff) / 255.0f;
	float gi = ((col >> 8)  & 0xff) / 255.0f;
	float bi = (col & 0xff) / 255.0f; */
// #endif

	float ar = alpha * red + (1.0f - alpha) * ri;
	float ag = alpha * green + (1.0f - alpha) * gi;
	float ab = alpha * blue + (1.0f - alpha) * bi;

	int r = (int)(ar * 255);
	int g = (int)(ag * 255);
	int b = (int)(ab * 255);

//#ifdef OPENGL
	image[y * texture->texWidth + x] = 0xff << 24 | b << 16 | g << 8 | r;
//#else
	//image[y * texture->texWidth + x] = 0xff << 24 | r << 16 | g << 8 | b;
//#endif
}

Kore::Graphics4::Texture* loadTexture(const char* filename) {
	return new Graphics4::Texture(filename, true);
}

void destroyTexture(Kore::Graphics4::Texture* image) {
	delete image;
}

void drawTexture(Kore::Graphics4::Texture* inImage, int x, int y) {
	int ystart = max(0, -y);
	int xstart = max(0, -x);
	int h = min(inImage->height, height - y);
	int w = min(inImage->width, width - x);
	for (int yy = ystart; yy < h; ++yy) {
		for (int xx = xstart; xx < w; ++xx) {
			int col = readPixel(inImage, xx, yy);

			float a = ((col >> 24) & 0xff) / 255.0f;
			float r = ((col >> 16) & 0xff) / 255.0f;
			float g = ((col >> 8)  & 0xff) / 255.0f;
			float b = (col & 0xff) / 255.0f;
			setPixel(x + xx, y + yy, r, g, b, a);
		}
	}
}

int readPixel(Kore::Graphics4::Texture* image, int x, int y) {
	int c = *(int*)&((u8*)image->data)[image->texWidth * 4 * y + x * 4];
	int a = (c >> 24) & 0xff;

	int b = (c >> 16) & 0xff;
	int g = (c >> 8) & 0xff;
	int r = c & 0xff;

	return a << 24 | r << 16 | g << 8 | b;
}

void endFrame() {
	texture->unlock();
	
	Kore::Graphics4::setPipeline(program);
	Graphics4::setTexture(tex, texture);
	Graphics4::setVertexBuffer(*vb);
	Graphics4::setIndexBuffer(*ib);
	Graphics4::drawIndexedVertices();

	Graphics4::end();
	Graphics4::swapBuffers();
}

void initGraphics() {
	FileReader vs("shader.vert");
	FileReader fs("shader.frag");
	vertexShader = new Kore::Graphics4::Shader(vs.readAll(), vs.size(), Kore::Graphics4::VertexShader);
	fragmentShader = new Kore::Graphics4::Shader(fs.readAll(), fs.size(), Kore::Graphics4::FragmentShader);
	Kore::Graphics4::VertexStructure structure;
	structure.add("pos", Kore::Graphics4::Float3VertexData);
	structure.add("tex", Kore::Graphics4::Float2VertexData);
	program = new Kore::Graphics4::PipelineState();
	program->vertexShader = vertexShader;
	program->fragmentShader = fragmentShader;
	program->inputLayout[0] = &structure;
	program->inputLayout[1] = nullptr;
	program->compile();

	tex = program->getTextureUnit("tex");

	texture = new Kore::Graphics4::Texture(width, height, Kore::Graphics4::Image::RGBA32, false);
	image = (int*)texture->lock();
	for (int y = 0; y < texture->texHeight; ++y) {
		for (int x = 0; x < texture->texWidth; ++x) {
			image[y * texture->texWidth + x] = 0;
		}
	}
	texture->unlock();

	// Correct for the difference between the texture's desired size and the actual power of 2 size
	float xAspect = (float)texture->width / texture->texWidth;
	float yAspect = (float)texture->height / texture->texHeight;


	vb = new Kore::Graphics4::VertexBuffer(4, structure);
	float* v = vb->lock();
	{
		int i = 0;
		v[i++] = -1; v[i++] = 1; v[i++] = 0.5; v[i++] = 0; v[i++] = 0;
		v[i++] = 1;  v[i++] = 1; v[i++] = 0.5; v[i++] = xAspect; v[i++] = 0;
		v[i++] = 1; v[i++] = -1;  v[i++] = 0.5; v[i++] = xAspect; v[i++] = yAspect;
		v[i++] = -1; v[i++] = -1;  v[i++] = 0.5; v[i++] = 0; v[i++] = yAspect;
	}
	vb->unlock();

	ib = new Kore::Graphics4::IndexBuffer(6);
	int* ii = ib->lock();
	{
		int i = 0;
		ii[i++] = 0; ii[i++] = 1; ii[i++] = 3;
		ii[i++] = 1; ii[i++] = 2; ii[i++] = 3;
	}
	ib->unlock();
}
