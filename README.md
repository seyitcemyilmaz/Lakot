# Lakot

### Description

Lakot is a hobby game engine project I developed in my free time.
Android and Linux support will be added to the project in the future.

### Code Quality

[![CodeFactor](https://www.codefactor.io/repository/github/seyitcemyilmaz/lakot/badge)](https://www.codefactor.io/repository/github/seyitcemyilmaz/lakot)

### Installation

1. Install vcpkg. (From: https://vcpkg.io/en/getting-started.html)

```sh
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
vcpkg integrate install
```

2. Add vcpkg root directory as VCPKG_ROOT variable to path. After that, close the open terminals.

3. Open new terminal, and install necessary libraries.

```sh
vcpkg install glfw3 --triplet x64-windows
vcpkg install glad --triplet x64-windows
vcpkg install glm --triplet x64-windows
vcpkg install stb --triplet x64-windows
vcpkg install assimp --triplet x64-windows
vcpkg install spdlog --triplet x64-windows
vcpkg install imgui[core,opengl3-binding,glfw-binding,docking-experimental] --triplet x64-windows
```

4. Open project with Visual Studio, and build the project.
