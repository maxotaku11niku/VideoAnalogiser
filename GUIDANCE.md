# Guidance

VideoAnalogiser has a few settings, and it may not be immediately obvious what they do. Since even `-preview` can take a while, it's probably useful to have some idea of what these settings all do.

## `-h`

Displays a help dialog that reminds you of these commands, then quits.

## `-bsys [system]`

Determines the broadcast standard to use, which is how you specify the resolution and framerate of the video. Defaults to M for NTSC, I for PAL and L for SECAM. Valid values are `m`, `n`, `b`, `g`, `h`, `i`, `d`, `k`, `l`, `vhs525`, `vhs625`.

## `-bsyshelp [system]`

Displays parameters of the given broadcast system, then quits the program.

## `-csys [system]`

Determines the colour system to use, which can dramatically affect the output image. This program supports all the 3 major formats that were used in the era of analogue TV:

- `pal`: [PAL](https://en.wikipedia.org/wiki/PAL) (Phase Alternating Line), Developed in Europe, uses the [YUV](https://en.wikipedia.org/wiki/YUV) colourspace and [quadrature amplitude modulation](https://en.wikipedia.org/wiki/Quadrature_amplitude_modulation) for the chroma subcarrier. Its defining feature is the alternation of the subcarrier phase with each scanline, which cancels out hue shifts associated with global phase errors.
- `ntsc`: [NTSC](https://en.wikipedia.org/wiki/NTSC) (National Television System Committee), Developed in America, uses 4 the [YIQ](https://en.wikipedia.org/wiki/YIQ) colourspace and [quadrature amplitude modulation](https://en.wikipedia.org/wiki/Quadrature_amplitude_modulation) for the chroma subcarrier. Its defining feature is the fact that the Q (violet-green) component is transmitted at a lower bandwidth than the I (blue-orange) component.
- `secam`:[SECAM](https://en.wikipedia.org/wiki/SECAM) (SÉquentiel de Couleur À Mémoire),  Developed in France, uses the [YDbDr](https://en.wikipedia.org/wiki/YDbDr) colourspace and [frequency modulation](https://en.wikipedia.org/wiki/Frequency_modulation) for the chroma subcarrier. Its defining feature is the use of FM for the subcarrier, but this feature has proven very difficult for me to decode easily. Results from this format may not be very good.

This setting defaults to PAL.

## `-vhs`

This is the 'quick VHS' option. This switches the broadcast standard to VHS, with the number of lines determined by the colour system used (`vhs525` is used for NTSC, `vhs625` is used for PAL and SECAM).

## `-preview`

Generates 300 frames of video, rather than the full length. This is intended as a quick preview though it may still take a couple of minutes to make the preview.

## `-noise [amount]`

Adds white noise to the signal, with `[amount]` controlling its magnitude. This will require you to up the bitrate if you take it too far, so be careful. Recommended values 0.0 - 0.5. Defaults to 0.0.

## `-jitter [amount]`

Simulates noise in the scanning mechanism. Upping this value will cause the video to become shaky and wobbly. Recommended values 0.0 - 0.01. Defaults to 0.0.

## `-reso [amount]`

Affects how sharp the frequency cutoffs are in the decode filters. High values will cause ringing artifacts to appear. Recommended values 2.0 - 20.0. Defaults to 5.0.

## `-prefreq [amount]`

Affects the width of the encoding prefilters. This is a multiplier to the usual decode filter width as specified by the broadcast standard. This can be used to simulate poor quality recording equipment and to manage crosstalk artifacts. Recommended values 0.1 - 1.0. Defaults to 0.7.

## `-psnoise [amount]`

Simulates noise in the phase of the subcarrier decoder, over each scanline. Upping this value will cause multicoloured streaks to appear. This won't affect the colours of SECAM. Recommended values 0.0 - 3.0. Defaults to 0.0.

## `-crosstalk [amount]`

Affects how much the chroma and luma information mix. Upping this value will strengthen the dot patterns on the screen and cause colours to shift. Recommended values 0.0 - 1.0. Defaults to 0.0.

## `-pfnoise [amount]`

Simulates noise in the phase of the subcarrier decoder, over each field. Upping this value will cause flashing colours to appear. This won't affect the colours of SECAM, and its effect will be more subtle in PAL. Recommended values 0.0 - 3.0. Defaults to 0.0.