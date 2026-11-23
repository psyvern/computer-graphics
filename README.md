# Computer Graphics Project (`CG-PROJ`)

A 3D OpenGL project featuring a drivable car.

---

## Prerequisites & Required Versions

To build and run this project, make sure the following tools and libraries are installed on your system:

* **C++ Compiler:** GCC 13+ or Clang 16+ with **C++20** support (required for `<numbers>` and `<format>`).
* **Build System:** CMake **3.16+** and `make`
* **OpenGL:** OpenGL **3.3** Core Profile compatible GPU/drivers
* **GLFW:** **3.4+** (required for `GLFW_PLATFORM` / `GLFW_PLATFORM_X11` hints)
* **GLEW:** **2.1+**
* **GLM:** **0.9.9+** (with `GLM_ENABLE_EXPERIMENTAL`)
* **Assimp:** **5.0+**

## Building the Project

Generate `build/` repository using cmake.

To build the project, run `make` from `build/`

---

## Running the Project

**Important:** Run the executable from the **project root directory** (not from inside `build/`), because asset paths (`./car.obj`, `./wheel.obj`, `./cube_2.obj`, `./car.png`) are resolved relative to the current working directory:

```bash
./build/main
```

### Controls

| Action | Key |
| :--- | :--- |
| Drive Forward / Backward | `W` / `S` |
| Steer / Air Yaw & Roll | `A` / `D` |
| Jump / Air Flip | `Space` |
| Boost / Air Roll Modifier | `Left Shift` |
| Camera Angle | `Arrow Keys` |
| Reset Orientation | `R` |
| Pause / Step Frame | `P` / `L` |
| Quit | `Esc` |