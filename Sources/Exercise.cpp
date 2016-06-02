#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Audio/Mixer.h>
#include "SimpleGraphics.h"

using namespace Kore;

namespace {
	double startTime;
	Texture* image;

	void update() {
		float t = (float)(System::time() - startTime);
		Kore::Audio::update();
		
		startFrame();

		/************************************************************************/
		/* Exercise 2, Practical Task:   
		/* Add some interesting animations or effects here          
		/************************************************************************/
		// clear(0, 0, 0);
		drawTexture(image, (int)(sin(t) * 400), (int)(abs(sin(t * 1.5f)) * 470));

		endFrame();
	}
}

int kore(int argc, char** argv) {

	Kore::System::setName("TUD Game Technology - ");
	Kore::System::setup();
	Kore::WindowOptions options;
	options.title = "Exercise 2";
	options.width = width;
	options.height = height;
	options.x = 100;
	options.y = 100;
	options.targetDisplay = -1;
	options.mode = WindowModeWindow;
	options.rendererOptions.depthBufferBits = 16;
	options.rendererOptions.stencilBufferBits = 8;
	options.rendererOptions.textureFormat = 0;
	options.rendererOptions.antialiasing = 0;
	Kore::System::initWindow(options);

	initGraphics();
	Kore::System::setCallback(update);


	
	startTime = System::time();
	
	image = loadTexture("irobert-fb.png");
	Kore::Mixer::init();
	Kore::Audio::init();
	Kore::Mixer::play(new SoundStream("back.ogg", true));

	Kore::System::start();

	destroyTexture(image);
	
	return 0;
}
