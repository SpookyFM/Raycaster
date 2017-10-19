#include "pch.h"

#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Audio/Mixer.h>
#include "SimpleGraphics.h"
#include <Kore/Input/Keyboard.h>
#include <Kore/Log.h>
#include <cstring>
#include <cmath>
#include "Kore/Graphics/Texture.h"
#include <cassert>

using namespace Kore;

// Level definition. A level is made up of LevelWidth * LevelHeight cells that have a size of CellSize. If a cell is filled with a 
// value of (0, 0, 0), it is empty. Otherwise, it is a wall with the specified color.
static constexpr unsigned int LevelWidth = 10;
static constexpr unsigned int LevelHeight = 10;

float CellSize = 100.0f;

Kore::vec3 Level[LevelWidth][LevelHeight];

Kore::vec2 CurrentPosition = Kore::vec2(
	LevelWidth * CellSize * 0.5f,
	LevelHeight * CellSize * 0.5f
);

float CurrentAngle;

bool KeyLeftDown;
bool KeyRightDown;
bool KeyUpDown;
bool KeyDownDown;

const float TurningSpeed = 2.0f;
const float WalkingSpeed = 100.0f;

const float DistanceFactor = 25.0f * 512.0f;


Kore::Texture* wallTexture; 

namespace {
	double startTime;
	Texture* image;

	/** Initializes the map with colors and empty cells */
	void SetupMap()
	{
		std::memset(&Level, 0, sizeof(Kore::vec3)*LevelWidth*LevelHeight);
		const Kore::vec3 colorNorth = Kore::vec3(1.0f, 0.0f, 0.0f);
		const Kore::vec3 colorEast = Kore::vec3(0.0f, 1.0f, 0.0f);
		const Kore::vec3 colorSouth = Kore::vec3(0.0f, 0.0f, 1.0f);
		const Kore::vec3 colorWest = Kore::vec3(1.0f, 0.0f, 1.0f);
		const Kore::vec3 colorInterior = Kore::vec3(1.0f, 1.0f, 1.0f);
		// Let's have the outsides filled with walls
		int NumWallCells = 0;
		for (unsigned int x = 0; x < LevelWidth; x++)
		{
			for (unsigned int y = 0; y < LevelHeight; y++)
			{
				if (y == 0)
				{
					Level[x][y] = colorNorth;
					NumWallCells++;
				}
				if (y == LevelHeight - 1)
				{
					Level[x][y] = colorSouth;
					NumWallCells++;
				}
				if (x == 0)
				{
					Level[x][y] = colorWest;
					NumWallCells++;
				}
				if (x == LevelWidth - 1)
				{
					Level[x][y] = colorEast;
					NumWallCells++;
				}
			}
		}
	}
	
	float RadToDegrees(float Angle)
	{
		return Angle * 180.0f / Kore::pi;
	}

	float WrapAngle(float Angle)
	{
		float Result = Angle;
		// @@TODO: Should be faster
		while (Result < 0.0f)
		{
			Result += Kore::pi * 0.5f;
		}
		while (Result > Kore::pi * 0.5f)
		{
			Result -= Kore::pi;
		}
		return Result;
	}

	Kore::vec2 GetForwardVector(float Angle)
	{
		return Kore::vec2(
			Kore::cos(Angle),
			-Kore::sin(Angle)
		);
	}

	Kore::vec2i GetCell(Kore::vec2 Position)
	{
		Kore::vec2i result;
		result[0] = (int)Kore::floor(Position.x() / CellSize);
		result[1] = (int)Kore::floor(Position.y() / CellSize);
		return result;
	}

	Kore::vec3 GetColor(Kore::vec2i Cell)
	{
		return Level[Cell.x()][Cell.y()];
	}

	Kore::vec2 GetPositionInCurrentCell(Kore::vec2 Position)
	{
		Kore::vec2i CurrentCell = GetCell(Position);
		Kore::vec2 CurrentOrigin = Kore::vec2((float)CurrentCell.x(), (float)CurrentCell.y()) * CellSize;
		return Position - CurrentOrigin;
	}

	bool IsSolid(const Kore::vec3& Color)
	{
		return Color.x() > 0.0f || Color.y() > 0.0f || Color.z() > 0.0f;
	}

