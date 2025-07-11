#ifdef IMGUI_BUNDLE_WITH_IMGUIZMO
// Demo ImGuizmo (only the 3D gizmo)
// See equivalent python program: bindings/imgui_bundle/demos/demos_imguizmo/demo_guizmo_pure.py

// https://github.com/CedricGuillemet/ImGuizmo
// v 1.89 WIP
//
// The MIT License(MIT)
//
// Copyright(c) 2021 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#define IMGUI_DEFINE_MATH_OPERATORS

#include "demo_utils/api_demos.h"
#include "immapp/immapp.h"
#include "ImGuizmoPure/ImGuizmoPure.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <math.h>


static bool useWindow = true;
static int gizmoCount = 1;
static float camDistance = 8.f;
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);


using ImGuizmo::Matrix16;
using ImGuizmo::Matrix3;
using ImGuizmo::Matrix6;


template<typename T>
std::vector<T> vec_n_first(std::vector<T> const &v, size_t n)
{
    auto first = v.cbegin();
    auto last = v.cbegin() + n;

    std::vector<T> vec(first, last);
    return vec;
}


static std::vector<Matrix16> gObjectMatrix = {
    Matrix16({
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f }),
    Matrix16({
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        2.f, 0.f, 0.f, 1.f }),
    Matrix16({
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        2.f, 0.f, 2.f, 1.f }),
    Matrix16({
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 2.f, 1.f })
};

static const ImGuizmo::Matrix16 identityMatrix(
    { 1.f, 0.f, 0.f, 0.f,
      0.f, 1.f, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
      0.f, 0.f, 0.f, 1.f });

void Frustum(float left, float right, float bottom, float top, float znear, float zfar, Matrix16& m16)
{
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    m16.values[0] = temp / temp2;
    m16.values[1] = 0.0;
    m16.values[2] = 0.0;
    m16.values[3] = 0.0;
    m16.values[4] = 0.0;
    m16.values[5] = temp / temp3;
    m16.values[6] = 0.0;
    m16.values[7] = 0.0;
    m16.values[8] = (right + left) / temp2;
    m16.values[9] = (top + bottom) / temp3;
    m16.values[10] = (-zfar - znear) / temp4;
    m16.values[11] = -1.0f;
    m16.values[12] = 0.0;
    m16.values[13] = 0.0;
    m16.values[14] = (-temp * zfar) / temp4;
    m16.values[15] = 0.0;
}

void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, Matrix16& m16)
{
    float ymax, xmax;
    ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
    xmax = ymax * aspectRatio;
    Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
}

void Cross(const Matrix3& a, const Matrix3& b, Matrix3& r)
{
    r.values[0] = a.values[1] * b.values[2] - a.values[2] * b.values[1];
    r.values[1] = a.values[2] * b.values[0] - a.values[0] * b.values[2];
    r.values[2] = a.values[0] * b.values[1] - a.values[1] * b.values[0];
}

float Dot(const Matrix3& a, const Matrix3& b)
{
    return a.values[0] * b.values[0] + a.values[1] * b.values[1] + a.values[2] * b.values[2];
}

void Normalize(const Matrix3& a, Matrix3& r)
{
    float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
    r.values[0] = a.values[0] * il;
    r.values[1] = a.values[1] * il;
    r.values[2] = a.values[2] * il;
}

