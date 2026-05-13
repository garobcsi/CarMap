// TITLE//
#include "framework.h"
#include <random>
#include <mutex>
#include <algorithm>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// csúcspont árnyaló
const char *vertSource = R"(
@VERT_SHADER@)";

// pixel árnyaló
const char *fragSource = R"(
@FRAG_SHADER@)";

static int g_fbWidth = 800, g_fbHeight = 600;

class Noise
{
	unsigned int seed = 0;

	// Quintic Interpolation curve (improved perlin noise), Fade Function (aka smootherstep, "smoothstep"), éles átmenet elkerülése érdekében
	float smootherstep(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

	// Linear Interpolation, kettö pont között t paraméterrel interpolál
	float lerp(float a, float b, float t) { return a + t * (b - a); }

	// hash függvény 2D koordinátákhoz, véletlenszerű értékek generálásához
	// Bit Mixer algoritmus
	unsigned int hash2(int x, int y)
	{
		unsigned int h = (unsigned int)(x * 73856093 ^ y * 19349663) + seed * 2654435761u; // Primes for hashing

		// Avalanche effect, good distribution
		h ^= h >> 16;
		h *= 0x45d9f3bu;
		h ^= h >> 16;

		return h;
	}

	// Gradient Function, hash alapján véletlenszerű irányokat ad vissza, (dot product)
	float grad2(unsigned int h, float x, float y)
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

public:
	Noise(unsigned int seed = 0) : seed(seed) {}

	unsigned int &getSeed() { return seed; }

	// Perlin noise function in 2D, egy térképet generál a koordináták alapján, sima átmenetekkel
	float perlin2(float x, float y)
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
	float fbm(float x, float y, float freq = 1.0f, float amp = 0.5f)
	{
		float sum = 0.0f;
		for (int i = 0; i < 5; i++)
		{
			sum += perlin2(x * freq, y * freq) * amp;
			freq *= 2.0f;
			amp *= 0.5f;
		}
		return sum;
	}
};

class Camera
{
public:
	vec3 position = vec3(0.0f, 6.0f, 12.0f);
	// left right rotation
	float yaw = -M_PI / 2.0f;
	// up down rotation
	float pitch = -0.2f;
	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 200.0f;

	float moveSpeed = 5.0f;
	float mouseSensitivity = 0.0035f;

	bool mouseDown = false;
	int lastMouseX = 0;
	int lastMouseY = 0;

	vec3 forward() const
	{
		float cp = cosf(pitch);
		return vec3(cosf(yaw) * cp, sinf(pitch), sinf(yaw) * cp);
	}
	vec3 up() const { return vec3(0, 1, 0); }
	vec3 right() const { return normalize(cross(forward(), up())); }

	void rotateDelta(int dx, int dy)
	{
		yaw = fmodf(yaw + dx * mouseSensitivity, 2.0f * (float)M_PI);
		pitch = std::clamp(pitch - dy * mouseSensitivity, -1.4f, 1.4f);
	}

public:
	mat4 projection() const
	{
		return perspective(fov * (float)M_PI / 180.0f, (float)g_fbWidth / (float)g_fbHeight, nearPlane, farPlane);
	}
	mat4 view() const
	{
		return lookAt(position, position + forward(), up());
	}
	mat4 MVP() const { return projection() * view(); }

	void handleKeyboard(float dt)
	{
		float speed = moveSpeed * dt;
		if (pollKey(GLFW_KEY_W))
			position += forward() * speed;
		if (pollKey(GLFW_KEY_S))
			position -= forward() * speed;
		if (pollKey(GLFW_KEY_A))
			position -= right() * speed;
		if (pollKey(GLFW_KEY_D))
			position += right() * speed;
		if (pollKey(GLFW_KEY_E))
			position += up() * speed;
		if (pollKey(GLFW_KEY_Q))
			position -= up() * speed;
		if (pollKey(GLFW_KEY_SPACE))
			position.y += speed;
		if (pollKey(GLFW_KEY_LEFT_CONTROL))
			position.y -= speed;
	}

	void handleMousePress(MouseButton but, int x, int y)
	{
		if (but == MOUSE_LEFT)
		{
			mouseDown = true;
			lastMouseX = x;
			lastMouseY = y;
		}
	}

	void handleMouseRelease(MouseButton but)
	{
		if (but == MOUSE_LEFT)
			mouseDown = false;
	}

	void handleMouseMove(int x, int y)
	{
		if (!mouseDown)
			return;
		rotateDelta(x - lastMouseX, y - lastMouseY);
		lastMouseX = x;
		lastMouseY = y;
	}

	const vec3 &getPosition() const { return position; }
	const float &getYaw() const { return yaw; }
};

class Map : Geometry<vec3>
{
	Noise *noise = nullptr;

public:
	int gridSize = 128;
	float cellSize = 0.18f;

	float noiseScale = 0.08f;
	float heightScale = 2.2f;

	vec2 *lastVec = nullptr;

	Texture *noiseTex = nullptr;