	/** Casts a ray and returns the color of the wall at this position */
	// Coordinate system has the point (0, 0) at the top-left corner of the map
	// Increasing x to the right
	// Increasing y to the bottom
	// Note: This means that we have to account for the vertical direction of the unit circle being the other way around than the coordinate system
	float CastRay(Kore::vec2 Position, float Angle, Kore::vec3& Result, Kore::vec2i& HitCell, int& TexCoordX, bool Debug = false)
	{
		if (Debug) Kore::log(Info, "Position %.2f%.2f, Angle %.2f", Position.x(), Position.y(), RadToDegrees(Angle));
		// Check for edge cases
		float fmodResult = fmod(Angle, (Kore::pi * 0.5f));
		float d = Kore::abs(fmodResult / (Kore::pi * 0.5f));

		float WrappedAngle = Angle;
		float SinAngle = Kore::sin(WrappedAngle);
		float CosAngle = Kore::cos(WrappedAngle);
		float TanAngle = SinAngle / CosAngle;
		const float epsilon = 0.01f;
		if (d < epsilon || Kore::abs(1.0f - d) < epsilon)
		{
			// Worry about those later
			return CellSize * (LevelHeight * LevelWidth);
		}


		Kore::vec2i CurrentCell = GetCell(Position);
		// We want to check the intersections with the grid cells
		const Kore::vec2 PositionInCurrentCell = GetPositionInCurrentCell(Position);
		// First, advance horizontally

		// In which direction are we pointing?
		int SignHorizontal = Kore::cos(WrappedAngle) > 0.0f ? 1 : -1;
		int SignVertical = Kore::sin(WrappedAngle) > 0.0f ? -1 : 1;


		if (Debug) Kore::log(Info, "Tan: %f, SignHorizontal %i, SignVertical %i", TanAngle, SignHorizontal, SignVertical);

		// Compute the delta to the first intersection
		float FirstDeltaX = SignHorizontal > 0.0f ? CellSize - PositionInCurrentCell.x() : -PositionInCurrentCell.x();
		// On the y-axis, we need to multiply with -1 in order to account for the coordinate system being the other way around than the unit circle.
		float FirstDeltaY = -TanAngle * FirstDeltaX;

		// This is the first intersection horizontally
		Kore::vec3 HorizontalColor;
		float HorizontalDistance = -1.0f;

		if (Debug) Kore::log(Info, "Testing horizontal: %0.2f|%0.2f", CurrentPosition.x() + FirstDeltaX, CurrentPosition.y() + FirstDeltaY);
		Kore::vec2i TestCell = Kore::vec2i(
			//CurrentCell.x() + SignHorizontal,
			(int)Kore::floor((Position.x() + FirstDeltaX) / CellSize),
			(int)Kore::floor((Position.y() + FirstDeltaY) / CellSize)
		);

		Kore::vec2i HitHorizontalCell;

		Kore::vec3 CurrentColorHorizontal = GetColor(TestCell);
		if (IsSolid(CurrentColorHorizontal))
		{
			HorizontalColor = CurrentColorHorizontal;
			HorizontalDistance = FirstDeltaX;
			Kore::log(Info, "Horizontal: Hit test on first attempt.");
		}
		else
		{
			// We need to iterate from this point
			unsigned int NumHorizontalChecks = SignHorizontal > 0 ? LevelWidth - CurrentCell.x() : CurrentCell.x();
			// Inverted due to inverted y-axis
			float DeltaY = -TanAngle * CellSize * SignVertical;
			float CurrentY = Position.y() + FirstDeltaY;
			// If we walk to the left, we are checking for the right side of the cell
			//TestCell[0] += SignHorizontal < 0 ? -1 : 0;
			for (unsigned int i = 0; i < NumHorizontalChecks; i++)
			{
				TestCell[0] += SignHorizontal;
				CurrentY += DeltaY;
				TestCell[1] = (int)Kore::floor(CurrentY / CellSize);
				if (Debug) Kore::log(Info, "Testing horizontal: %0.2f|%0.2f", TestCell.x() * CellSize, CurrentY);
				if (TestCell.y() < 0 || TestCell.y() >= LevelHeight)
				{
					if (Debug) Kore::log(Info, "Horizontal: Out of bounds.");
					break;
				}
				CurrentColorHorizontal = GetColor(TestCell);
				if (IsSolid(CurrentColorHorizontal))
				{
					HorizontalDistance = FirstDeltaX + CellSize * (i + 1) * SignHorizontal;
					HorizontalColor = CurrentColorHorizontal;
					HitHorizontalCell = TestCell;
					Kore::vec2 HitPoint(TestCell[0] * CellSize, CurrentY);
					float TestHorizontalDistance = HitPoint[0] - CurrentPosition[0];
					// assert(Kore::abs(TestHorizontalDistance - HorizontalDistance) < 0.5f);
					if (Debug) Kore::log(Info, "Horizontal: Hit at point: %.2f|%.2f, distance %.2f", HitPoint.x(), HitPoint.y(), (HitPoint - CurrentPosition).getLength());
					break;
				}
			}
		}

		Kore::vec3 VerticalColor;
		float VerticalDistance = -1.0f;
		Kore::vec2i HitVerticalCell;
		
		FirstDeltaY = SignVertical > 0.0f ? CellSize - PositionInCurrentCell.y() : -PositionInCurrentCell.y();
		FirstDeltaX = -FirstDeltaY / TanAngle;

		if (Debug) Kore::log(Info, "Testing vertical: %0.2f|%0.2f", CurrentPosition.x() + FirstDeltaX, CurrentPosition.y() + FirstDeltaY);

		// Now, the same for vertical
		TestCell = Kore::vec2i(
			(int)Kore::floor((Position.x() + FirstDeltaX) / CellSize),
			CurrentCell.y() + SignVertical
		);

		Kore::vec3 CurrentColorVertical = GetColor(TestCell);
		if (IsSolid(CurrentColorVertical))
		{
			VerticalColor = CurrentColorVertical;
			VerticalDistance = FirstDeltaY;
			Kore::log(Info, "Vertical: Hit test on first attempt.");
		}
		else
		{
			// We need to iterate from this point
			// If we are moving up (SignVertical = -1) we are hitting the lower bounds of the cell, if we are moving down (SignVertical = 1) we are hitting the top of the cell
			unsigned int NumVerticalChecks = SignVertical > 0 ? LevelHeight - CurrentCell.y() : CurrentCell.y();
			float DeltaX = -CellSize / TanAngle * SignHorizontal;
			float CurrentX = Position.x() + FirstDeltaX;
			// Adjust for the direction
			// TestCell[1] += SignVertical > 0 ? 0 : 1;
			for (unsigned int i = 0; i < NumVerticalChecks; i++)
			{
				TestCell[1] += SignVertical;
				CurrentX += DeltaX;
				TestCell[0] = (int)Kore::floor(CurrentX / CellSize);
				if (Debug) Kore::log(Info, "Testing vertical: %0.2f|%0.2f", CurrentX, TestCell[1] * CellSize);
				if (TestCell.x() < 0 || TestCell.x() >= LevelWidth)
				{
					if (Debug) Kore::log(Info, "Vertical: Out of bounds.");
					break;
				}
				CurrentColorVertical = GetColor(TestCell);
				if (IsSolid(CurrentColorVertical))
				{
					//VerticalDistance = FirstDeltaY + CellSize * (i + 1) * SignVertical;
					VerticalDistance = FirstDeltaY + CellSize * (i + 1) * SignVertical;
					VerticalColor = CurrentColorVertical;
					HitVerticalCell = TestCell;
					Kore::vec2 HitPoint(CurrentX, TestCell[1] * CellSize);
					if (Debug) Kore::log(Info, "Vertical: Hit at point: %.2f|%.2f, distance %.2f", HitPoint.x(), HitPoint.y(), (HitPoint - CurrentPosition).getLength());
					break;
				}
			}
		}

		float Distance = CellSize * Kore::max(LevelWidth, LevelHeight);
		if (IsSolid(HorizontalColor))
		{
			float Y = -TanAngle * HorizontalDistance;
			// Distance = Kore::sqrt(HorizontalDistance * HorizontalDistance + Y * Y);
			Distance = Kore::cos(CurrentAngle) * HorizontalDistance - Kore::sin(CurrentAngle) * Y;
			Result = HorizontalColor;
			HitCell = HitHorizontalCell;
			float yHitPosition = fmod(CurrentPosition.y() + Y, 100.0f) / 100.0f;
			TexCoordX = (int)(yHitPosition * 64.0f);
		} 
		if (IsSolid(VerticalColor))
		{
			float X = -VerticalDistance / TanAngle;
			//float TestDistance = Kore::sqrt(VerticalDistance * VerticalDistance + X * X);
			float TestDistance = Kore::cos(CurrentAngle) * X - Kore::sin(CurrentAngle) * VerticalDistance;
			if (TestDistance < Distance)
			{
				Distance = TestDistance;
				Result = VerticalColor;
				HitCell = HitVerticalCell;
				float xHitPosition = fmod(CurrentPosition.x() + X, 100.0f) / 100.0f;
				TexCoordX = (int)(xHitPosition * 64.0f);
			}
		}
		return Distance;
	}


