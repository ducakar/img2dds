img2dds
-------

Tool for conversion of images to DDS format + script designed for Kerbal Space Program to convert
all textures inside `GameData/` to DDS format.

### Installation ###

1. Install Pyhton (it works fine with both 2.x and 3.x).
2. Unzip this to your KSP directory, so that `dds.py` and `img2dds/` are directly inside your KSP
   directory.

### Usage ###

Double-click `dds.py` or run in from console (type `./dds.py` on Linux) and wait until it finishes.

**Warning: original textures are deleted after conversion.**

Alternatively, you can also run `./dds.py /path/to/GameData` in case you haven't unzipped this
archive into your (current) KSP directory, or if you have multiple KSP installations.

If `dds.py` script doesn't convert everything correctly (i.e. doesn't generate mipmaps for some
model, converts some PNGs it shouldn't etc.) you should open it and add exceptions to the arrays
at the beginning of the script. I only added exceptions and did fine-tuning for the most common mods
and the mods I have installed.

### Sources ###

https://github.com/ducakar/img2dds
