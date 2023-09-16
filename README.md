# Muffin

*An open source alternative firmware for the excellent Midi Fighter Twister from DJ Tech Tools*

## About

This project was started to add new functionality to the Midi Fighter Twister.
It is based on [AVR-GCC](https://gcc.gnu.org/wiki/avr-gcc) and the excellent [LUFA USB libraries](http://www.fourwalledcubicle.com/LUFA.php)

## Planned Features

The overall intention for the project is to remove some of the limitations of the original firmware, such as fixed midi channels, however there is the possibility of adding new features.
See below regarding feature requests.

## Feature Requests and Bug Reports

Please use the [issues](https://github.com/bxzn/MuffinTwister/issues) page of the repository to request new features, or to submit a bug.

## Why Muffin?

- To add new features to the MFT, and fix some of the original firmwares' curiosities.
- To provide a clean and well organised firmware project that other users can extend for themselves.
- As a personal project to experiment with the AVR platform.

## Getting Involved

### Line Endings

Make sure to use 'nix line endings (LF) in all files.
Alternatively run [scripts/ConvertLineEndings.sh](scripts/ConvertLineEndings.sh) from the root folder in a terminal.

## Usage

1. Set environment variables (the period before the script is important :)
2. Generate build files (only needs to be executed once)
3. Compile all build-targets

```
$ . ./scripts/setEnv.sh
$ ./scripts/mesonGenerate.sh
$ ./scripts/build.sh
```

Now make some modifications and just run the `build.sh` script when required.