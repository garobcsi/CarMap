// TITLE//
#include "framework.h"
#include <random>
#include <algorithm>
#include <optional>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// csúcspont árnyaló
const char *vertSource = R"(
@VERT_SHADER@)";

// pixel árnyaló
const char *fragSource = R"(
@FRAG_SHADER@)";

static int g_fbWidth = 800, g_fbHeight = 600;
static const float kPi = (float)M_PI;

class Noise
{

public:
	unsigned int seed = 0;

protected:
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
	float moveSpeed = 5.0f;
	float mouseSensitivity = 0.0035f;
	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 200.0f;

protected:
	vec3 position = vec3(0.0f, 6.0f, 12.0f);
	// left right rotation
	float yaw = -kPi / 2.0f;
	// up down rotation
	float pitch = -0.2f;

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
		yaw = fmodf(yaw + dx * mouseSensitivity, 2.0f * kPi);
		pitch = std::clamp(pitch - dy * mouseSensitivity, -1.4f, 1.4f);
	}

public:
	mat4 projection() const
	{
		return perspective(fov * kPi / 180.0f, (float)g_fbWidth / (float)g_fbHeight, nearPlane, farPlane);
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

	void setPosition(const vec3 &pos) { position = pos; }
	const vec3 &getPosition() const { return position; }
	const vec2 getPosition2D() const { return vec2(position.x, position.z); }
	const float &getYaw() const { return yaw; }
};

class Map : Geometry<vec3>
{

public:
	int gridSize = 128;
	float cellSize = 0.18f;

	float noiseScale = 0.08f;
	float heightScale = 2.2f;

protected:
	Noise *noise = nullptr;
	std::optional<vec2> lastCenter = std::nullopt;
	Texture *noiseTex = nullptr;

	vec2 snapToGrid(const vec2 &pos) const
	{
		return vec2(
			floorf(pos.x / cellSize + 0.5f) * cellSize,
			floorf(pos.y / cellSize + 0.5f) * cellSize);
	}

	void buildTerrain(const vec2 &center)
	{
		vec2 snapped = snapToGrid(center);
		float half = (gridSize - 1) * 0.5f;

		std::vector<vec3> heights(gridSize * gridSize);
		for (int z = 0; z < gridSize; z++)
			for (int x = 0; x < gridSize; x++)
			{
				float wx = snapped.x + (x - half) * cellSize;
				float wz = snapped.y + (z - half) * cellSize;
				float h = heightAt(wx, wz);
				heights[z * gridSize + x] = vec3(h, h, h);
			}
		noiseTex->updateTexture(gridSize, gridSize, heights);

		auto &vtx = this->Vtx();
		vtx.clear();
		vtx.reserve((gridSize - 1) * (gridSize - 1) * 6);
		for (int z = 0; z < gridSize - 1; z++)
			for (int x = 0; x < gridSize - 1; x++)
			{
				int i00 = z * gridSize + x;
				int i10 = z * gridSize + (x + 1);
				int i01 = (z + 1) * gridSize + x;
				int i11 = (z + 1) * gridSize + (x + 1);

				float wx0 = snapped.x + (x - half) * cellSize;
				float wx1 = snapped.x + (x + 1 - half) * cellSize;
				float wz0 = snapped.y + (z - half) * cellSize;
				float wz1 = snapped.y + (z + 1 - half) * cellSize;

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
		this->updateGPU();
	}

public:
	Map(Noise *noise) : noise(noise), noiseTex(new Texture(gridSize, gridSize)) {}

	float heightAt(float x, float z) const
	{
		return noise->fbm(x * noiseScale, z * noiseScale) * heightScale;
	}

	void Draw(GPUProgram *prog, vec2 pos, vec3 color)
	{
		vec2 snapped = snapToGrid(pos);

		if (!lastCenter || *lastCenter != snapped)
		{
			buildTerrain(snapped);
			lastCenter = snapped;
		}
		Geometry::Draw(prog, GL_TRIANGLES, color);
	}

	int getSize() const { return gridSize; }
	Texture *getNoiseTexture() const { return noiseTex; }
	vec2 getCenter() const { return lastCenter.value_or(vec2(0, 0)); }
};

class CarBody
{

protected:
	Geometry<vec3> *boxGeo = nullptr;
	Geometry<vec3> *cylGeo = nullptr;

