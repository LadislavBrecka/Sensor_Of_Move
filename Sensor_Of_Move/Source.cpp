#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "escapi.h"
#include<iostream>

int nFrameWidth = 320;
int nFrameHeight = 240;

constexpr uint32_t s_CameraNumber = 1;

struct frame
{
	float* pixels = nullptr;

	frame()
	{
		pixels = new float[nFrameWidth * nFrameHeight];
	}

	~frame()
	{
		delete[] pixels;
	}


	float get(int x, int y)
	{
		if (x >= 0 && x < nFrameWidth && y >= 0 && y < nFrameHeight)
		{
			return pixels[y * nFrameWidth + x];
		}
		else
			return 0.0f;
	}

	void set(int x, int y, float p)
	{
		if (x >= 0 && x < nFrameWidth && y >= 0 && y < nFrameHeight)
		{
			pixels[y * nFrameWidth + x] = p;
		}
	}


	void operator=(const frame& f)
	{
		memcpy(this->pixels, f.pixels, nFrameWidth * nFrameHeight * sizeof(float));
	}
};



class Sensor : public olc::PixelGameEngine
{
private:
	int nCameras = 0;
	SimpleCapParams capture;
	frame input, output, prev_input;

	union RGBint
	{
		int rgb;
		unsigned char c[4];
	};

	
public:
	Sensor()
	{
		sAppName = "Example";
	}

	void DrawFrame(frame& f, int x, int y)
	{
		for (int i = 0; i < nFrameWidth; i++)
			for (int j = 0; j < nFrameHeight; j++)
			{
				int c = (int)std::min(std::max(0.0f, f.pixels[j * nFrameWidth + i] * 255.0f), 255.0f);
				Draw(x + i, y + j, olc::Pixel(c, c, c));
			}
	}

	bool OnUserCreate() override
	{
		// Initialise webcam to screen dimensions
		nCameras = setupESCAPI();
		if (nCameras == 0)	return false;
		capture.mWidth = nFrameWidth;
		capture.mHeight = nFrameHeight;
		capture.mTargetBuf = new int[nFrameWidth * nFrameHeight];
		if (initCapture(s_CameraNumber, &capture) == 0)	return false;
		return true;
	}


	bool OnUserUpdate(float fElapsedTime) override
	{
		prev_input = input;

		doCapture(s_CameraNumber); while (isCaptureDone(s_CameraNumber) == 0) {}
		for (int y = 0; y < capture.mHeight; y++)
			for (int x = 0; x < capture.mWidth; x++)
			{
				RGBint col;
				int id = y * capture.mWidth + x;
				col.rgb = capture.mTargetBuf[id];
				input.pixels[y * nFrameWidth + x] = (float)col.c[1] / 255.0f;
			}

		/*output = input;*/
		int nAttempts = 0;

		for (int x = 0; x < capture.mWidth; x++)
			for (int y = 0; y < capture.mHeight; y++)
			{
				float fDiff = fabs(input.get(x, y) - prev_input.get(x, y));

				//LOW-PASS FILTERING
				output.set(x, y, output.get(x, y) + (input.get(x, y) - output.get(x, y)) * 0.05f);

				//MOTION FILTER
				/*output.set(x, y, fabs(input.get(x, y) - prev_input.get(x, y)));*/
				
				//TRESHOLDING INPUT IMAGE
				output.set(x, y, fDiff >= 0.05f ? fDiff : 0.0f);
				/*output.set(x, y, output.get(x, y) >= 0.5f ? 1.0f : 0.0f);*/
			}

		for (int x = 0; x < capture.mWidth; x++)
			for (int y = 0; y < capture.mHeight; y++)
			{
				if (output.get(x, y) != 0.0f)
					nAttempts++;
			}

		Clear(olc::DARK_BLUE);

		if (nAttempts > 500)
			DrawString(230, 270, "THERE IS A MOVE!", olc::RED, 2);

		DrawFrame(input, 10, 10);
		DrawFrame(output, 340, 10);

		if (GetKey(olc::Key::ESCAPE).bPressed) return false;
			return true;


		return true;
	}
};
int main()
{
	Sensor move;
	if (move.Construct(nFrameWidth * 2 + 30, nFrameHeight + 100, 2, 2))
		move.Start();
	return 0;
}