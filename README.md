# GPU-Accelerated Particle Life (DirectX 11 & C++)

A highly performant, real-time "Particle Life" simulation running entirely on the GPU. Built natively for Windows using C++ and DirectX 11. It features emergent cellular/flocking behaviors, real-time dynamic rule mutation, mouse interaction, and a Dear ImGui control panel.

![Particle Life Simulation Demo](demo.mp4) 

### 🤖 AI Authorship Acknowledgment
The architectural design, DirectX 11 pipeline, HLSL shader programming, and C++ implementations in this repository were collaboratively generated with the assistance of an AI (Google Gemini). 

---

## 🚀 Features
* **Compute Shader Physics:** Capable of simulating hundreds of thousands of particles at 60+ FPS by keeping all physics calculations natively on the GPU.
* **Instanced Rendering:** Zero CPU-to-GPU data transfer during the render loop.
* **Interactive Sandbox:** Pause, mutate, and tweak the laws of physics (attraction matrices, radii, friction) in real-time.
* **Interactive Mouse Tools:** Summon gravitational wells or repulsive blasts to manipulate the swarms.
* **Procedural Rule Generation:** High-quality Mersenne Twister randomness ensures unique universe generation.

---

## 🛠️ Prerequisites & Dependencies

To compile and run this project, you need:
1. **Windows 10 or 11**
2. **Visual Studio Installer:** Ensure you have the **"Desktop development with C++"** workload installed. This provides the `cl.exe` compiler and the necessary Windows/DirectX SDKs.
3. **Dear ImGui (UI Library)**

### Installing Dear ImGui
Because this project compiles directly via the command line without a package manager, you must manually include the ImGui source files.

1. Download the latest source code zip from the [Dear ImGui GitHub](https://github.com/ocornut/imgui).
2. Extract the zip.
3. Inside this project's `src` folder, create a new folder named `imgui`.
4. Copy the following files from the extracted zip into `src/imgui/`:
   * `imgui.h`, `imgui_internal.h`, `imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h`, `imconfig.h`
   * `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`
5. Open the `backends/` folder in the extracted zip and copy these files into `src/imgui/` as well:
   * `imgui_impl_win32.h` & `imgui_impl_win32.cpp`
   * `imgui_impl_dx11.h` & `imgui_impl_dx11.cpp`

### Expected Directory Structure
Your project folder should look exactly like this before compiling:

```text
ParticleLifeDX/
│
├── src/
│   ├── main.cpp
│   ├── Renderer.h
│   ├── Renderer.cpp
│   └── imgui/
│       ├── imgui.cpp
│       ├── imgui_impl_dx11.cpp
│       └── (and the other ImGui files listed above)
│
├── shaders/
│   ├── Compute.hlsl
│   ├── Vertex.hlsl
│   ├── Pixel.hlsl
│   └── Matrix.hlsl
│
├── .gitignore
└── README.md
```

---

## ⚙️ Compilation

This project is built using the Microsoft C++ Compiler (`cl.exe`) from the command line.

1. Press the Windows key and search for **Developer Command Prompt for Visual Studio**. Open it.
2. Navigate to the root directory of this project:
   ```cmd
   cd path\to\ParticleLifeDX
   ```
3. Run the following command to compile the C++ files, compile the ImGui files, and link the Windows libraries:
   ```cmd
   cl /EHsc /W3 /std:c++17 /D UNICODE /D _UNICODE src\main.cpp src\Renderer.cpp src\imgui\*.cpp /I src /I src\imgui /link user32.lib /out:ParticleLife.exe
   ```

*(Note: DirectX libraries are linked automatically via `#pragma` directives in the source code).*

---

## 🎮 Usage

Run the generated executable from the root directory so it can locate the `shaders/` folder:
```cmd
ParticleLife.exe
```

### Controls
* **UI Panel:** Use the sliders on the right to adjust maximum distance, minimum overlap, friction, and simulation speed.
* **Mutate:** Click the Mutate button (or press the **Spacebar**) to instantly clear the screen, randomize the rules of physics, and scatter a new batch of particles.
* **Left Mouse Button (Hold):** Create a green gravity well to attract nearby particles.
* **Right Mouse Button (Hold):** Create a red repulsive field to blast particles apart.
* **Colors:** Use the color pickers in the UI to change the visual representation of any particle type.

---

## 📜 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.