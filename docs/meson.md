# Meson

## Setup
```
$ meson setup --cross-file ./crossfiles/avr/xmega/128a4u.cross build
```

## Compile

```
$ meson compile -C build
```

## How to build external projects using a custom meson.build

1. Add a wrap file, either git or file.
2. Create a packagefiles directory for the project.
3. Create a meson.build flie in this directory, and set it up like any normal project, but export a dependency or library.

## How to "inject" custom files into a sub-project.

A 'patch_directory' variable can set in a wrap file for a given dependency.
The patch directories are located in 'subprojects/packagefiles/...'

For example, tinyusb requires a configuration header with some defines for the target platform (tusb_config.h). This must be provided during compilation.

1. Create a new directory in packagesfiles:
```
    subprojects/packagefiles/tinyusb/
```

2. Add the patch_directory definition to the meson.build for this wrap:

```
[wrap-git]
url = https://github.com/nic-starke/tinyusb.git
patch_directory = tinyusb
..
```
3. Now add the subdirectories that you want to inject into the dependency. In the case of tinyusb we want to add a header to its /src directory, so we create the following directory:

```
subprojects/packagefiles/tinyusb/src
```

4. Now add any files that need to be added to the tinyusb source:
```
subprojects/packagefiles/tinyusb/src/tusb_config.h
```

Now apply packagefiles to your subprojects, from the terminal run:

```
$ meson subprojects packagefiles --apply
```

The directories and files will be copied from packagefiles to the dependency.
