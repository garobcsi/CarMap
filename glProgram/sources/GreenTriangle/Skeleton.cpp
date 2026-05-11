// TITLE//
#include "framework.h"
#include <random>

// csúcspont árnyaló
const char *vertSource = R"(
@VERT_SHADER@)";

// pixel árnyaló
const char *fragSource = R"(
@FRAG_SHADER@)";

static unsigned int g_seed = 0;

// Quintic Interpolation curve (improved perlin noise), Fade Function (aka smootherstep, "smoothstep"), éles átmenet elkerülése érdekében
float smootherstep(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

// Linear Interpolation, kettö pont között t paraméterrel interpolál
float lerp(float a, float b, float t) { return a + t * (b - a); }

// hash függvény 2D koordinátákhoz, véletlenszerű értékek generálásához
// Bit Mixer algoritmus
static unsigned int hash2(int x, int y)
{
	unsigned int h = (unsigned int)(x * 73856093 ^ y * 19349663) + g_seed * 2654435761u; // Primes for hashing

	// Avalanche effect, good distribution
	h ^= h >> 16;
	h *= 0x45d9f3bu;
	h ^= h >> 16;

	return h;
}

// Gradient Function, hash alapján véletlenszerű irányokat ad vissza, (dot product)
static float grad2(unsigned int h, float x, float y)
{
	switch (h & 7u)
	{
	case 0:
		return x + y;
	case 1:
		return -x + y;
	case 2:
		return x - y;
	case 3:
		return -x - y;
	case 4:
		return x;
	case 5:
		return -x;
	case 6:
		return y;
	default:
		return -y;
	}
}

// Perlin noise function in 2D, egy térképet generál a koordináták alapján, sima átmenetekkel
static float perlin2(float x, float y)
{
	// get grid cell coordinates
	int x0 = (int)floorf(x);
	int y0 = (int)floorf(y);
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	// local coordinates within the cell
	float xf = x - (float)x0;
	float yf = y - (float)y0;

	// smoothing
	float u = smootherstep(xf);
	float v = smootherstep(yf);

	// corner influences
	float n00 = grad2(hash2(x0, y0), xf, yf);
	float n10 = grad2(hash2(x1, y0), xf - 1.0f, yf);
	float n01 = grad2(hash2(x0, y1), xf, yf - 1.0f);
	float n11 = grad2(hash2(x1, y1), xf - 1.0f, yf - 1.0f);

	// Bilinear Interpolation between the corners
	// mixing
	float nx0 = lerp(n00, n10, u);
	float nx1 = lerp(n01, n11, u);
	return lerp(nx0, nx1, v);
}

// Fractal Brownian Motion
// more detail by adding multiple octaves of noise together
static float fbm(float x, float y, float amp = 0.5f, float freq = 1.0f)
{
	float sum = 0.0f;
	for (int i = 0; i < 5; i++)
	{
		sum += amp * perlin2(x * freq, y * freq);
		freq *= 2.0f;
		amp *= 0.5f;
	}
	return sum;
}

class CarMap : public glApp
{
	GPUProgram *gpuProgram = nullptr; // csúcspont és pixel árnyalók

public:
	// glApp::glApp(unsigned int _majorNumber, unsigned int _minorNumber, unsigned int _windowWidth, unsigned int _windowHeight, const char * _windowCaption)
	CarMap() : glApp(3, 3, 1920 - 300, 1080 - 300, "Car Map") {}

	// Inicializáció
	void onInitialization()
	{
		g_seed = std::random_device{}(); // random seed for noise

		gpuProgram = new GPUProgram(vertSource, fragSource);
		glClearColor(0.55f, 0.7f, 0.9f, 1.0f); // backrgound color
	}

	// Ablak rjrarajzols (drawing)
	void onDisplay()
	{
		glClear(GL_COLOR_BUFFER_BIT); // clear buffer

		// Fix rendering off screen
		ImGuiIO &io = ImGui::GetIO();
		int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
		int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
		glViewport(0, 0, fb_width, fb_height);
	}

	// virtual void onMousePressed(MouseButton but, int pX, int pY) {

	// }

	void onGui() override
	{
		ImGui::Begin("Settings");

		{
			if (ImGui::CollapsingHeader("World Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// background color
				static float bgColor[3] = {0.55f, 0.7f, 0.9f};
				if (ImGui::ColorEdit3("Background Color", bgColor))
				{
					glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
				}
			}
		}

		ImGui::Separator();

		{
			if (ImGui::CollapsingHeader("Math visualization", ImGuiTreeNodeFlags_DefaultOpen))
			{
				static int mapWidth = 256;
				static int mapHeight = 256;
				static float scale = 30.0f;
				static bool updateTexture = true;

				if (ImGui::Button("Randomize Seed"))
				{
					g_seed = std::random_device{}();
					updateTexture = true;
				}

				ImGui::SameLine();

				if (ImGui::InputScalar("Seed", ImGuiDataType_U32, &g_seed))
					updateTexture = true;

				if (ImGui::SliderFloat("Noise Scale", &scale, 1.0f, 50.0f))
					updateTexture = true;

				// Perlin noise visualization
				{
					static Texture *noiseTex = nullptr;

					if (updateTexture)
					{
						if (!noiseTex)
							noiseTex = new Texture(mapWidth, mapHeight);

						std::vector<vec3> image(mapWidth * mapHeight);
						for (int y = 0; y < mapHeight; y++)
							for (int x = 0; x < mapWidth; x++)
							{
								float v = (perlin2(x / (float)mapWidth * scale, y / (float)mapHeight * scale) + 1.0f) * 0.5f;
								image[y * mapWidth + x] = vec3(v, v, v);
							}

						noiseTex->updateTexture(mapWidth, mapHeight, image);
					}

					if (noiseTex)
					{
						ImGui::Image((void *)(intptr_t)noiseTex->getId(), ImVec2((float)mapWidth, (float)mapHeight));
					}
				}

				// FBM visualization
				{
					static Texture *noiseTex = nullptr;

					static float amp = 0.5f, freq = 1.0f;
					if (ImGui::SliderFloat("FBM Amplitude", &amp, 0.1f, 1.0f))
						updateTexture = true;
					if (ImGui::SliderFloat("FBM Frequency", &freq, 0.1f, 10.0f))
						updateTexture = true;

					if (updateTexture)
					{
						if (!noiseTex)
							noiseTex = new Texture(mapWidth, mapHeight);

						std::vector<vec3> image(mapWidth * mapHeight);
						for (int y = 0; y < mapHeight; y++)
							for (int x = 0; x < mapWidth; x++)
							{
								float v = (fbm(x / (float)mapWidth * scale, y / (float)mapHeight * scale, amp, freq) + 1.0f) * 0.5f;
								image[y * mapWidth + x] = vec3(v, v, v);
							}

						noiseTex->updateTexture(mapWidth, mapHeight, image);
					}

					if (noiseTex)
					{
						ImGui::Image((void *)(intptr_t)noiseTex->getId(), ImVec2((float)mapWidth, (float)mapHeight));
					}
				}

				updateTexture = false;
			}
		}

		ImGui::End();
	}
};

CarMap app;
