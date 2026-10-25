// ===========================================================================
// gravsim -- the window.
//
// Each frame:
//   1. read the mouse and keyboard
//   2. advance the physics by a FIXED number of steps (see Simulation.hpp)
//   3. turn physics positions (double, AU) into GPU vertices (float)
//   4. draw trails, then bodies, then the control panel
//   5. show the finished picture
// The physics never sees steps 3-5. The renderer never changes the physics.
// ===========================================================================
#include "gfx/Gl.hpp"
#include "gfx/OrbitCamera.hpp"
#include "gfx/PointRenderer.hpp"
#include "gfx/TrailRenderer.hpp"
#include "app/Simulation.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Colour and on-screen size for body i. Heavy bodies (stars) are big and warm,
// light ones (planets, comets) small and cool.
static void styleFor(double mass, float rgb[3], float& size) {
    if (mass > 0.1) { rgb[0] = 1.00f; rgb[1] = 0.80f; rgb[2] = 0.45f; size = 26.0f; }
    else            { rgb[0] = 0.55f; rgb[1] = 0.80f; rgb[2] = 1.00f; size = 16.0f; }
}

int main(int argc, char** argv)
{
    // --frames N : run N frames then quit (a quick "does it work" check)
    // --scenario K : start on preset K
    int maxFrames = -1, startPreset = 0;
    for (int i = 1; i + 1 < argc; ++i) {
        if (!std::strcmp(argv[i], "--frames"))   maxFrames   = std::atoi(argv[i + 1]);
        if (!std::strcmp(argv[i], "--scenario")) startPreset = std::atoi(argv[i + 1]);
    }

    // ---- 1. Window and OpenGL context ------------------------------------
#ifdef GLFW_PLATFORM_X11
    // GLFW 3.4+ can use Wayland or X11. Under WSL, X11 is the route we
    // verified reaches your NVIDIA GPU (glxinfo), so ask for it explicitly.
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if (!glfwInit()) { std::fprintf(stderr, "glfwInit failed\n"); return 1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);   // modern GL only
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);            // enables error reporting
    glfwWindowHint(GLFW_SAMPLES, 4);                                 // smoother lines

    GLFWwindow* win = glfwCreateWindow(1400, 900, "gravsim", nullptr, nullptr);
    if (!win) {
        std::fprintf(stderr, "could not create a window with OpenGL 4.3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);                    // vsync: don't draw faster than the screen refreshes
    enableGlDebugOutput();

    const std::string renderer = (const char*)glGetString(GL_RENDERER);
    const std::string glVersion = (const char*)glGetString(GL_VERSION);
    std::printf("OpenGL renderer: %s\nOpenGL version : %s\n", renderer.c_str(), glVersion.c_str());

    // ---- 2. The control panel library (Dear ImGui) -----------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;               // don't litter imgui.ini files

    // ---- 3. Renderers, camera, simulation --------------------------------
    // Everything that owns GPU memory lives inside this { } block, so its
    // destructors free that memory BEFORE the window (and its OpenGL context)
    // is destroyed below. Freeing GPU memory with no context is an error.
    {
    PointRenderer points;
    TrailRenderer trails;
    if (!points.init() || !trails.init()) return 1;

    const std::vector<Preset> P = presets();
    int presetIdx = std::clamp(startPreset, 0, (int)P.size() - 1);

    Simulation sim;
    OrbitCamera cam;
    float yearsPerSecond = 0.0f;
    bool  paused = false, showTrails = true;
    int   focus = -1;                       // -1 = centre of mass, else a body index

    auto loadPreset = [&](int k) {
        sim.load(P[k]);
        cam.frame((float)P[k].viewRadius);
        yearsPerSecond = (float)P[k].yearsPerSecond;
        focus = -1;
    };
    loadPreset(presetIdx);

    std::vector<PointVertex>         pv;
    std::vector<TrailVertex>         tv;
    std::vector<std::pair<int,int>>  ranges;
    std::vector<Vec3>                trailPts;
    const int maxStepsPerFrame = 20000;

    double lastTime = glfwGetTime();
    int frame = 0;

    // ======================================================================
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        // Real time since last frame. In --frames mode use a fixed 1/60 s so
        // the check behaves the same on every machine.
        const double now = glfwGetTime();
        double realDt = (maxFrames > 0) ? 1.0 / 60.0 : std::min(now - lastTime, 0.1);
        lastTime = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ---- input: only if the mouse isn't over the panel ----------------
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                cam.orbit(io.MouseDelta.x, io.MouseDelta.y);
            if (io.MouseWheel != 0.0f) cam.zoom(io.MouseWheel);
        }
        if (!io.WantCaptureKeyboard && ImGui::IsKeyPressed(ImGuiKey_Space)) paused = !paused;

        // ---- physics ------------------------------------------------------
        if (!paused)
            sim.advance(realDt * yearsPerSecond * units::YEAR, maxStepsPerFrame);

        const State& s = sim.state();

        // ---- CAMERA-RELATIVE RENDERING --------------------------------------
        // Subtract the focus point in DOUBLE precision, THEN convert to float.
        // The GPU only ever sees small numbers near the camera, so zooming
        // in close never makes things jitter, however far from the Sun.
        const Vec3 focusPos = (focus < 0) ? s.centreOfMass() : s.pos[focus];
        auto rel = [&](const Vec3& p) {
            const Vec3 d = p - focusPos;
            return glm::vec3((float)d.x, (float)d.y, (float)d.z);
        };

        pv.clear();
        tv.clear();
        ranges.clear();
        for (std::size_t i = 0; i < s.size(); ++i) {
            float rgb[3], size;
            styleFor(s.mass[i], rgb, size);
            const glm::vec3 p = rel(s.pos[i]);
            pv.push_back({ p.x, p.y, p.z, rgb[0], rgb[1], rgb[2], size });

            if (showTrails) {
                sim.trail(i, trailPts);
                const int first = (int)tv.size(), n = (int)trailPts.size();
                for (int k = 0; k < n; ++k) {
                    const glm::vec3 q = rel(trailPts[k]);
                    const float age = (n > 1) ? (float)k / (n - 1) : 1.0f;   // 0 old .. 1 new
                    tv.push_back({ q.x, q.y, q.z, rgb[0], rgb[1], rgb[2], 0.85f * age });
                }
                ranges.push_back({ first, n });
            }
        }

        // ---- draw -----------------------------------------------------------
        int fbw, fbh;
        glfwGetFramebufferSize(win, &fbw, &fbh);    // pixels (can differ from window size)
        glViewport(0, 0, fbw, fbh);
        glClearColor(0.015f, 0.015f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = (fbh > 0) ? (float)fbw / fbh : 1.0f;
        const glm::mat4 viewProj = cam.proj(aspect) * cam.view();
        trails.draw(tv, ranges, viewProj);
        points.draw(pv, viewProj);

        // ---- control panel ---------------------------------------------------
        ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_Once);
        ImGui::Begin("gravsim");

        ImGui::TextDisabled("%s", renderer.c_str());
        ImGui::Separator();

        if (ImGui::BeginCombo("Scenario", P[presetIdx].name.c_str())) {
            for (int k = 0; k < (int)P.size(); ++k)
                if (ImGui::Selectable(P[k].name.c_str(), k == presetIdx)) {
                    presetIdx = k;
                    loadPreset(k);
                }
            ImGui::EndCombo();
        }
        if (ImGui::Button(paused ? "Play  [space]" : "Pause [space]")) paused = !paused;
        ImGui::SameLine();
        if (ImGui::Button("Reset")) loadPreset(presetIdx);

        ImGui::SliderFloat("Years/second", &yearsPerSecond, 0.001f, 50.0f, "%.3f",
                           ImGuiSliderFlags_Logarithmic);
        ImGui::Checkbox("Trails", &showTrails);

        // Focus: what the camera centres on
        std::string focusName = (focus < 0) ? "Centre of mass" : s.name[focus];
        if (ImGui::BeginCombo("Follow", focusName.c_str())) {
            if (ImGui::Selectable("Centre of mass", focus < 0)) focus = -1;
            for (int i = 0; i < (int)s.size(); ++i)
                if (ImGui::Selectable(s.name[i].c_str(), focus == i)) focus = i;
            ImGui::EndCombo();
        }

        ImGui::Separator();
        // ---- live diagnostics: the physics checking itself ----------------
        const Diagnostics d  = diagnose(s);
        const Diagnostics& d0 = sim.baseline();
        const double dE = (d.total - d0.total) / std::fabs(d0.total);
        const double dL = (norm(d.angularMomentum) - norm(d0.angularMomentum))
                          / norm(d0.angularMomentum);
        ImGui::Text("Time      %10.3f years", sim.time() / units::YEAR);
        ImGui::Text("Steps     %10ld", sim.steps());
        ImGui::Text("Energy    dE/E  = %+.2e", dE);
        ImGui::Text("Ang. mom. dL/L  = %+.2e", dL);
        ImGui::Text("Momentum  |P|   = %.2e", norm(d.momentum));
        ImGui::Separator();
        ImGui::TextDisabled("Drag: rotate   Scroll: zoom   %.0f fps", io.Framerate);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);                    // show the finished frame

        if (maxFrames > 0 && ++frame >= maxFrames) {
            std::printf("ran %d frames, %ld steps, dE/E = %+.2e\n", frame, sim.steps(), dE);
            break;
        }
    }

    }   // renderers destroyed here, while OpenGL still exists

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}