void LookAt(const Matrix3& eye, const Matrix3& at, const Matrix3& up, Matrix16& m16)
{
    Matrix3 X, Y, Z, tmp;

    tmp.values[0] = eye.values[0] - at.values[0];
    tmp.values[1] = eye.values[1] - at.values[1];
    tmp.values[2] = eye.values[2] - at.values[2];
    Normalize(tmp, Z);
    Normalize(up, Y);

    Cross(Y, Z, tmp);
    Normalize(tmp, X);

    Cross(Z, X, tmp);
    Normalize(tmp, Y);

    m16.values[0] = X.values[0];
    m16.values[1] = Y.values[0];
    m16.values[2] = Z.values[0];
    m16.values[3] = 0.0f;
    m16.values[4] = X.values[1];
    m16.values[5] = Y.values[1];
    m16.values[6] = Z.values[1];
    m16.values[7] = 0.0f;
    m16.values[8] = X.values[2];
    m16.values[9] = Y.values[2];
    m16.values[10] = Z.values[2];
    m16.values[11] = 0.0f;
    m16.values[12] = -Dot(X, eye);
    m16.values[13] = -Dot(Y, eye);
    m16.values[14] = -Dot(Z, eye);
    m16.values[15] = 1.0f;
}

void OrthoGraphic(const float l, float r, float b, const float t, float zn, const float zf, Matrix16& m16)
{
    m16.values[0] = 2 / (r - l);
    m16.values[1] = 0.0f;
    m16.values[2] = 0.0f;
    m16.values[3] = 0.0f;
    m16.values[4] = 0.0f;
    m16.values[5] = 2 / (t - b);
    m16.values[6] = 0.0f;
    m16.values[7] = 0.0f;
    m16.values[8] = 0.0f;
    m16.values[9] = 0.0f;
    m16.values[10] = 1.0f / (zf - zn);
    m16.values[11] = 0.0f;
    m16.values[12] = (l + r) / (l - r);
    m16.values[13] = (t + b) / (b - t);
    m16.values[14] = zn / (zn - zf);
    m16.values[15] = 1.0f;
}

inline void rotationY(const float angle, Matrix16& m16)
{
    float c = cosf(angle);
    float s = sinf(angle);

    m16.values[0] = c;
    m16.values[1] = 0.0f;
    m16.values[2] = -s;
    m16.values[3] = 0.0f;
    m16.values[4] = 0.0f;
    m16.values[5] = 1.f;
    m16.values[6] = 0.0f;
    m16.values[7] = 0.0f;
    m16.values[8] = s;
    m16.values[9] = 0.0f;
    m16.values[10] = c;
    m16.values[11] = 0.0f;
    m16.values[12] = 0.f;
    m16.values[13] = 0.f;
    m16.values[14] = 0.f;
    m16.values[15] = 1.0f;
}

template<typename T>
std::optional<T> ifFlag(bool flag, const T& value)
{
    if (flag)
        return value;
    else
        return std::nullopt;
}

