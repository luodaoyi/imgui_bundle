#ifdef IMGUI_BUNDLE_WITH_IMOGUIZMO
#include "immapp/runner.h"
#include "imoguizmo/imoguizmo.hpp"

static float viewMatrix[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
};

// Example project matrix
// A 90-degree field of view (FOV)
// An aspect ratio of 1.0 (square viewport)
// A near plane at 0.1 and a far plane at 1000.0.
static float projMat[16] = {
    2.41421f, 0.0f,      0.0f,    0.0f,
    0.0f,     2.41421f,  0.0f,    0.0f,
    0.0f,     0.0f,     -1.0002f, -1.0f,
    0.0f,     0.0f,     -0.20002f, 0.0f
};


void Gui()
{
    ImGui::Text("Hello");
    // it is recommended to use a separate projection matrix since the values that work best
    // can be very different from what works well with normal renderings
    // e.g., with glm -> glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

    // optional: configure color, axis length and more
    // ImOGuizmo::config.axisLengthScale = 1.0f;

    // specify position and size of gizmo (and its window when using ImOGuizmo::BeginFrame())
    ImOGuizmo::SetRect(60.f /* x */, 60.f /* y */, 120.0f /* square size */);
    ImOGuizmo::BeginFrame(); // to use you own window remove this call
    // and wrap everything in between ImGui::Begin() and ImGui::End() instead

    // optional: set distance to pivot (-> activates interaction)
    float pivotDistance = 0.1f;
    if(ImOGuizmo::DrawGizmo(viewMatrix, projMat, pivotDistance /* optional: default = 0.0f */))
    {
        // in case of user interaction viewMatrix gets updated
    }
}

int main(int, char **)
{
    ImmApp::Run(Gui);
}


#else
int main(int, char **) {}
#endif