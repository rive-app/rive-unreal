# Rive Unreal

An Unreal runtime library for [Rive](https://rive.app). This is currently an Alpha for Mac and Windows installs of Unreal. We're hoping to gather feedback about the API and feature-set as we expand platform support.

PLEASE WATCH [THIS VIDEO](https://ucarecdn.com/a320730a-abb9-48cc-b945-7fb4ad65767c/)  ON OUR [GETTING STARTED PAGE](https://rive.app/docs/game-runtimes/unreal/getting-started) TO AVOID COMMON ISSUES!

### Rendering support

Currently supported platforms and backends include:

- Metal on Mac
- Metal on iOS
- D3D11 & D3D12 on Windows
- OpenGL on Android

Planned support for:

- Vulkan on Linux, Android, Windows

### Feature support

The rive-unreal runtime uses the latest Rive C++ runtime.

| Feature                      | Supported   |
| ---------------------------- | ----------- |
| Animation Playback           | ✅           |
| Fit and Alignment            | ✅           |
| Listeners                    | ✅           |
| Setting State Machine Inputs | ✅           |
| Listening to Events          | ✅           |
| Updating text at runtime     | ✅           |
| Out-of-band assets           | ✅           |
| Procedural rendering         | Coming soon |
| PNG images                   | ✅           |
| WEBP and JPEG images         | Coming soon |

## Table of contents

- ⭐️ [Rive Overview](#rive-overview)
- 🚀 [Getting Started](#getting-started)
- 👨‍💻 [Contributing](#contributing)
- ❓ [Issues](#issues)

## Rive overview

[Rive](https://rive.app) is a real-time interactive design and animation tool that helps teams
create and run interactive animations anywhere. Designers and developers use our collaborative
editor to create motion graphics that respond to different states and user inputs. Our lightweight
open-source runtime libraries allow them to load their animations into apps, games, and websites.

🏡 [Homepage](https://rive.app/)

📘 [Rive docs](https://rive.app/docs/getting-started/introduction) 

📘 [Rive Unreal docs](https://rive.app/docs/game-runtimes/unreal/unreal)

🛠 [Learning Rive](https://rive.app/learn-rive/)

🛠 [Rive Community](https://community.rive.app/feed) 

## Getting started

You can find the source for the rive-unreal-demos [here](https://github.com/rive-app/rive-unreal-demos).

You will need to work in the **Unreal 5.3, 5.4, or 5.5** editor that supports OpenGL, D3D11, or D3D12 for Windows, or a Mac with ARM64 (M1, M2, etc) architecture.

Select either D3D11/D3D12/OpenGL for Windows, or Metal for Mac/iOS as the Renderer ID under Editor Preferences -> Plugins - Graphics Switching in Unreal. If you are making an Android Build, make sure to use OpenGL instead of Vulcan.

### Adding the Rive Unreal plugin

You can add the Rive Unreal plugin to your existing Unreal projects by downloading the plugin from the Github Releases, and copy/pasting the Rive folder into your `Plugins/` folder of your project (or creating one). 

Then drag in a `.riv` file to your Unreal project to generate a new Widget Blueprint, Texture Object, and a Rive File definition.

### Awesome Rive

For even more examples and resources on using Rive at runtime or in other tools, check out the [awesome-rive](https://github.com/rive-app/awesome-rive) repo.

## Issues

Have an issue with using the runtime, or want to suggest a feature/API to help make your development
life better? Log an issue in our [issues](https://github.com/rive-app/rive-unreal/issues) tab! You
can also browse older issues and discussion threads there to see solutions that may have worked for
common problems.