void EditTransform(
    Matrix16& cameraView, // may be modified
    const Matrix16& cameraProjection,
    Matrix16& objectMatrix, // may be modified
    bool editTransformDecomposition)
{
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
    static bool useSnap = false;
    static Matrix3 snap({ 1.f, 1.f, 1.f });
    static Matrix6 bounds({ -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f });
    static Matrix3 boundsSnap({ 0.1f, 0.1f, 0.1f });
    static bool boundSizing = false;
    static bool boundSizingSnap = false;

    if (editTransformDecomposition)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) // r Key
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::RadioButton("Universal", mCurrentGizmoOperation == ImGuizmo::UNIVERSAL))
            mCurrentGizmoOperation = ImGuizmo::UNIVERSAL;
        auto matrixComponents = ImGuizmo::DecomposeMatrixToComponents(objectMatrix);
        bool edited = false;
        edited |= ImGui::InputFloat3("Tr", matrixComponents.Translation.values);
        edited |= ImGui::InputFloat3("Rt", matrixComponents.Rotation.values);
        edited |= ImGui::InputFloat3("Sc", matrixComponents.Scale.values);
        if (edited) {
            objectMatrix = ImGuizmo::RecomposeMatrixFromComponents(matrixComponents);
        }

        if (mCurrentGizmoOperation != ImGuizmo::SCALE)
        {
            if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                mCurrentGizmoMode = ImGuizmo::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                mCurrentGizmoMode = ImGuizmo::WORLD;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            useSnap = !useSnap;
        ImGui::Checkbox("##UseSnap", &useSnap);
        ImGui::SameLine();

        switch (mCurrentGizmoOperation)
        {
            case ImGuizmo::TRANSLATE:
                ImGui::InputFloat3("Snap", &snap.values[0]);
                break;
            case ImGuizmo::ROTATE:
                ImGui::InputFloat("Angle Snap", &snap.values[0]);
                break;
            case ImGuizmo::SCALE:
                ImGui::InputFloat("Scale Snap", &snap.values[0]);
                break;
            default:
                break;
        }
        ImGui::Checkbox("Bound Sizing", &boundSizing);
        if (boundSizing)
        {
            ImGui::PushID(3);
            ImGui::Checkbox("##BoundSizing", &boundSizingSnap);
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", boundsSnap.values);
            ImGui::PopID();
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    float viewManipulateRight = io.DisplaySize.x;
    float viewManipulateTop = 0;
    static ImGuiWindowFlags gizmoWindowFlags = 0;
    if (useWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImVec2(400,20), ImGuiCond_Appearing);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)ImColor(0.35f, 0.3f, 0.3f));
        ImGui::Begin("Gizmo", NULL, gizmoWindowFlags);
        ImGuizmo::SetDrawlist();
        float windowWidth = (float)ImGui::GetWindowWidth();
        float windowHeight = (float)ImGui::GetWindowHeight();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);
        viewManipulateRight = ImGui::GetWindowPos().x + windowWidth;
        viewManipulateTop = ImGui::GetWindowPos().y;
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        gizmoWindowFlags = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max) ? ImGuiWindowFlags_NoMove : 0;
    }
    else
    {
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    }

    ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);

    ImGuizmo::DrawCubes(cameraView, cameraProjection, vec_n_first(gObjectMatrix, gizmoCount));

    ImGuizmo::Manipulate(
        cameraView,
        cameraProjection,
        mCurrentGizmoOperation,
        mCurrentGizmoMode,
        objectMatrix,
        std::nullopt,
        ifFlag(useSnap, snap),
        ifFlag(boundSizing, bounds),
        ifFlag(boundSizingSnap, boundsSnap)
    );

    ImGuizmo::ViewManipulate(
        cameraView,
        camDistance,
        ImVec2(viewManipulateRight - 128, viewManipulateTop),
        ImVec2(128, 128),
        0x10101010);

    if (useWindow)
    {
        ImGui::End();
        ImGui::PopStyleColor(1);
    }
}

