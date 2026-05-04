// TITLE//
#include "framework.h"

// csúcspont árnyaló
const char *vertSource = R"(
@VERT_SHADER@)";

// pixel árnyaló
const char *fragSource = R"(
@FRAG_SHADER@)";

class GreenTriangleApp : public glApp
{
	GPUProgram *gpuProgram = nullptr; // csúcspont és pixel árnyalók

public:
	// glApp::glApp(unsigned int _majorNumber, unsigned int _minorNumber, unsigned int _windowWidth, unsigned int _windowHeight, const char * _windowCaption) {
	GreenTriangleApp() : glApp(3,3,1920-300,1080-300,"Green triangle") {}

	// Inicializáció,
	void onInitialization()
	{
		gpuProgram = new GPUProgram(vertSource, fragSource);
	}

	// Ablak rjrarajzols (drawing)
	void onDisplay()
	{
		glClearColor(0, 0, 0, 0);	  // backrgound color
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
		ImGui::End();
	}
};

GreenTriangleApp app;
