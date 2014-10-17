#include "pch.h"

#include <Kore/Application.h>
#include <Kore/IO/FileReader.h>
#include "SimpleGraphics.h"

using namespace Kore;

namespace {
	void update() {
		startFrame();

		// Draw something right here using setPixel(...)

		endFrame();
	}
}

int kore(int argc, char** argv) {
	Application* app = new Application(argc, argv, width, height, false, "Exercise1");
	
	initGraphics();
	
	app->setCallback(update);
	app->start();
	delete app;
	
	return 0;
}