// This returns a closure function that will later be invoked to run the app
GuiFunction make_closure_demo_guizmo_pure()
{
    int lastUsing = 0;

    Matrix16 cameraView(
        { 1.f, 0.f, 0.f, 0.f,
          0.f, 1.f, 0.f, 0.f,
          0.f, 0.f, 1.f, 0.f,
          0.f, 0.f, 0.f, 1.f });

    Matrix16 cameraProjection;

    // Camera projection
    bool isPerspective = true;
    float fov = 27.f;
    float viewWidth = 10.f; // for orthographic
    float camYAngle = 165.f / 180.f * 3.14159f;
    float camXAngle = 32.f / 180.f * 3.14159f;

    bool firstFrame = true;

    auto gui = [=]() mutable // mutable => this is a closure
    {
        ImGuiIO& io = ImGui::GetIO();
        if (isPerspective)
        {
            Perspective(fov, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f, cameraProjection);
        }
        else
        {
            float viewHeight = viewWidth * io.DisplaySize.y / io.DisplaySize.x;
            OrthoGraphic(-viewWidth, viewWidth, -viewHeight, viewHeight, 1000.f, -1000.f, cameraProjection);
        }
        ImGuizmo::SetOrthographic(!isPerspective);
        ImGuizmo::BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(1024, 100), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(256, 256), ImGuiCond_Appearing);

        // create a window and insert the inspector
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(320, 340), ImGuiCond_Appearing);
        ImGui::Begin("Editor");
        if (ImGui::RadioButton("Full view", !useWindow)) useWindow = false;
        ImGui::SameLine();
        if (ImGui::RadioButton("Window", useWindow)) useWindow = true;

        ImGui::Text("Camera");
        bool viewDirty = false;
        if (ImGui::RadioButton("Perspective", isPerspective)) isPerspective = true;
        ImGui::SameLine();
        if (ImGui::RadioButton("Orthographic", !isPerspective)) isPerspective = false;
        if (isPerspective)
        {
            ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);
        }
        else
        {
            ImGui::SliderFloat("Ortho width", &viewWidth, 1, 20);
        }
        viewDirty |= ImGui::SliderFloat("Distance", &camDistance, 1.f, 10.f);
        ImGui::SliderInt("Gizmo count", &gizmoCount, 1, 4);

        if (viewDirty || firstFrame)
        {
            Matrix3 eye({ cosf(camYAngle) * cosf(camXAngle) * camDistance, sinf(camXAngle) * camDistance, sinf(camYAngle) * cosf(camXAngle) * camDistance });
            Matrix3 at({ 0.f, 0.f, 0.f });
            Matrix3 up({ 0.f, 1.f, 0.f });
            LookAt(eye, at, up, cameraView);
            firstFrame = false;
        }

        ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
        if (ImGuizmo::IsUsing())
        {
            ImGui::Text("Using gizmo");
        }
        else
        {
            ImGui::Text(ImGuizmo::IsOver()?"Over gizmo":"");
            ImGui::SameLine();
            ImGui::Text(ImGuizmo::IsOver(ImGuizmo::TRANSLATE) ? "Over translate gizmo" : "");
            ImGui::SameLine();
            ImGui::Text(ImGuizmo::IsOver(ImGuizmo::ROTATE) ? "Over rotate gizmo" : "");
            ImGui::SameLine();
            ImGui::Text(ImGuizmo::IsOver(ImGuizmo::SCALE) ? "Over scale gizmo" : "");
        }
        ImGui::Separator();
        for (int matId = 0; matId < gizmoCount; matId++)
        {
            ImGuizmo::PushID(matId);

            EditTransform(cameraView, cameraProjection, gObjectMatrix[matId], lastUsing == matId);
            if (ImGuizmo::IsUsing())
            {
                lastUsing = matId;
            }
            ImGuizmo::PopID();
        }

        ImGui::End();

    };
    return gui;
}


#ifndef IMGUI_BUNDLE_BUILD_DEMO_AS_LIBRARY
int main(int, char**)
{
    auto gui = make_closure_demo_guizmo_pure();

    // Run app
    HelloImGui::RunnerParams runnerParams;
    runnerParams.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
    runnerParams.imGuiWindowParams.enableViewports = true;
    runnerParams.callbacks.ShowGui = gui;
    runnerParams.appWindowParams.windowGeometry.size = {1200, 800};
    // Docking Splits
    {
        HelloImGui::DockingSplit split;
        split.initialDock = "MainDockSpace";
        split.newDock = "EditorDock";
        split.direction = ImGuiDir_Left;
        split.ratio = 0.25f;
        runnerParams.dockingParams.dockingSplits = {split};
    }
    // Dockable windows
    HelloImGui::DockableWindow winEditor;
    winEditor.label = "Editor";
    winEditor.dockSpaceName = "EditorDock";
    HelloImGui::DockableWindow winGuizmo;
    winGuizmo.label = "Gizmo";
    winGuizmo.dockSpaceName = "MainDockSpace";
    runnerParams.dockingParams.dockableWindows = { winEditor, winGuizmo};

    ImmApp::Run(runnerParams);
    return 0;
}
#endif

#else // #ifdef IMGUI_BUNDLE_WITH_IMGUIZMO
#ifndef IMGUI_BUNDLE_BUILD_DEMO_AS_LIBRARY
#include <cstdio>
int main(int, char **) { printf("This demo requires ImGuizmo\n"); return 0; }
#endif
#endif
