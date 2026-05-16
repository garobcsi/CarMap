//TITLE//
#include "framework.h"

// csúcspont árnyaló
const char * vertSource = R"(
@VERT_SHADER@)";

// pixel árnyaló
const char * fragSource = R"(
@FRAG_SHADER@)";

const int winWidth = 600, winHeight = 600;

class GreenTriangleApp : public glApp {
	Geometry<vec2>* triangle = nullptr;  // geometria
	Geometry<vec2>* points = nullptr;
	GPUProgram* gpuProgram = nullptr;	 // csúcspont és pixel árnyalók
public:
	GreenTriangleApp() : glApp("Green triangle") { }

	// Inicializáció, 
	void onInitialization() {
		triangle = new Geometry<vec2>;
		triangle->Vtx() = { vec2(-0.8f, -0.8f), vec2(-0.6f, 1.0f), vec2(0.8f, -0.2f) };
		triangle->updateGPU();
		points = new Geometry<vec2>;
		gpuProgram = new GPUProgram(vertSource, fragSource);

		glPointSize(10.0f);
	}

	// Ablak rjrarajzols (drawing)
	void onDisplay() {
		glClearColor(0, 0, 0, 0);     // backrgound color
		glClear(GL_COLOR_BUFFER_BIT); // clear buffer
		
		// Fix rendering off screen
		ImGuiIO& io = ImGui::GetIO();
		int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
		int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
		glViewport(0, 0, fb_width, fb_height);
		
		triangle->Draw(gpuProgram, GL_TRIANGLES, vec3(0.0f, 1.0f, 0.0f));
		points->Draw(gpuProgram, GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
	}

	virtual void onMousePressed(MouseButton but, int pX, int pY) {
		if (but == MOUSE_LEFT) {
			float x = 2.0f * pX / winWidth - 1.0f;
			float y = 1.0f - 2.0f * pY / winHeight;
			points->Vtx().push_back(vec2(x, y));
			points->updateGPU();
		}
		refreshScreen();
	}

	void onGui() override {
		ImGui::Begin("ImGui Test Window");
		ImGui::Text("Hello from ImGui in Grafika!");
		ImGui::End();
	}
};

GreenTriangleApp app;
