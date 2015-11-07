#include "pch.h"

#include <Kore/Application.h>
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
	Application* app = new Application(argc, argv, width, height, 0, false, "Exercise2", true);
	
	initGraphics();
	app->setCallback(update);

	startTime = System::time();
	image = loadTexture("irobert-fb.png");
	Kore::Mixer::init();
	Kore::Audio::init();
	Kore::Mixer::play(new SoundStream("back.ogg", true));

	app->start();

	destroyTexture(image);
	delete app;
	
	return 0;
}
