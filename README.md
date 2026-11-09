# ChromiumAirlock
A standalone, out-of-process host for Chromium that acts as a protective buffer between the web engine and your application. It decouples Chromium into an external, captive process, exposing rendering, navigation, and network primitives over a local JSON-RPC WebSocket interface.

## Why out of process
Chromium's internal architecture is multi-process by design, isolating tabs, rendering engines, and GPU tasks from each other. However, standard embedded frameworks (like CEF or WebView) stop short of isolating the browser from the host itself: they require your application process to double as the Chromium Browser Process.

In that model, the host application is hostage to the web engine. A fatal assertion, unhandled error, memory pressure spike, or DCHECK abort within Chromium takes down your entire application process with zero opportunity for recovery. It also surrenders navigation control, forcing the host to react to the browser's decisions rather than driving them.

**ChromiumAirlock** reverses that relationship:
- **Fault & Crash Buffering**: Chromium runs in its own process tree on the other side of the airlock. If Chromium panics or crashes, your application hull remains intact, retaining full control to restart, inspect, or tear down the engine.
- **Host Sovereignty**: The web engine is reduced to what it is best at: fetching web resources, executing JavaScript, and rendering pages. All authority over navigation, popups, window creation, downloads, and lifecycle decisions remains with the host application.
- **Language & Runtime Agnostic**: Because communication takes place over standard protocols, host applications do not need to link C/C++ runtimes or match Chromium's compiler toolchains. Any environment capable of opening a local WebSocket can control the browser.

## How it works
- **WebSocket Interface**: The airlock process manages Chromium's browser and child processes, listening on a dedicated local WebSocket port. It enforces a single-client policy dedicated entirely to the controlling host.
- **JSON-RPC Protocol**: The host issues commands (navigating, evaluating scripts, injecting user input, resizing viewport) and receives notifications through lightweight JSON messages.
- **Low-Overhead Frame Sharing**: Operates in CEF's off-screen rendering (OSR) mode, and rendered output is passed back to the host with minimal latency:
  - **Software OSR**: Dirty rectangles and pixel buffers are written to shared memory objects and duplicated directly into the host process space.
  - **Hardware-Accelerated OSR**: Direct3D shared texture handles are passed directly to the host for native GPU compositing.

## Command-line arguments
The executable accepts the following switches on startup:
- `--server-port=<int>` (Required): The local TCP port the WebSocket server listens on.
- `--application-process-id=<int>` (Required): The PID of your host application, used for Win32 handle duplication.
- `--server-ready-event-name=<string>` (Optional): A named Win32 Event signaled as soon as the WebSocket server is initialized and ready to accept connections.

Standard Chromium command-line switches can also be forwarded directly.

## Building

### Requirements
- Windows 10/11
- Visual Studio 2022 (MSVC with C++20 support)
- CMake 3.21 or newer
- Python 3.9+ (used by CMake to automatically download the CEF binary distribution)

### Build Steps
1. Open a terminal and create a build directory: mkdir build cd build
2. Generate the Visual Studio 2022 solution for 32-bit Windows: cmake -G "Visual Studio 17 2022" -A Win32 ..
3. Compile the Release build: cmake --build . --config Release

CMake will download and unpack the prebuilt CEF distribution automatically during configuration. You can also open build\cef.sln directly in Visual Studio to build or debug. 
