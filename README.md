# Walker Effects

Audio effect plugins built with [JUCE](https://juce.com) and CMake. Shared code lives in `components/` and is reused by each plugin in `plugins/`.

## Layout

```
walker_effects/
├── CMakeLists.txt
├── cmake/
│   └── WalkerPlugin.cmake      # walker_add_plugin, walker_add_component, walker_add_subdirectories
├── external/
│   └── JUCE/                   # git submodule
├── components/                 # shared static libraries
│   └── dsp/
│       ├── CMakeLists.txt
│       ├── inc/walker/dsp/     # public headers
│       └── src/                # sources
└── plugins/                    # one folder per plugin
    └── gain/
        ├── CMakeLists.txt
        └── src/
```

Folders and source files starting with `_` are skipped by the build, so work in progress can live in `_wip_reverb/` without being compiled.

## Requirements

- CMake 3.22+
- Ninja
- A C++20 compiler (GCC or Clang)
- Git

### Arch Linux

```bash
sudo pacman -Syu --needed \
  base-devel cmake ninja git pkgconf clang \
  alsa-lib freetype2 fontconfig \
  libx11 libxcomposite libxcursor libxext libxinerama libxrandr libxrender \
  mesa glu pipewire-jack
```

Use `jack2` instead of `pipewire-jack` if you run classic JACK. Do not install both.

### Debian / Ubuntu

```bash
sudo apt install build-essential cmake ninja-build pkg-config git \
  libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev
```

## Getting the code

```bash
git clone --recurse-submodules <repo-url>
cd walker_effects
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

## Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Build a single plugin:

```bash
cmake --build build --target WalkerGain_Standalone
cmake --build build --target WalkerGain_VST3
```

The first build compiles the JUCE modules and takes a few minutes. Later builds are incremental.

For a release build:

```bash
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

## Run

Standalone app:

```bash
./build/plugins/gain/WalkerGain_artefacts/Debug/Standalone/"Walker Gain"
```

If the path differs, find it with:

```bash
find build -type f -name "Walker Gain" -perm -u+x
```

The VST3 is copied to `~/.vst3/` after each build. Rescan plugins in your DAW to pick it up.

To validate a built plugin with [pluginval](https://github.com/Tracktion/pluginval):

```bash
pluginval --strictness-level 5 --validate-in-process ~/.vst3/"Walker Gain.vst3"
```

## Adding a component

1. Create `components/<name>/` with `inc/walker/<name>/` for headers and `src/` for sources.
2. Add a `CMakeLists.txt` containing:

   ```cmake
   walker_add_component(PUBLIC_LINK juce::juce_dsp)
   ```

3. Rebuild. The target is `walker_<name>` with the alias `walker::<name>`.

Include its headers as `#include <walker/<name>/Header.h>`. A component with no `.cpp` files becomes a header-only library.

## Adding a plugin

1. Create `plugins/<name>/src/` with a `PluginProcessor.h` and `PluginProcessor.cpp`.
2. Add a `CMakeLists.txt` containing:

   ```cmake
   walker_add_plugin(
       PLUGIN_CODE Wxyz
       LINK walker::dsp)
   ```

3. Rebuild.

`PLUGIN_CODE` is required. It must be exactly 4 characters and unique across all plugins, because hosts use it to identify the plugin. The target name and product name come from the folder name, so `plugins/tape_delay` becomes `WalkerTapeDelay` and "Walker Tape Delay". Override them with `NAME` and `PRODUCT_NAME` if needed.

## Editor setup (Neovim)

Symlink the compilation database to the project root so `clangd` can find it:

```bash
ln -sf build/compile_commands.json .
```

Add a `.clangd` file in the project root:

```yaml
CompileFlags:
  CompilationDatabase: build
Diagnostics:
  UnusedIncludes: None
```

Build from inside Neovim:

```lua
vim.opt.makeprg = 'cmake --build build'
```

Then use `:make` and `:copen` to see errors in the quickfix list.

## Troubleshooting

| Problem | Likely cause |
|---|---|
| `Unknown CMake command "walker_add_component"` | The helper file is not included before `add_subdirectory(components)` in the root `CMakeLists.txt`. |
| `walker/dsp/Smoother.h: No such file` | The header is not at `components/dsp/inc/walker/dsp/Smoother.h`. |
| `Target "walker::dsp" not found` | The component folder starts with `_`, has no `CMakeLists.txt`, or is added after `plugins`. |
| Missing `jack` at configure time | Install `pipewire-jack` or `jack2`. |
| New files not picked up | Rebuild so the glob re-runs, or rerun `cmake -B build`. |