	void buildUnitBox()
	{
		auto &vtx = boxGeo->Vtx();
		vtx.clear();
		const float h = 0.5f;
		vec3 p000(-h, -h, -h), p001(-h, -h, h), p010(-h, h, -h), p011(-h, h, h);
		vec3 p100(h, -h, -h), p101(h, -h, h), p110(h, h, -h), p111(h, h, h);
		// +Z, -Z, -X, +X, +Y, -Y faces (two triangles each)
		vtx.insert(vtx.end(), {p001, p101, p111, p001, p111, p011});
		vtx.insert(vtx.end(), {p100, p000, p010, p100, p010, p110});
		vtx.insert(vtx.end(), {p000, p001, p011, p000, p011, p010});
		vtx.insert(vtx.end(), {p101, p100, p110, p101, p110, p111});
		vtx.insert(vtx.end(), {p010, p011, p111, p010, p111, p110});
		vtx.insert(vtx.end(), {p000, p100, p101, p000, p101, p001});
		boxGeo->updateGPU();
	}

	void buildCylinder(int segs)
	{
		auto &vtx = cylGeo->Vtx();
		vtx.clear();
		vtx.reserve(segs * 12);
		const float r = 0.5f;
		for (int i = 0; i < segs; ++i)
		{
			float a0 = 2.0f * kPi * i / segs;
			float a1 = 2.0f * kPi * (i + 1) / segs;
			float x0 = cosf(a0) * r, y0 = sinf(a0) * r;
			float x1 = cosf(a1) * r, y1 = sinf(a1) * r;
			// Side quad
			vtx.push_back(vec3(x0, y0, -0.5f));
			vtx.push_back(vec3(x1, y1, -0.5f));
			vtx.push_back(vec3(x1, y1, 0.5f));
			vtx.push_back(vec3(x0, y0, -0.5f));
			vtx.push_back(vec3(x1, y1, 0.5f));
			vtx.push_back(vec3(x0, y0, 0.5f));
			// +Z cap
			vtx.push_back(vec3(0, 0, 0.5f));
			vtx.push_back(vec3(x0, y0, 0.5f));
			vtx.push_back(vec3(x1, y1, 0.5f));
			// -Z cap
			vtx.push_back(vec3(0, 0, -0.5f));
			vtx.push_back(vec3(x1, y1, -0.5f));
			vtx.push_back(vec3(x0, y0, -0.5f));
		}
		cylGeo->updateGPU();
	}

	void drawBox(GPUProgram *prog, const mat4 &mvp, vec3 col) const
	{
		prog->setUniform(mvp, "MVP");
		boxGeo->Draw(prog, GL_TRIANGLES, col);
	}

	void drawCyl(GPUProgram *prog, const mat4 &mvp, vec3 col) const
	{
		prog->setUniform(mvp, "MVP");
		cylGeo->Draw(prog, GL_TRIANGLES, col);
	}

	vec3 up() const { return vec3(0, 1, 0); }

public:
	float carLength = 1.8f, carWidth = 0.9f, carHeight = 0.45f;
	float cabLength = 0.9f, cabWidth = 0.65f, cabHeight = 0.35f;
	float wheelRadius = 0.28f, wheelWidth = 0.22f;
	float wheelBase = 1.25f, trackWidth = 0.95f;
	float carClearance = 0.08f;

	CarBody()
	{
		boxGeo = new Geometry<vec3>();
		cylGeo = new Geometry<vec3>();
		buildUnitBox();
		buildCylinder(24);
	}

