# Muffin

*An open source alternative firmware for the excellent Midi Fighter Twister from DJ Tech Tools*

## About

This project was started to add new functionality to the Midi Fighter Twister.
It is based on [AVR-GCC](https://gcc.gnu.org/wiki/avr-gcc) and the excellent [LUFA USB libraries](http://www.fourwalledcubicle.com/LUFA.php)

### **NOTICE**
If you use this software you must accept the License, most importantly you should be aware of the Disclaimer of Warranty and Limitation of Liability. Additionally, using this firmware will likely void any warranty you have with the original equipment manufacturer. The license can be viewed [here](https://github.com/bxzn/Muffin/blob/main/LICENSE), and [here](https://www.gnu.org/licenses/gpl-3.0.en.html).

## Where Can I Download It?

Muffin is still in development, when its ready I'll create a release and make downloads available.

## Planned Features

The overall intention for the project is to remove some of the limitations of the original firmware, such as fixed midi channels, however there is the possibility of adding new features.
See below regarding feature requests.

See [Features](Features.MD) for more information.

## Feature Requests and Bug Reports

Please use the [issues](https://github.com/bxzn/MuffinTwister/issues) page of the repository to request new features, or to submit a bug.

## Why Muffin?

- To add new features to the MFT, and fix some of the original firmwares' curiosities.
- To provide a clean and well organised firmware project that other users can extend for themselves.
- As a personal project to experiment with the AVR platform.

## Getting Involved

I have created a virtual machine development environment that can be used to quickly get going with the project.

It is available [here](https://github.com/bxzn/AVRGCC_DevelopmentEnvironment), its a generic Ubuntu VM configured with the latest AVR-GCC.

It uses a [Vagrant](www.vagrantup.com) configuration script so you don't have to configure anything.

### Line Endings

Make sure to use 'nix line endings (LF) in all files.
Alternatively run [scripts/ConvertLineEndings.sh](scripts/ConvertLineEndings.sh) from the root folder in a terminal.

## Building the Project

The vagrant environment should drop your terminal directly into the checkout folder. Then just run a few scripts to get started:

1. Set environment variables (the period before the script is important :)
2. Generate build files (only needs to be executed once)
3. Compile all build-targets

```bash
$ . ./scripts/setEnv.sh
$ ./scripts/mesonGenerate.sh
$ ./scripts/build.sh
```

Now make some modifications and just run the `build.sh` script when required.

If you want a clean build run `build.sh clean`.

## Disclaimer

[Please review the license prior to use](https://github.com/bxzn/Muffin/blob/main/LICENSE)