	void DrawVerticalLine(const Kore::vec3& Color, int X, int LineHeight)
	{
		for (int y = 0; y < LineHeight; y++)
		{
			setPixel(X, ((height - LineHeight) / 2) + y, Color.x(), Color.y(), Color.z());
		}
	}


	void DrawVerticalLine(const Kore::Texture* InTexture, int X, int texX, int LineHeight)
	{
		for (int y = 0; y < LineHeight; y++)
		{
			float fY = (float)y / (float)LineHeight;
			int texY = (int)(fY * wallTexture->texHeight);
			int col  = readPixel(wallTexture, texX, texY);
			float a = ((col >> 24) & 0xff) / 255.0f;
			float r = ((col >> 16) & 0xff) / 255.0f;
			float g = ((col >> 8)  & 0xff) / 255.0f;
			float b = (col & 0xff) / 255.0f;
			setPixel(X, ((height - LineHeight) / 2) + y, r, g, b, a);
		}
	}

	//@@TODO: Implement DeltaT
	void UpdateView(float DeltaT)
	{
		// Handle inputs
		if (KeyLeftDown)
		{
			CurrentAngle += TurningSpeed * DeltaT;
		}
		if (KeyRightDown)
		{
			CurrentAngle -= TurningSpeed * DeltaT;
		}
		if (KeyUpDown)
		{
			Kore::vec2 Forward = GetForwardVector(CurrentAngle);
			Kore::log(Info, "Forward Vector: %.2f|%.2f", Forward.x(), Forward.y());
			CurrentPosition += Forward * WalkingSpeed * DeltaT;
		}
		if (KeyDownDown)
		{
			Kore::vec2 Forward = GetForwardVector(CurrentAngle);
			CurrentPosition -= Forward * WalkingSpeed * DeltaT;
		}

		// Draw graphics
		float HalfFOV = Kore::pi * 0.25f;
		float StartAngle = CurrentAngle + HalfFOV;
		float DeltaAngle = -HalfFOV * 2.0f / (float)width;
		float CurrentRayAngle = StartAngle;
		Kore::vec3 CurrentColor;
		Kore::vec2i HitCell;
		int TexCoordX = 0;
		for (int X = 0; X < width; X++)
		{
			float Distance = CastRay(CurrentPosition, CurrentRayAngle, CurrentColor, HitCell, TexCoordX);
			// float LineHeight = DistanceFactor / Distance;
			float LineHeight = DistanceFactor / Distance;
			// DrawVerticalLine(CurrentColor, X, LineHeight);
			DrawVerticalLine(wallTexture, X, TexCoordX, (int) LineHeight);
			CurrentRayAngle += DeltaAngle;
		}

		// For debugging, calculate the current forward Angle
		// CurrentAngle = -0.2f;
		float TestDistance = CastRay(CurrentPosition, CurrentAngle, CurrentColor, HitCell, TexCoordX, true);
		Kore::vec2i Cell = GetCell(CurrentPosition);
		// Kore::log(Info, "DeltaT: %f", DeltaT);
		Kore::log(Info, "Player is at %.1f|%.1f, angle %.3f, cell %i|%i, hit cell %i|%i distance is %.2f", CurrentPosition.x(), CurrentPosition.y(), RadToDegrees(CurrentAngle), Cell.x(), Cell.y(), HitCell.x(), HitCell.y(), TestDistance);
		// Kore::log(Info, "Tex Coord X: %f", TexCoordX);
		// And draw a red line in the center
		DrawVerticalLine(Kore::vec3(1.0f, 0.0f, 0.0f), width / 2, height);
		//DrawVerticalLine(wallTexture, width / 2, 32, height);
	}