	void Draw(GPUProgram *prog, const mat4 &projView, const mat4 &CT,
			  float wheelSteer, const float wheelSpin[4]) const
	{
		const vec3 bodyRed(0.88f, 0.08f, 0.08f);
		const vec3 darkRed(0.68f, 0.04f, 0.04f);
		const vec3 black(0.07f, 0.07f, 0.07f);
		const vec3 darkGrey(0.20f, 0.20f, 0.22f);
		const vec3 silver(0.78f, 0.78f, 0.82f);
		const vec3 brightRed(0.97f, 0.04f, 0.04f);
		const vec3 glassBlue(0.32f, 0.52f, 0.70f);

		float cL = carLength, cH = carHeight, cW = carWidth;

		{
			auto box = [&](vec3 off, vec3 sz, vec3 col)
			{
				drawBox(prog, projView * CT * translate(off) * scale(sz), col);
			};

			// BODY
			box(vec3(0, -cH * 0.04f, 0), vec3(cL, cH * 0.88f, cW), bodyRed); // main sill

			// Hood panel + centre rib
			box(vec3(cL * 0.24f, cH * 0.455f, 0), vec3(cL * 0.43f, cH * 0.11f, cW * 0.86f), bodyRed);
			box(vec3(cL * 0.24f, cH * 0.515f, 0), vec3(cL * 0.38f, cH * 0.035f, cW * 0.16f), darkRed);

			// Trunk lid + spoiler lip
			box(vec3(-cL * 0.21f, cH * 0.445f, 0), vec3(cL * 0.35f, cH * 0.09f, cW * 0.86f), bodyRed);
			box(vec3(-cL * 0.39f, cH * 0.485f, 0), vec3(cL * 0.06f, cH * 0.045f, cW * 0.72f), darkRed);

			// Cabin glass + roof panel
			vec3 cabOff(-cL * 0.10f, cH * 0.56f, 0.0f);
			box(cabOff, vec3(cabLength, cabHeight, cabWidth), glassBlue);
			box(vec3(cabOff.x, cabOff.y + cabHeight * 0.5f, 0.0f),
				vec3(cabLength * 0.96f, 0.05f, cabWidth * 0.96f), bodyRed);

			// Headlights + grille
			box(vec3(cL * 0.501f, cH * 0.20f, -(cW * 0.35f)), vec3(cL * 0.02f, cH * 0.15f, cW * 0.2f), vec3(1.0f, 1.0f, 0.9f));
			box(vec3(cL * 0.501f, cH * 0.20f, (cW * 0.35f)), vec3(cL * 0.02f, cH * 0.15f, cW * 0.2f), vec3(1.0f, 1.0f, 0.9f));
			box(vec3(cL * 0.505f, cH * 0.05f, 0.0f), vec3(cL * 0.02f, cH * 0.18f, cW * 0.45f), black);

			// Taillights
			box(vec3(-cL * 0.501f, cH * 0.18f, -(cW * 0.35f)), vec3(cL * 0.02f, cH * 0.12f, cW * 0.2f), brightRed);
			box(vec3(-cL * 0.501f, cH * 0.18f, (cW * 0.35f)), vec3(cL * 0.02f, cH * 0.12f, cW * 0.2f), brightRed);

			// Bumpers + undercarriage
			box(vec3(cL * 0.51f, -cH * 0.20f, 0), vec3(cL * 0.07f, cH * 0.28f, cW * 1.02f), darkGrey);
			box(vec3(-cL * 0.51f, -cH * 0.20f, 0), vec3(cL * 0.07f, cH * 0.28f, cW * 1.02f), darkGrey);
			box(vec3(0, -cH * 0.47f, 0), vec3(cL * 0.88f, cH * 0.06f, cW * 0.78f), black);
		}

		{
			// WHEELS
			float wD = wheelRadius * 2.0f;
			float rimD = wD * 0.65f;
			float axF = wheelBase * 0.5f;
			float axR = -wheelBase * 0.5f;
			float trK = trackWidth * 0.5f;
			float wDrop = cH * 0.5f + carClearance; // wheel centre offset downward in car space

			const vec3 wOff[4] = {
				vec3(axF, -wDrop, -trK), vec3(axF, -wDrop, trK),
				vec3(axR, -wDrop, -trK), vec3(axR, -wDrop, trK)};

			for (int i = 0; i < 4; i++)
			{
				bool isFront = (i < 2);
				float steerA = isFront ? wheelSteer : 0.0f;

				mat4 axle = CT * translate(wOff[i]) * rotate(-steerA, up());
				mat4 spin = axle * rotate(-wheelSpin[i], vec3(0, 0, 1));

				auto wCyl = [&](const mat4 &localM, vec3 col)
				{
					drawCyl(prog, projView * localM, col);
				};

				auto wBox = [&](const mat4 &localM, vec3 col)
				{
					drawBox(prog, projView * localM, col);
				};

				// Tyre (black rubber)
				wCyl(spin * scale(vec3(wD, wD, wheelWidth)), black);
				// Rim outer lip
				wCyl(spin * scale(vec3(rimD, rimD, wheelWidth * 1.02f)), silver);
				// Rim inner shadow (depth illusion)
				wCyl(spin * scale(vec3(rimD * 0.8f, rimD * 0.8f, wheelWidth * 1.03f)), darkGrey);
				// Central hub
				wCyl(spin * scale(vec3(rimD * 0.25f, rimD * 0.25f, wheelWidth * 1.04f)), silver);
				// Five spokes
				for (int s = 0; s < 5; s++)
				{
					float ang = s * (2.0f * kPi / 5.0f);
					mat4 spokeM = spin *
								  rotate(ang, vec3(0, 0, 1)) *
								  translate(vec3(rimD * 0.2f, 0.0f, 0.0f)) *
								  scale(vec3(rimD * 0.45f, wD * 0.05f, wheelWidth * 1.04f));
					wBox(spokeM, silver);
				}
			}
		}
	}
};

class Car
{
	CarBody *body = nullptr;
	Map *map = nullptr;

