# Rive Unreal

An Unreal runtime library for [Rive](https://rive.app). This is currently a **technical preview** for Mac and Windows installs of Unreal. We're hoping to gather feedback about the API and feature-set as we expand platform support.

### Rendering Support

Currently supported platforms and backends include:

- Metal on Mac
- Metal on iOS
- D3D11 on Windows
- OpenGL on Android

Planned support for:

- D3D12 on Windows
- Vulkan on Linux, Android, Windows

### Feature Support

The rive-unreal runtime utilizes the latest Rive C++ runtime. All Rive features are supported for playback. Work is in progress to add runtime configuration support to the Unreal package for Rive-specific features.

## Table of contents

- ⭐️ [Rive Overview](#rive-overview)
- 🚀 [Getting Started](#getting-started)
- 👨‍💻 [Contributing](#contributing)
- ❓ [Issues](#issues)

## Rive Overview

[Rive](https://rive.app) is a real-time interactive design and animation tool that helps teams
create and run interactive animations anywhere. Designers and developers use our collaborative
editor to create motion graphics that respond to different states and user inputs. Our lightweight
open-source runtime libraries allow them to load their animations into apps, games, and websites.

🏡 [Homepage](https://rive.app/)

📘 [General help docs](https://help.rive.app/) 

📘 Rive Unreal docs (Coming soon!)

🛠 [Learning Rive](https://rive.app/learn-rive/)

## Getting Started

Check out the [example project](https://github.com/rive-app/rive-unreal/releases/download/0.1.0/SampleProjectSource.zip) to see a few techniques for adding Rive graphics in an Unreal project.

To see how to add Rive to your Unreal project and how to navigate the example project, see some quick docs in the [0.1.0 release here](https://github.com/rive-app/rive-unreal/releases/tag/0.1.0). Stay tuned for more detailed and official documentation on using Rive in Unreal.

You will need to work in the **Unreal 5.3** editor that supports OpenGL or D3D11 for Windows, or a Mac with ARM64 (M1, M2, etc) architecture.

Select either D3D11/OpenGL for Windows, or Metal for Mac/iOS as the Renderer ID under Editor Preferences -> Plugins - Graphics Switching in Unreal.

### Adding the Rive Unreal Plugin

For this technical preview, you can add the Rive Unreal plugin to your existing Unreal projects by downloading the plugin from the Github Releases, and copy/pasting the Rive folder into your `Plugins/` folder of your project (or creating one). 

Now, you should be able to drag in `.riv` files to your Unreal project to generate a new Widget Blueprint, and a Rive File definition.

### Awesome Rive

For even more examples and resources on using Rive at runtime or in other tools, check out the [awesome-rive](https://github.com/rive-app/awesome-rive) repo.

## Issues

Have an issue with using the runtime, or want to suggest a feature/API to help make your development
life better? Log an issue in our [issues](https://github.com/rive-app/rive-unreal/issues) tab! You
can also browse older issues and discussion threads there to see solutions that may have worked for
common problems.