	float lastT = 0.0f;
	void update() {
		float t = (float)(System::time() - startTime);
		float deltaT = t - lastT;
		lastT = t;
		Kore::Audio::update();

		startFrame();

		/************************************************************************/
		/* Exercise 2, Practical Task:
		/* Add some interesting animations or effects here
		/************************************************************************/
		clear(0.0f, 0, 0);
		//drawTexture(image, (int)(sin(t) * 400), (int)(abs(sin(t * 1.5f)) * 470));
		UpdateView(deltaT);

		endFrame();
	}


}

void handleInput(KeyCode code, bool Value)
{
	if (code == Key_Left)
	{
		KeyLeftDown = Value;
	}
	else if (code == Key_Right)
	{
		KeyRightDown = Value;
	}
	else if (code == Key_Up)
	{
		KeyUpDown = Value;
	}
	else if (code == Key_Down)
	{
		KeyDownDown = Value;
	}
}

void keyDown(KeyCode code, wchar_t character) {
	handleInput(code, true);
}

void keyUp(KeyCode code, wchar_t character) {
	handleInput(code, false);
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

	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;

	SetupMap();
	Kore::System::setCallback(update);


	
	startTime = System::time();
	
	image = loadTexture("irobert-fb.png");
	wallTexture = loadTexture("finalredbrick1.png");
	Kore::Mixer::init();
	Kore::Audio::init();
	Kore::Mixer::play(new SoundStream("back.ogg", true));

	Kore::System::start();

	destroyTexture(image);
	
	return 0;
}
