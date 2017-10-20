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

const float CellSize = 100.0f;
const float TextureSize = 64.0f;

int Level[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
				1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
				1, 0, 0, 1, 1, 1, 0, 0, 0, 1,
				1, 0, 0, 0, 0, 1, 0, 0, 0, 1,
				1, 0, 0, 0, 0, 1, 0, 0, 0, 1,
				1, 0, 0, 1, 1, 1, 0, 0, 0, 1,
				1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
				1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

Kore::vec2 CurrentPosition = Kore::vec2(
	LevelWidth * CellSize * 0.2f,
	LevelHeight * CellSize * 0.2f
);

float CurrentAngle;

bool KeyLeftDown;
bool KeyRightDown;
bool KeyUpDown;
bool KeyDownDown;

const float TurningSpeed = 2.0f;
const float WalkingSpeed = 100.0f;

const float DistanceFactor = 25.0f * 512.0f;

constexpr int NumTextures = 1;
Kore::Texture** Textures;
Kore::vec3* Colors;

namespace {
	double startTime;
	Texture* image;

	/** Initializes the map with colors and empty cells */
	/* void SetupMap()
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
	} */
	
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

	int GetIndex(const Kore::vec2i& Cell)
	{
		return Level[Cell.y() * LevelWidth + Cell.x()];
	}

	Kore::vec3 GetColor(const Kore::vec2i& Cell)
	{
		return Colors[GetIndex(Cell)];
	}

	Kore::Texture* GetTexture(const Kore::vec2i& Cell)
	{
		return Textures[GetIndex(Cell)];
	}

	Kore::vec2 GetPositionInCurrentCell(Kore::vec2 Position)
	{
		Kore::vec2i CurrentCell = GetCell(Position);
		Kore::vec2 CurrentOrigin = Kore::vec2((float)CurrentCell.x(), (float)CurrentCell.y()) * CellSize;
		return Position - CurrentOrigin;
	}

	bool IsSolid(int Index)
	{
		return Index > 0;
	}

	bool IsSolid(const Kore::vec3& Color)
	{
		return Color.x() > 0.0f || Color.y() > 0.0f || Color.z() > 0.0f;
	}

	int Sign(float Value)
	{
		float TestValue = Value + 0.0000001f;
		return Value >= 0.0f ? 1 : -1;
	}

	void AssertSign(float Value, int SignValue)
	{
		if (Kore::abs(Value) > 0.00000001f)
		{
			assert(Sign(Value) == SignValue);
		}
	}

	/** Casts a ray and returns the color of the wall at this position */
	// Coordinate system has the point (0, 0) at the top-left corner of the map
	// Increasing x to the right
	// Increasing y to the bottom
	// Note: This means that we have to account for the vertical direction of the unit circle being the other way around than the coordinate system
	float CastRay(Kore::vec2 Position, float Angle, int& Result, Kore::vec2i& HitCell, int& TexCoordX, bool Debug = false)
	{
		Result = -1;
		if (Debug) Kore::log(Info, "Position %.2f%.2f, Angle %.2f", Position.x(), Position.y(), RadToDegrees(Angle));
		// Check for edge cases at multiples of 90 degrees
		float fmodResult = fmod(Angle, (Kore::pi * 0.5f));
		float d = Kore::abs(fmodResult / (Kore::pi * 0.5f));

		float SinAngle = Kore::sin(Angle);
		float CosAngle = Kore::cos(Angle);
		float TanAngle = SinAngle / CosAngle;
		const float epsilon = 0.01f;
		if (d < epsilon || Kore::abs(1.0f - d) < epsilon)
		{
			// Worry about edge cases later
			return CellSize * (LevelHeight * LevelWidth);
		}
		// In which direction are we pointing?
		int SignHorizontal = CosAngle > 0.0f ? 1 : -1;
		int SignVertical = SinAngle > 0.0f ? -1 : 1;
		if (Debug) Kore::log(Info, "Tan: %f, SignHorizontal %i, SignVertical %i", TanAngle, SignHorizontal, SignVertical);

		// These values will indicate if we had a horizontal or vertical hit or both (in this case, the shorter distance is taken)
		int HorizontalIndex = -1;
		float HorizontalDistance = -1.0f;
		Kore::vec2i HitHorizontalCell;

		int VerticalIndex = -1;
		float VerticalDistance = -1.0f;
		Kore::vec2i HitVerticalCell;

		Kore::vec2i CurrentCell = GetCell(Position);
		const Kore::vec2 PositionInCurrentCell = GetPositionInCurrentCell(Position);

		//////////////////////////////////////////////////////////////////////////
		// First, advance horizontally and check for the first intersection with a cell border on the y-axis
		//////////////////////////////////////////////////////////////////////////
		// Compute the delta to the first intersection
		float FirstDeltaX = SignHorizontal > 0.0f ? CellSize - PositionInCurrentCell.x() : -PositionInCurrentCell.x();
		// On the y-axis, we need to multiply with -1 in order to account for the coordinate system being the other way around than the unit circle.
		float FirstDeltaY = -TanAngle * FirstDeltaX;
		AssertSign(FirstDeltaX, SignHorizontal);
		AssertSign(FirstDeltaY, SignVertical);

		// This is the first intersection horizontally
		// We find the x-value by just stepping one cell
		if (Debug) Kore::log(Info, "Testing horizontal: %0.2f|%0.2f", CurrentPosition.x() + FirstDeltaX, CurrentPosition.y() + FirstDeltaY);
		Kore::vec2i TestCell = Kore::vec2i(
			CurrentCell.x() + SignHorizontal,
			(int)Kore::floor((Position.y() + FirstDeltaY) / CellSize)
		);

		int CurrentIndexHorizontal = GetIndex(TestCell);
		if (IsSolid(CurrentIndexHorizontal))
		{
			HorizontalIndex = CurrentIndexHorizontal;
			HorizontalDistance = FirstDeltaX;
			HitHorizontalCell = TestCell;
			if (Debug) Kore::log(Info, "Horizontal: Hit result in initial test.");
		}
		else
		{
			// We need to iterate from this point
			// Inverted due to inverted y-axis
			float DeltaY = -TanAngle * CellSize * SignHorizontal;
			AssertSign(DeltaY, SignVertical);
			float CurrentY = Position.y() + FirstDeltaY;
			//@@TODO: Figure out what the upper limit of tests is actually - now, we are relying on the level to be bounded
			// At this point, we are at the first intersection at the border of our current cell
			// i counts the number of cells we have moved away from the first tested cell
			for (int i = 1;;i++)
			{
				// In x-direction, we are advancing one cell at a time
				TestCell[0] += SignHorizontal;
				// In y-direction, we are advancing the associated DeltaY
				CurrentY += DeltaY;
				TestCell[1] = (int)Kore::floor(CurrentY / CellSize);
				if (Debug) Kore::log(Info, "Testing horizontal cell: %i|%i", TestCell.x(), TestCell.y());
				if (TestCell.y() < 0 || TestCell.y() >= LevelHeight)
				{
					if (Debug) Kore::log(Info, "Horizontal: Out of bounds.");
					break;
				}

				CurrentIndexHorizontal = GetIndex(TestCell);
				if (IsSolid(CurrentIndexHorizontal))
				{
					// We have hit a cell from the left or the right
					// Horizontally, we have moved the initial distance to the first cell border plus i additional cells
					HorizontalDistance = FirstDeltaX + CellSize * i * SignHorizontal;
					HorizontalIndex = CurrentIndexHorizontal;
					HitHorizontalCell = TestCell;
					if (Debug) Kore::log(Info, "Horizontal: Hit at cell %i|%i", HitHorizontalCell.x(), HitHorizontalCell.y());
					break;
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Second, advance vertically and check for the first intersection with a cell border on the x-axis
		//////////////////////////////////////////////////////////////////////////
		// Compute the delta to the first intersection
		// On the y-axis, we need to multiply with -1 in order to account for the coordinate system being the other way around than the unit circle.
		FirstDeltaY = SignVertical > 0.0f ? CellSize - PositionInCurrentCell.y() : -PositionInCurrentCell.y();
		FirstDeltaX = -FirstDeltaY / TanAngle;
		AssertSign(FirstDeltaX, SignHorizontal);
		AssertSign(FirstDeltaY, SignVertical);

		// This is the first intersection vertically 
		if (Debug) Kore::log(Info, "Testing vertically: %0.2f|%0.2f", CurrentPosition.x() + FirstDeltaX, CurrentPosition.y() + FirstDeltaY);
		TestCell = Kore::vec2i(
			(int)Kore::floor((Position.x() + FirstDeltaX) / CellSize),
			CurrentCell.y() + SignVertical
		);

		int CurrentIndexVertical = GetIndex(TestCell);
		if (IsSolid(CurrentIndexVertical))
		{
			VerticalIndex = CurrentIndexVertical;
			VerticalDistance = FirstDeltaY;
			HitVerticalCell = TestCell;
			if (Debug) Kore::log(Info, "Vertical: Hit result in initial test.");
		}
		else
		{
			// We need to iterate from this point
			// Inverted due to inverted y-axis
			float DeltaX = -CellSize / TanAngle * SignVertical;
			AssertSign(DeltaX, SignHorizontal);
			float CurrentX = Position.x() + FirstDeltaX;
			//@@TODO: Figure out what the upper limit of tests is actually - now, we are relying on the level to be bounded
			// At this point, we are at the first intersection at the border of our current cell
			// i counts the number of cells we have moved away from the first tested cell
			for (int i = 1;;i++)
			{
				// In y-direction, we are advancing one cell at a time
				TestCell[1] += SignVertical;
				// In x-direction, we are advancing the associated DeltaY
				CurrentX += DeltaX;
				TestCell[0] = (int)Kore::floor(CurrentX / CellSize);
				if (Debug) Kore::log(Info, "Testing vertical cell: %i|%i", TestCell.x(), TestCell.y());
				if (TestCell.y() < 0 || TestCell.y() >= LevelHeight)
				{
					if (Debug) Kore::log(Info, "Horizontal: Out of bounds.");
					break;
				}

				int CurrentIndexVertical = GetIndex(TestCell);
				if (IsSolid(CurrentIndexVertical))
				{
					// We have hit a cell from the top or the bottom 
					// Vertically, we have moved the initial distance to the first cell border plus i additional cells
					VerticalDistance = FirstDeltaY + CellSize * i * SignVertical;
					VerticalIndex = CurrentIndexVertical;
					HitVerticalCell = TestCell;
					if (Debug) Kore::log(Info, "Vertical: Hit at cell %i|%i", HitVerticalCell.x(), HitVerticalCell.y());
					break;
				}
			}
		} 

		//////////////////////////////////////////////////////////////////////////
		// Now, check which distance is shorter
		//////////////////////////////////////////////////////////////////////////
		float Distance = CellSize * Kore::max(LevelWidth, LevelHeight) + 100000.0f;
		if (IsSolid(HorizontalIndex))
		{
			// For the vertical distance, we need to invert again
			float Y = -TanAngle * HorizontalDistance;
			AssertSign(HorizontalDistance, SignHorizontal);
			AssertSign(Y, SignVertical);
			Distance = Kore::cos(CurrentAngle) * HorizontalDistance - Kore::sin(CurrentAngle) * Y;
			Result = HorizontalIndex;
			HitCell = HitHorizontalCell;
			float yHitPosition = fmod(CurrentPosition.y() + Y, CellSize) / CellSize;
			TexCoordX = (int)(yHitPosition * TextureSize);
		} 
		if (IsSolid(VerticalIndex))
		{
			// For the vertical distance, we need to invert again
			float X = -VerticalDistance / TanAngle;
			AssertSign(X, SignHorizontal);
			AssertSign(VerticalDistance, SignVertical);
			float TempDistance = Kore::cos(CurrentAngle) * X - Kore::sin(CurrentAngle) * VerticalDistance;
			if (TempDistance < Distance)
			{
				Distance = TempDistance;
				Result = VerticalIndex;
				HitCell = HitVerticalCell;
				float xHitPosition = fmod(CurrentPosition.x() + X, CellSize) / CellSize;
				TexCoordX = (int)(xHitPosition * TextureSize);
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


	void DrawVerticalLine(Kore::Texture* InTexture, int X, int texX, int LineHeight)
	{
		for (int y = 0; y < LineHeight; y++)
		{
			float fY = (float)y / (float)LineHeight;
			int texY = (int)(fY * TextureSize);
			int col  = readPixel(InTexture, texX, texY);
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
		int CurrentIndex;
		Kore::vec2i HitCell;
		int TexCoordX = 0;
		for (int X = 0; X < width; X++)
		{
			float Distance = CastRay(CurrentPosition, CurrentRayAngle, CurrentIndex, HitCell, TexCoordX);
			float LineHeight = DistanceFactor / Distance;
			if (IsSolid(CurrentIndex))
			{
				Kore::Texture* CurrentTexture = Textures[CurrentIndex];
				DrawVerticalLine(CurrentTexture, X, TexCoordX, (int)LineHeight);
			}
			CurrentRayAngle += DeltaAngle;
		}

		// For debugging, calculate the current forward Angle
		// CurrentAngle = -0.2f;
		float TestDistance = CastRay(CurrentPosition, CurrentAngle, CurrentIndex, HitCell, TexCoordX, true);
		Kore::vec2i Cell = GetCell(CurrentPosition);
		bool IsInsideBlock = IsSolid(GetColor(Cell));
		assert(!IsInsideBlock);
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

	// SetupMap();
	Kore::System::setCallback(update);


	
	startTime = System::time();
	
	image = loadTexture("irobert-fb.png");
	Colors = new Kore::vec3[NumTextures + 1];
	Colors[1] = Kore::vec3(1.0f, 0.0f, 0.0f);
	Textures = new Kore::Texture*[NumTextures + 1];
	Textures[0] = nullptr;
	Textures[1] = loadTexture("finalredbrick1.png");
	Kore::Mixer::init();
	Kore::Audio::init();
	Kore::Mixer::play(new SoundStream("back.ogg", true));

	Kore::System::start();

	destroyTexture(image);
	
	return 0;
}
