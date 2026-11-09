# ChromiumAirlock
A standalone host process for the Chromium Embedded Framework (CEF) that acts as a protective buffer between Chromium and your application, exposing web controls and frame rendering over a local JSON-RPC WebSocket interface.


## Why out of process
Chromium's multi-process architecture isolates individual tabs, renderer engines, and GPU tasks from one another. However, standard CEF integrations stop short of protecting the host application itself: CEF forces your application process to double as the primary Chromium Browser Process.

When embedded in-process, any fatal browser-level assertion, DCHECK failure, memory spike, or unhandled crash inside Chromium abruptly terminates your entire application. For live programming environments and long-running host runtimes, an unexpected crash in the web engine is particularly catastrophic.

**chromium-airlock** functions as an active pressure buffer between the host and Chromium:
- **Host Protection & Fault Isolation**: The host application remains completely decoupled from Chromium's process tree. If Chromium hits a fatal assertion or panics, the host remains untouched and retains full supervisory control to restart or cleanly drop the connection.
- **Language & Runtime Agnostic**: Host applications do not need to link C/C++ libraries, manage native memory, or adhere to CEF's rigid threading models. Any environment capable of establishing a local WebSocket connection (Smalltalk, Rust, C#, Go, Python, etc.) can drive Chromium directly.
- **Clean Lifecycle Separation**: Eliminates message-loop synchronization, foreign signal handler installations, and runtime conflicts between the host runtime and CEF.

## How it works
- **WebSocket Interface**: Runs the CEF Browser Process (which spawns and supervises Chromium's renderer and utility child processes) and listens on a local WebSocket endpoint, accepting a single controlling host connection.
- **JSON-RPC Protocol**: Navigation, DOM/script evaluation, user input dispatch, and lifecycle notifications are exchanged as lightweight JSON messages.
- **Low-Overhead Frame Sharing**: When using off-screen rendering (OSR), frame buffers are delivered back to the host with minimal latency:
  - **Software OSR**: Dirty regions are mapped into Win32 shared memory and handed directly to the host process space.
  - **Hardware-Accelerated OSR**: Direct3D shared texture handles are passed directly to the host for zero-copy GPU compositing.

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