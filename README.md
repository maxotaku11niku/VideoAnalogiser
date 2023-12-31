# VideoAnalogiser
**Command Line Utility for Analogising Digital Videos**

# About the program
**VideoAnalogiser** is a command line application using libav\* that makes videos look analoguey by simulating the actual signal. It is essentially an extension to [AnalogueConvertEffect](https://github.com/maxotaku11niku/AnalogueConvertEffect), and probably quite overkill but it does work very well. All 3 colour formats (PAL, NTSC and SECAM) are supported, and pretty much all signal formats are supported as well (though in practice, there is little difference between most of them). You can also simulate VHS, but tape-specific artifacts are not yet supported. But in any case you can experience the nostalgia in **moving images!**

The source code for the particular version of the FFmpeg libraries (libav\*) I used is available here: (https://github.com/FFmpeg/FFmpeg/commit/eacfcbae69)

# Getting started

Invoke the program with `videoanalogiser`. Invoking without arguments or with `-h` will show you some basic help and remind you of the options available to you. To get started, use `videoanalogiser [input filename] [output filename] -csys [pal/ntsc/secam] <-vhs> -br [bitrate in kb/s]` as a simple starting command. `-preview` is useful for seeing what your chosen options will do before committing to a long encoding process.

# Building
Clone this repository and invoke the makefile with `make` to build a standard executable for the platform you're compiling on. Currently the new build process is only known to work on Linux, and you'll need the following libraries installed on your system with their dev tools:

- libavcodec
- libavformat
- libavutil
- libswscale
- libswresample

Other make commands can be used to do other things.

- `make default` : Redundant, just use `make`. Builds for the current platform, and assumes a generic processor (i.e. no extensions).
- `make native` : Builds for the specific platform and processor, including extensions. Use this to build a nice faster executable for yourself.
- `make debug` : Builds for the current platform, making an executable ideal for debugging.
- `make clean` : Cleans the output directories, use this before switching to another build option.
- `make install-linux` : Copies the output executable to `/usr/local/bin/`, requiring root permissions.
