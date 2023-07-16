# VideoAnalogiser
**Command Line Utility for Analogising Digital Videos**

# About the program
**VideoAnalogiser** is a command line application using libav\* that makes videos look analoguey by simulating the actual signal. It is essentially an extension to AnalogueConvertEffect, and probably quite overkill but it does work very well. All 3 colour formats (PAL, NTSC and SECAM) are supported, and pretty much all signal formats are supported as well (though in practice, there is little difference between most of them). You can also simulate VHS, but tape-specific artifacts are not yet supported. But in any case you can experience the nostalgia in **moving images!**

The source code for the particular version of the FFmpeg libraries (libav\*) I used is available here: (https://github.com/FFmpeg/FFmpeg/commit/eacfcbae69)

# Getting started

Invoke the program with `VideoAnalogiser`. Invoking without arguments or with `-h` will show you some basic help and remind you of the options available to you. To get started, use `VideoAnalogiser [input filename] [output filename] -csys [pal/ntsc/secam] <-vhs> -br [bitrate in kb/s]` as a simple starting command. `-preview` is useful for seeing what your chosen options will do before committing to a long encoding process.

# Building
This is a CMake project, so in theory all you would need to do is just git clone and everything works?