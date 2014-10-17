#include "pch.h"
#include "SimpleGraphics.h"
#include <Kore/Application.h>
#include <Kore/IO/FileReader.h>
#include <Kore/Graphics/Graphics.h>
#include <Kore/Graphics/Shader.h>
#include <Kore/IO/FileReader.h>
#include <limits>

using namespace Kore;

namespace {
	Shader* vertexShader;
	Shader* fragmentShader;
	Program* program;
	TextureUnit tex;
	VertexBuffer* vb;
	IndexBuffer* ib;
	Texture* texture;
	int* image;
}

void startFrame() {
	image = (int*)texture->lock();
}

void setPixel(int x, int y, float red, float green, float blue) {
	int r = (int)(red * 255);
	int g = (int)(green * 255);
	int b = (int)(blue * 255);
#ifdef OPENGL
	image[y * texture->width + x] = 0xff << 24 | b << 16 | g << 8 | r;
#else
	image[y * texture->width + x] = 0xff << 24 | r << 16 | g << 8 | b;
#endif
}

void endFrame() {
	texture->unlock();

	Graphics::begin();
	Graphics::clear(0);

	program->set();
	texture->set(tex);
	vb->set();
	ib->set();
	Graphics::drawIndexedVertices();

	Graphics::end();
	Graphics::swapBuffers();
}

void initGraphics() {
	FileReader vs("shader.vert");
	FileReader fs("shader.frag");
	vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
	fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);
	VertexStructure structure;
	structure.add("pos", Float3VertexData);
	structure.add("tex", Float2VertexData);
	program = new Program;
	program->setVertexShader(vertexShader);
	program->setFragmentShader(fragmentShader);
	program->link(structure);

	tex = program->getTextureUnit("tex");

	vb = new VertexBuffer(4, structure);
	float* v = vb->lock();
	{
		int i = 0;
		v[i++] = -1; v[i++] = 1; v[i++] = 0.5; v[i++] = 0; v[i++] = 0;
		v[i++] = 1;  v[i++] = 1; v[i++] = 0.5; v[i++] = 1; v[i++] = 0;
		v[i++] = 1; v[i++] = -1;  v[i++] = 0.5; v[i++] = 1; v[i++] = 1;
		v[i++] = -1; v[i++] = -1;  v[i++] = 0.5; v[i++] = 0; v[i++] = 1;
	}
	vb->unlock();

	ib = new IndexBuffer(6);
	int* ii = ib->lock();
	{
		int i = 0;
		ii[i++] = 0; ii[i++] = 1; ii[i++] = 3;
		ii[i++] = 1; ii[i++] = 2; ii[i++] = 3;
	}
	ib->unlock();

	texture = new Texture(width, height, Image::Format::RGBA32, false);
	image = (int*)texture->lock();
	for (int y = 0; y < texture->height; ++y) {
		for (int x = 0; x < texture->width; ++x) {
			image[y * texture->width + x] = 0;
		}
	}
	texture->unlock();
}
