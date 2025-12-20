Repository Guidelines

  ## Project Structure & Module Organization

  The project follows a standard directory structure for a D3D12 graphics application:

  - src/: Contains main source code files
      - D3DManager.cpp/D3DManager.h: Manages the D3D12 environment initialization and rendering pipeline
      - D3D_2.cpp/D3D_2.h: Main program entry point, handles window creation and user interaction
      - PrimitiveShape.cpp/PrimitiveShape.h: Generates vertex and index data for various geometric shapes
      - SceneObject.cpp/SceneObject.h: Manages scene objects with position, rotation, scale properties
      - Shaders.hlsl: Contains HLSL shader code for rendering effects
  - assets/: Static resources like shaders, textures, configs, etc.
  - docs/: Documentation files for graphics concepts and APIs
  - scripts/: Build and utility scripts for compiling shaders, generating mipmaps, etc.

  ## Build, Test, and Development Commands

  To develop locally:

  - cmake --build . -t install -DCMAKE_BUILD_TYPE=Debug -DD3D12_ENABLED=ON - Install with D3D12 support
  - .\tests\run_tests.bat - Runs all graphics tests
  - .\scripts\build.sh - Builds the application (if Makefile exists)
  - .\bin\app.exe - Runs the graphics application

  ## Coding Style & Naming Conventions

  - Use 4-space indentation
  - Follow C++ style guidelines appropriate for graphics programming
  - Use snake_case for variables and functions
  - Use PascalCase for classes
  - All files should have a descriptive name starting with lowercase letter

  ## Testing Guidelines

  Tests are written using a custom testing framework designed for graphics applications.

  - Test files should be named test_*.cpp
  - Each test should be isolated and self-contained
  - Coverage target: 90% minimum for critical rendering paths
  - Run tests with: .\tests\run_tests.bat

  ## Commit & Pull Request Guidelines

  Commit messages should follow conventional commits:

  - feat:, fix:, docs:, style:, refactor:, perf:, test:, chore:
  - Include issue number if fixing a bug: fix #123
  - PRs must include:
      - Clear description of changes to graphics behavior or performance
      - Link to relevant issues
      - Screenshots or GIFs if visual changes
      - Test coverage report

  ## Program Summary

  This is an interactive 3D graphics application built with DirectX 12 API. The program allows users to:

  1. Create and manage various basic geometric shapes (sphere, cylinder, plane, cube, tetrahedron) through a menu system
  2. Navigate and interact with the 3D scene using keyboard (WASDQE) and mouse controls
  3. Select, move, and delete objects in the scene
  4. Experience real-time rendering with proper lighting and shading effects

  The program architecture consists of:

  - D3DManager class: Handles D3D12 device creation, command lists, swap chain management
  - PrimitiveShape class: Generates vertex and index data for different geometric shapes
  - SceneObject class: Manages individual scene objects with position, rotation, scale properties
  - Main program (D3D_2.cpp): Sets up the window, initializes D3D12, and processes user input

  This application serves as an excellent learning tool for understanding modern graphics programming with DirectX 12,
  demonstrating best practices in resource management, state transitions, and efficient rendering techniques.