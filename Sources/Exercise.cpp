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
	Image* image;

	void update() {
		float t = (float)(System::time() - startTime);
		Kore::Audio::update();
		
		startFrame();

		// Replace this
		//clear(0, 0, 0);
		drawImage(image, (int)(sin(t) * 400), (int)(abs(sin(t * 1.5f)) * 470));

		endFrame();
	}
}

int kore(int argc, char** argv) {
	Application* app = new Application(argc, argv, width, height, 0, false, "Exercise2", true);
	
	initGraphics();
	app->setCallback(update);

	startTime = System::time();
	image = loadImage("irobert-fb.png");
	Kore::Mixer::init();
	Kore::Audio::init();
	Kore::Mixer::play(new SoundStream("back.ogg", true));

	app->start();

	destroyImage(image);
	delete app;
	
	return 0;
}
