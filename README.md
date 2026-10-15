# Zombie Shooter 2 Engine (Steam)

A Visual Studio 2022 **Win32/x86** C++ project for the engine of **Zombie Shooter 2 Engine (Steam)**. This repository is intended for preservation, source-level maintenance and study of the Reloaded engine.

This package contains source code, the Visual Studio project, the Win32 executable resource payload and the bundled Ogg/Vorbis code. It **does not** contain an executable, a complete game installation or the original game-data files.

## Requirements

- Windows and Visual Studio 2022 with **Desktop development with C++**.
- The **MSVC v143** toolset, **Win32/x86** support and the **Windows 10 SDK**.
- A legally obtained installation of **Zombie Shooter 2 (Steam)** for runtime assets.
- The **32-bit DirectX 9 runtime** dependency, including `d3dx9_31.dll` (provided through an appropriate DirectX installation or beside the game executable).

Bundled Ogg/Vorbis sources are compiled into the project. The Windows SDK supplies the Direct3D import libraries. No other edition's project settings or resources are used.

## Build

Open `ZombieShooter2.sln` in Visual Studio 2022 and select **Win32** with the desired configuration:

| Configuration | Platform |
| --- | --- |
| `Release` | `Win32` |
| `Debug` | `Win32` |

The build keeps the Reloaded executable's established PE32 **OS 4.0 / subsystem 4.0** header settings.

## Running

Place the newly built `ZombieShooter2.exe` in the directory of the matching **Zombie Shooter 2 (Steam)** installation. Run it with that directory as the working directory; retain a backup of the original executable. Original game files are required and are not distributed here.

## Repository layout

- `src/` — Reloaded engine and game code, including graphics, maps, gameplay, scripts and interface.
- `src/zm/` — Reloaded gameplay, inventory, RPG and menu systems.
- `third_party/` — bundled Ogg/Vorbis and their license notices.
- `resources/` — executable export definitions and the required Win32 resource payload.
- `vs2022/` — Visual Studio 2022 project and file filters.
- `tools/build/` — Reloaded-specific MSBuild PE header support.
- `ZombieShooter2.sln` and `BUILD.bat` — build entry points.

## Third-party notices

The included **libogg** and **libvorbis** components retain their respective license notices:

- [libogg COPYING](third_party/libogg/COPYING)
- [libvorbis COPYING](third_party/libvorbis/COPYING)

See [LICENSE.md](LICENSE.md) for the source-package rights notice. The Alien Shooter title and original game content belong to their respective rights holders. The included Win32 resource payload remains subject to the relevant rights.
