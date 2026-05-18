---
name: smart-alarm-build
description: Builds, configures, and tests this Smart Alarm Qt/CMake project with the correct local Windows MinGW environment. Use when working in this repository and you need to run CMake, build, ctest, verify changes, or fix errors like "g++ cannot execute as" or missing as.exe.
---

# Smart Alarm Build

## Environment

Run all commands from the repository root:

```powershell
C:\_\smart-alarm
```

Always prepend the configured MinGW toolchain to `PATH` before configure, build, or test commands. Do this first; plain `cmake --build` can fail because `g++` may not find `as.exe`.

```powershell
$env:PATH = 'C:\Program Files\JetBrains\CLion 2025.3.1\bin\mingw\bin;' + $env:PATH
```

## Build

Use the existing debug build directory:

```powershell
$env:PATH = 'C:\Program Files\JetBrains\CLion 2025.3.1\bin\mingw\bin;' + $env:PATH; cmake --build cmake-build-debug --config Debug
```

## Test

Run the project tests through CTest:

```powershell
$env:PATH = 'C:\Program Files\JetBrains\CLion 2025.3.1\bin\mingw\bin;' + $env:PATH; ctest --test-dir cmake-build-debug --output-on-failure
```

## Configure If Needed

If `cmake-build-debug` is missing or stale, configure it with Ninja and Qt 6.8.3 MinGW:

```powershell
$env:PATH = 'C:\Program Files\JetBrains\CLion 2025.3.1\bin\mingw\bin;' + $env:PATH; cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\mingw_64
```

Then run the build and test commands above.

## Failure Pattern

If the build fails with:

```text
g++: fatal error: cannot execute 'as': CreateProcess: No such file or directory
```

the command was run without the MinGW `bin` path above. Rerun the build with the exact `PATH` prefix from this skill.