	// physics
	vec3 position;
	float yaw = 0.0f, speed = 0.0f;
	float pitch = 0.0f, roll = 0.0f;
	float wheelSteer = 0.0f;
	float wheelSpin[4] = {};

	vec3 forward() const { return vec3(cosf(yaw), 0.0f, sinf(yaw)); }
	vec3 up() const { return vec3(0, 1, 0); }
	vec3 right(const vec3 &fwd) const { return normalize(cross(fwd, up())); }

public:
	// tuning
	float maxSpeed = 10.0f, maxReverse = -4.5f;
	float accel = 8.5f, brakeForce = 11.0f, drag = 3.2f;
	float turnRate = 1.7f, maxSteerAngle = 0.6f;

	Car(Map *map) : map(map), body(new CarBody())
	{
		float h = map->heightAt(0.0f, 0.0f);
		position = vec3(0.0f,
						h + body->wheelRadius + body->carHeight * 0.5f + body->carClearance,
						0.0f);
	}

	void handleKeyboard(float dt)
	{
		// throttle, brake
		float throttle = 0.0f;
		if (pollKey(GLFW_KEY_W))
			throttle += 1.0f;
		if (pollKey(GLFW_KEY_S))
			throttle -= 1.0f;

		if (throttle > 0.0f)
			speed += accel * throttle * dt;
		else if (throttle < 0.0f)
			speed += brakeForce * throttle * dt;
		else
			speed -= speed * drag * dt;
		speed = std::clamp(speed, maxReverse, maxSpeed);

		// steering
		float steerInput = 0.0f;
		if (pollKey(GLFW_KEY_A))
			steerInput -= 1.0f;
		if (pollKey(GLFW_KEY_D))
			steerInput += 1.0f;

		float speedAbs = fabsf(speed);
		float steerScale = 1.0f - 0.35f * fminf(speedAbs / maxSpeed, 1.0f);
		wheelSteer = steerInput * maxSteerAngle * steerScale;

		float yawRate = 0.0f;
		if (speedAbs > 0.01f)
		{
			float dir = (speed >= 0.0f) ? 1.0f : -1.0f;
			yawRate = steerInput * turnRate * dir * steerScale;
		}
		yaw += yawRate * dt;

		// move
		vec3 fwd = forward();
		position += fwd * speed * dt;
		vec3 rgt = right(fwd);

		// four-wheel terrain contact
		float axF = body->wheelBase * 0.5f;
		float axR = -body->wheelBase * 0.5f;
		float trK = body->trackWidth * 0.5f;

		const vec3 offsets[4] = {
			fwd * axF - rgt * trK, fwd * axF + rgt * trK,
			fwd * axR - rgt * trK, fwd * axR + rgt * trK};
		float h[4];
		for (int i = 0; i < 4; i++)
		{
			vec3 wp = vec3(position.x, 0.0f, position.z) + offsets[i];
			h[i] = map->heightAt(wp.x, wp.z);
		}

		float frontAvg = (h[0] + h[1]) * 0.5f, rearAvg = (h[2] + h[3]) * 0.5f;
		float leftAvg = (h[0] + h[2]) * 0.5f, rightAvg = (h[1] + h[3]) * 0.5f;
		pitch = atan2f(frontAvg - rearAvg, body->wheelBase);
		roll = atan2f(rightAvg - leftAvg, body->trackWidth);
		position.y = (frontAvg + rearAvg) * 0.5f + body->wheelRadius + body->carHeight * 0.5f + body->carClearance;

		// wheel spin
		for (int i = 0; i < 4; i++)
		{
			float sideSign = (i == 1 || i == 3) ? 1.0f : -1.0f;
			float ws = speed - yawRate * sideSign * trK;
			wheelSpin[i] += (ws / fmaxf(body->wheelRadius, 0.001f)) * dt;
		}
	}

