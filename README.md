# Plotters for visualization in Pothos GUI

The plotters toolkit gives plotter widgets: processing blocks with Qt
widgets that show signals in the Pothos GUI. The blocks are the wave
monitor (time domain), the periodogram and the spectrogram (frequency
domain), the constellation (IQ samples) and the logic analyzer. The
toolkit also gives 4 control widgets on Qwt: slider, thermo, knob and
dial.

This branch (`pothos2`) builds against the modernized core
(https://github.com/markvin/PothosCore, branch `pothos2`). The modules use
only the public API of the core.

## Dependencies

- an installed Pothos core (`find_package(Pothos)`);
- Qt 6.5 or higher with the Widgets component (for example the Arch Linux
  package `qt6-base`, or a Qt install from the Qt installer). The Qt Gui
  CMake package also needs the OpenGL headers (on Debian and Ubuntu, for
  example `libgl-dev`);
- Qwt 6.3 or higher, built for Qt 6 (for example the Arch Linux package
  `qwt`, or a Qwt that you build from https://qwt.sourceforge.io with the
  `qmake` of your Qt 6);
- at run time: PothosComms (the plotters use `/comms/wave_trigger`). The
  self-tests also need PothosBlocks (`/blocks/feeder_source`);
- CMake 3.25 or higher and a C++23 compiler.

When Qt 6 or Qwt is missing or too old, the configure step stops with an
error. Qt 5 is not supported. A Qwt that is built for Qt 5 is refused at
the configure step.

On Windows with a current MSVC (Visual Studio 2026), use Qt 6.8 or higher:
the headers of Qt 6.5 do not compile with its standard library. The CI
uses Qt 6.5.3 on Linux and macOS, and Qt 6.8.3 on Windows, with Qwt 6.3.0
built from source.

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=<core prefix>
cmake --build build
ctest --test-dir build --output-on-failure
```

`<core prefix>` is the installation prefix of the Pothos core. Do not set
it when the core is installed in a default location of the system.

When Qt or Qwt is not in a default location of the system, add its prefix
to `CMAKE_PREFIX_PATH` (for example
`-DCMAKE_PREFIX_PATH="<core prefix>;$HOME/Qt/6.8.0/gcc_64;$HOME/qwt-6.3.0"`),
or give `Qt6_DIR` and `Qwt_ROOT`. On Linux and macOS, the installed
modules then have an RPATH to the Qt and Qwt library directories. On
Windows, put the Qt `bin` directory and the Qwt DLL directory on `PATH`.

`cmake/FindQwt.cmake` finds Qwt with pkg-config (the module `Qt6Qwt6`),
or else in `CMAKE_PREFIX_PATH`. Without pkg-config, it examines the
libraries that the Qwt library uses, and refuses a Qwt for Qt 5.

Install the modules and the library `PothosPlotterUtils`:

```sh
cmake --install build
```

The core loads modules only from its own prefix (and from
`POTHOS_PLUGIN_PATH`). Thus the default install prefix of this project is
the prefix of the core. When you give `CMAKE_INSTALL_PREFIX`, set
`POTHOS_PLUGIN_PATH` to `<prefix>/lib/Pothos/modules<ABI version>`.

## Tests

The test `plotters.stage` installs the modules into `build/stage`. Then
`plotters.selftests` runs each self-test (`/plotters/tests/...`) with the
PothosUtil of the core and `POTHOS_PLUGIN_PATH` set to the staged modules.

Install PothosComms and PothosBlocks into the prefix of the core before
you run the tests. Without them, `plotters.selftests` stops with an error.

The self-tests:

- compare the window functions with reference data (the values of the
  spuce library that the toolkit used before);
- make each block, call its setters and read the values back;
- send samples through each plotter and look at the plot.

They need no display: CTest sets `QT_QPA_PLATFORM=offscreen`. When you run
a self-test without CTest and no Qt application exists, the test also
uses the `offscreen` platform:

```sh
POTHOS_PLUGIN_PATH=build/stage/lib/Pothos/modules<ABI version> \
    <core prefix>/bin/PothosUtil --self-test1 /plotters/tests/test_wave_monitor
```

`plotters.selftests` stops with an error when the plotters modules are
also installed in the prefix of the core (the core would load the two
copies). Remove the installed copy, or test against another installation
of the core.

## Documentation

* https://github.com/pothosware/PothosPlotters/wiki

## Licensing information

Use, modification and distribution is subject to the Boost Software
License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