	void buildTerrain(const vec2 &center)
	{
		std::vector<vec3> heights(gridSize * gridSize);
		float half = (gridSize - 1) * 0.5f;
		for (int z = 0; z < gridSize; z++)
		{
			for (int x = 0; x < gridSize; x++)
			{
				float wx = center.x + (x - half) * cellSize;
				float wz = center.y + (z - half) * cellSize;
				float h = (noise->fbm(wx * noiseScale, wz * noiseScale) * heightScale);

				heights[z * gridSize + x] = vec3(h, h, h);
			}
		}

		noiseTex->updateTexture(gridSize, gridSize, heights);

		auto &vtx = this->Vtx();
		vtx.clear();
		vtx.reserve((gridSize - 1) * (gridSize - 1) * 6);
		for (int z = 0; z < gridSize - 1; z++)
		{
			for (int x = 0; x < gridSize - 1; x++)
			{
				int i00 = z * gridSize + x;
				int i10 = z * gridSize + (x + 1);
				int i01 = (z + 1) * gridSize + x;
				int i11 = (z + 1) * gridSize + (x + 1);

				float wx0 = center.x + (x - half) * cellSize;
				float wx1 = center.x + (x + 1 - half) * cellSize;
				float wz0 = center.y + (z - half) * cellSize;
				float wz1 = center.y + (z + 1 - half) * cellSize;

				vec3 v00(wx0, heights[i00].x, wz0);
				vec3 v10(wx1, heights[i10].x, wz0);
				vec3 v01(wx0, heights[i01].x, wz1);
				vec3 v11(wx1, heights[i11].x, wz1);

				vtx.push_back(v00);
				vtx.push_back(v10);
				vtx.push_back(v11);

				vtx.push_back(v00);
				vtx.push_back(v11);
				vtx.push_back(v01);
			}
		}
		this->updateGPU();
	}

public:
	Map(Noise *noise) : noise(noise), noiseTex(new Texture(gridSize, gridSize)) {}

	void Draw(GPUProgram *prog, vec2 pos, vec3 color)
	{
		if (!lastVec || *lastVec != pos)
		{
			buildTerrain(pos);
			lastVec = new vec2(pos);
		}
		Geometry::Draw(prog, GL_TRIANGLES, color);
	}

	int getSize() const { return gridSize; }
	Texture *getNoiseTexture() const { return noiseTex; }
};

class CarMap : public glApp
{
	GPUProgram *gpuProgram = nullptr; // csúcspont és pixel árnyalók

	Noise *noise = nullptr;
	Camera *camera = nullptr;
	Map *map = nullptr;

public:
	// glApp::glApp(unsigned int _majorNumber, unsigned int _minorNumber, unsigned int _windowWidth, unsigned int _windowHeight, const char * _windowCaption)
	CarMap() : glApp(3, 3, 1920 - 300, 1080 - 300, "Car Map") {}

	// Inicializáció
	void onInitialization()
	{
		noise = new Noise(std::random_device{}());
		camera = new Camera();
		map = new Map(noise);

		glClearColor(0.55f, 0.7f, 0.9f, 1.0f); // backrgound color
		gpuProgram = new GPUProgram(vertSource, fragSource);

		glEnable(GL_DEPTH_TEST);
	}

	// Ablak rjrarajzols (drawing)
	void onDisplay()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Fix rendering off screen
		ImGuiIO &io = ImGui::GetIO();
		g_fbWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
		g_fbHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
		glViewport(0, 0, g_fbWidth, g_fbHeight);

		gpuProgram->setUniform(camera->MVP(), "MVP");

		vec3 camPos = camera->getPosition();
		if (map)
			map->Draw(gpuProgram, vec2(camPos.x, camPos.z), vec3(1.0f, 1.0f, 1.0f));
	}

	void onTimeElapsed(float startTime, float endTime)
	{
		float dt = endTime - startTime;
		camera->handleKeyboard(dt);
	}

	void onMousePressed(MouseButton but, int pX, int pY)
	{
		camera->handleMousePress(but, pX, pY);
	}

	void onMouseReleased(MouseButton but, int, int)
	{
		camera->handleMouseRelease(but);
	}

	void onMouseMotion(int pX, int pY)
	{
		camera->handleMouseMove(pX, pY);
	}

	void onGui() override
	{
		{
			static float miniMapCenterScale = 2.3f;

			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);

			ImGui::Begin("Minimap", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
			Texture *noiseTex = map->getNoiseTexture();
			if (noiseTex)
			{
				ImVec2 p = ImGui::GetCursorScreenPos();

				float mapSize = (float)map->getSize();
				ImGui::Image((void *)(intptr_t)noiseTex->getId(), ImVec2(mapSize, mapSize));

				ImDrawList *draw = ImGui::GetWindowDrawList();
				ImVec2 centre(p.x + mapSize * 0.5f, p.y + mapSize * 0.5f);
				draw->AddCircleFilled(centre, 4.0f * miniMapCenterScale, IM_COL32(255, 50, 50, 255));

				float arrowHeight = 10.0f * miniMapCenterScale;
				float camYaw = camera->getYaw();

				float arrowWidth = arrowHeight * 0.2f;

				ImVec2 tip(
					centre.x + cosf(camYaw) * arrowHeight,
					centre.y + sinf(camYaw) * arrowHeight);

				draw->AddLine(centre, tip, IM_COL32(255, 220, 0, 255), arrowWidth);
			}
			ImGui::End();
		}

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
			ImGui::End();
		}
	}
};

CarMap app;
