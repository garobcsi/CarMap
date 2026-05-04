// TITLE//
#include "framework.h"

// csúcspont árnyaló
const char *vertSource = R"(
@VERT_SHADER@)";

// pixel árnyaló
const char *fragSource = R"(
@FRAG_SHADER@)";

class CarMap : public glApp
{
	GPUProgram *gpuProgram = nullptr; // csúcspont és pixel árnyalók

public:
	// glApp::glApp(unsigned int _majorNumber, unsigned int _minorNumber, unsigned int _windowWidth, unsigned int _windowHeight, const char * _windowCaption) {
	CarMap() : glApp(3, 3, 1920 - 300, 1080 - 300, "Car Map") {}

	// Inicializáció,
	void onInitialization()
	{
		gpuProgram = new GPUProgram(vertSource, fragSource);
	}

	// Ablak rjrarajzols (drawing)
	void onDisplay()
	{
		glClearColor(0.55f, 0.7f, 0.9f, 1.0f); // backrgound color
		glClear(GL_COLOR_BUFFER_BIT);		   // clear buffer

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
		ImGui::End();
	}
};

CarMap app;
