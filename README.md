
# Neuropixels PXI & OneBox Plugins

![Neuropixels PXI Plugin](https://open-ephys.github.io/gui-docs/_images/neuropix-pxi-01.png)

This repository contains two plugins for the [Open Ephys GUI](https://github.com/open-ephys/plugin-gui):

- **Neuropixels PXI**: Streams Neuropixels data from a PXI-based acquisition system.
- **OneBox**: Streams Neuropixels data from imec’s OneBox.


## Installation

These plugins are not included by default in the Open Ephys GUI. To install, use **ctrl-P** to access the Plugin Installer, browse to the “Neuropixels PXI” plugin, and click the “Install” button. This will also install the OneBox plugin. Alternatively, you can select the “Neuropixels” default configuration the first time the GUI is launched, and the plugin will be downloaded and installed automatically.

If you're upgrading from a previous version, close and re-open the GUI before using the plugin in a signal chain.


## Usage

- Instructions for using the Neuropixels PXI plugin are available [here](https://open-ephys.github.io/gui-docs/User-Manual/Plugins/Neuropixels-PXI.html).
- Instructions for using the OneBox plugin are available [here](https://open-ephys.github.io/gui-docs/User-Manual/Plugins/OneBox.html).


## Building from source

First, follow the instructions on [this page](https://open-ephys.github.io/gui-docs/Developer-Guide/Compiling-the-GUI.html) to build the Open Ephys GUI.

Be sure to clone the `neuropixels-pxi` repository into a directory at the same level as the `plugin-GUI`, e.g.:

```
Code
├── plugin-GUI
│   ├── Build
│   ├── Source
│   └── ...
├── OEPlugins
│   └── neuropixels-pxi
│       ├── Build
│       ├── Source
│       └── ...
```

### Windows (only)

**Requirements:** [Visual Studio](https://visualstudio.microsoft.com/) and [CMake](https://cmake.org/install/)

From the `Build` directory, generate the build files by entering:

```bash
cmake -G "Visual Studio 17 2022" -A x64 ..
```

Next, launch Visual Studio and open the `OE_PLUGIN_neuropixels-pxi.sln` file that was just created. Select the appropriate configuration (Debug/Release) and build the solution.

Selecting the `INSTALL` project and manually building it will copy the `.dll` and any other required files into the GUI's `plugins` directory. The next time you launch the GUI from Visual Studio, both the Neuropixels PXI and OneBox plugins should be available (if built).


## License

This source code is made available under a [GPL 3.0 License](LICENSE).

## Credits

This plugin was created by Josh Siegle, Pavel Kulik, and Anjal Doshi at the Allen Institute.

We thank Bill Karsh (Janelia) and Jan Putzeys (imec) for helpful advice throughout the development process.

© 2025 Allen Institute and Open Ephys