	void Draw(GPUProgram *prog, const mat4 &projView) const
	{
		mat4 bodyRot = rotate(-yaw, up()) * rotate(pitch, vec3(0, 0, 1)) * rotate(-roll, vec3(1, 0, 0));
		mat4 CT = translate(position) * bodyRot;
		body->Draw(prog, projView, CT, wheelSteer, wheelSpin);
	}

	vec3 getPosition() const { return position; }
	float getYaw() const { return yaw; }
	float getCarHeight() const { return body->carHeight; }
	vec2 getPosition2D() const { return vec2(position.x, position.z); }
};

class CarCamera : Camera
{

public:
	float followDistance = 4.8f;
	float followHeight = 2.4f;
	float lookAheadDist = 1.1f;
	float sharpness = 5.5f;

protected:
	vec3 target = vec3(0.0f);
	float manualYaw = 0.0f;
	float manualPitch = 0.0f;
	float smoothedCarYaw = 0.0f; // lag behind actual car yaw

	static float wrapAngle(float a)
	{
		while (a > kPi)
			a -= 2.0f * kPi;
		while (a < -kPi)
			a += 2.0f * kPi;
		return a;
	}

	static vec3 lerpVec(const vec3 &a, const vec3 &b, float t)
	{
		return a + (b - a) * t;
	}

public:
	CarCamera()
	{
		position = vec3(0.0f, followHeight, followDistance);
	}

	mat4 view() const { return lookAt(position, target, Camera::up()); }
	mat4 MVP() const { return Camera::projection() * view(); }

	void follow(const Car &car, float dt)
	{
		float t = 1.0f - expf(-sharpness * dt);

		// smoothly rotate
		smoothedCarYaw += wrapAngle(car.getYaw() - smoothedCarYaw) * t;

		vec3 carFwd(cosf(car.getYaw()), 0.0f, sinf(car.getYaw()));
		vec3 carCenter = car.getPosition() + vec3(0.0f, car.getCarHeight() * 0.6f, 0.0f);
		vec3 desiredTarget = carCenter + carFwd * lookAheadDist;

		// desired camera position
		float totalYaw = smoothedCarYaw + manualYaw;
		float hRadius = followDistance * cosf(manualPitch);
		float vHeight = followHeight + followDistance * sinf(manualPitch);
		vec3 orbitDir(cosf(totalYaw), 0.0f, sinf(totalYaw));
		vec3 desiredPos = desiredTarget - orbitDir * hRadius + vec3(0.0f, vHeight, 0.0f);

		position = lerpVec(position, desiredPos, t);
		target = lerpVec(target, desiredTarget, t);
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
		manualYaw -= (x - lastMouseX) * 0.01f;
		manualPitch = std::clamp(manualPitch + (y - lastMouseY) * 0.01f,
								 -kPi * 0.49f, kPi * 0.49f);
		lastMouseX = x;
		lastMouseY = y;
	}

	vec3 getPosition() const { return position; }
};

class CarMap : public glApp
{
	GPUProgram *gpuProgram = nullptr;
	Noise *noise = nullptr;
	Map *map = nullptr;

	Car *car = nullptr;
	CarCamera *carCam = nullptr;
	Camera *freeCam = nullptr;

	bool freeCamMode = false;

public:
	CarMap() : glApp(3, 3, 1920 - 300, 1080 - 300, "Car Map") {}

	void onInitialization() override
	{
		noise = new Noise(std::random_device{}());
		map = new Map(noise);
		car = new Car(map);
		carCam = new CarCamera();
		freeCam = new Camera();

		glClearColor(0.55f, 0.7f, 0.9f, 1.0f);
		gpuProgram = new GPUProgram(vertSource, fragSource);

		glEnable(GL_DEPTH_TEST);
	}

	void onDisplay() override
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGuiIO &io = ImGui::GetIO();
		g_fbWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
		g_fbHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
		glViewport(0, 0, g_fbWidth, g_fbHeight);

		mat4 carView = carCam->MVP();
		mat4 freeView = freeCam->MVP();

		if (!freeCamMode)
			gpuProgram->setUniform(carView, "MVP");
		else
			gpuProgram->setUniform(freeView, "MVP");

		map->Draw(gpuProgram, car->getPosition2D(), vec3(1.0f));
		gpuProgram->setUniform(true, "isCar");
		car->Draw(gpuProgram, freeCamMode ? freeView : carView);
		gpuProgram->setUniform(false, "isCar");
	}

	void onTimeElapsed(float startTime, float endTime) override
	{
		float dt = endTime - startTime;
		if (!freeCamMode)
		{
			car->handleKeyboard(dt);
			carCam->follow(*car, dt);
		}
		else
		{
			freeCam->handleKeyboard(dt);
		}
	}

	void onMousePressed(MouseButton but, int pX, int pY) override
	{
		if (!freeCamMode)
			carCam->handleMousePress(but, pX, pY);
		else
			freeCam->handleMousePress(but, pX, pY);
	}

	void onMouseReleased(MouseButton but, int, int) override
	{
		if (!freeCamMode)
			carCam->handleMouseRelease(but);
		else
			freeCam->handleMouseRelease(but);
	}

	void onMouseMotion(int pX, int pY) override
	{
		if (!freeCamMode)
			carCam->handleMouseMove(pX, pY);
		else
			freeCam->handleMouseMove(pX, pY);
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

				ImVec2 centre;
				{
					vec2 trackedPos = freeCamMode
										  ? freeCam->getPosition2D()
										  : car->getPosition2D();

					vec2 terrainCenter = map->getCenter();
					float worldSpan = (map->getSize() - 1) * map->cellSize; // total world units the texture covers
					float pixelsPerUnit = mapSize / worldSpan;

					centre = ImVec2(
						p.x + mapSize * 0.5f + (trackedPos.x - terrainCenter.x) * pixelsPerUnit,
						p.y + mapSize * 0.5f + (trackedPos.y - terrainCenter.y) * pixelsPerUnit);
				}

				{
					draw->AddCircleFilled(centre, 4.0f * miniMapCenterScale, IM_COL32(255, 50, 50, 255));
				}

				{
					float arrowLen = 10.0f * miniMapCenterScale;
					float yaw = freeCamMode ? freeCam->getYaw() : car->getYaw();

					ImVec2 tip(
						centre.x + cosf(yaw) * arrowLen,
						centre.y + sinf(yaw) * arrowLen);

					draw->AddLine(centre, tip, IM_COL32(255, 220, 0, 255), arrowLen * 0.2f);
				}
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
				if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (ImGui::Checkbox("Free Cam Mode", &freeCamMode))
					{
						freeCam->setPosition(carCam->getPosition());
					}
				}
			}
			ImGui::End();
		}
	}
};

CarMap app;