# Guide to the psxdmh Source Code

## High-Level Organization

psxdmh is essentially a MIDI-style music player. The raw sound samples are read
from LCD files. The music and instruments are defined in a WMD file.

psxdmh uses the concept of audio modules to play the music (see
[module.h](../src/module.h)). This is a simple idea, but provides great
flexibility since it allows modules to be chained together in arbitrary
arrangements. For example, the output from a song player can be passed through a
reverb module, then high-pass and low-pass modules.

Much of the code makes use of templates to allow the audio modules to operate on
either mono or stereo audio data.


## Conventions

The psxdmh source code uses the `psxdmh` namespace to isolate all of its types.
Preprocessor symbols are almost all distinguished by the `PSXDMH_` prefix and
use upper case. The naming style used for everything else is snake case: all
lower case with words separated by underscores.

psxdmh requires a compiler with support for C++11, and comes with support for
macOS and Windows. Recommended compilers are Xcode 11 and Visual Studio 2019 or
later.


## A Tour Through the Source Code

### App Group

_Files in this group relate to the overall operation of psxdmh, processing the
command line and orchestrating the various actions._

##### `psxdmh.cpp`
The main application. This handles the command line and calls to other modules
to perform the requested actions.

##### `global.h`
Common includes and definitions used by every `.cpp` file. This includes
determining which platform the app is being built for.

##### `extract_audio.h`, `extract_audio.cpp`
Handle the `song`, `track`, and `patch` actions. This involves building up a
graph of audio modules and writing their output to a WAV file.

##### `options.h`, `options.cpp`
Definition and parsing of the command line options supported by psxdmh.

##### `version.h`
Version numbers and related information for psxdmh.

### Audio Group

_Files in this group are all general-purpose audio processing modules._

##### `filter.h`
Audio module implementing
[Butterworth](https://en.wikipedia.org/wiki/Butterworth_filter)
[IIR](https://en.wikipedia.org/wiki/Infinite_impulse_response) low-pass and
high-pass filters.

##### `module.h`
Base class for all audio modules. This is templated to allow support for both
mono and stereo audio.

##### `normalizer.h`
Audio module that adjusts the level of the audio to use the full range
available. This requires temporarily storing the entire audio being processed,
which is achieved by writing it to a temporary file.

##### `resampler.h`, `resampler.cpp`
Audio modules that resample audio to arbitrary frequencies. This includes a
high-performance
[Lanczos windowed sinc filter](https://en.wikipedia.org/wiki/Lanczos_resampling)
which is used to change the pitch of sounds. The original SPU hardware used
4-tap Gaussian interpolation which is less complex but lower quality, and
limited how much the pitch could be raised. These files also include a simple
linear filter used for envelope resampling.

##### `sample.h`
Audio sample types for mono and stereo data. The `stereo_t` type provides
essentially the same behaviour as `mono_t` to make it easy for audio modules to
work with either type of data.

##### `silencer.h`
Audio module that can adjust the length of silent periods at the start, within,
or at the end of a song.

##### `splitter.h`
Audio module that splits an incoming audio stream into multiple streams. This is
used by the reverb module to split off a separate stream where the reverb effect
is calculated before being mixed back into the original audio.

##### `statistics.h`
Audio module that collects some simple statistics about the audio data passing
through it, and provides progress information via a callback.

##### `volume.h`
Audio module that adjusts the audio by a fixed amount.

##### `wav_file.h`
WAV file writer. This takes an audio module and writes its output to the file.

### Player Group

_Files in this group interpret the original data files and implement the music
player logic._

##### `lcd_file.h`, `lcd_file.cpp`
Parser for LCD format data files. These contain the patches (raw sound samples)
used by songs and sound effects.

##### `music_stream.h`, `music_stream.cpp`
Parser for the MIDI-style music events used in WMD song tracks.

##### `song_player.h`, `song_player.cpp`
Audio module that manages the playback of a song defined in the WMD file. Each
song is made up of one or more tracks.

##### `track_player.h`, `track_player.cpp`
Audio module that manages the playback of a single track of a song defined in
the WMD file.

##### `wmd_file.h`, `wmd_file.cpp`
Parser for WMD format data files. These files contain the definition of songs in
a MIDI-style format, plus the definition of the instruments used by the songs.

### SPU Group

_Files in this group emulate the SPU (Sound Processing Unit) hardware in the
PlayStation._

##### `adpcm.h`, `adpcm.cpp`
Audio module that decodes the
[ADPCM-encoded](https://en.wikipedia.org/wiki/Adaptive_differential_pulse-code_modulation)
sound data from LCD files. All sounds played by the SPU are encoded as ADPCM.
This module emulates the behaviour of the SPU as accurately as possible.

##### `channel.h`, `channel.cpp`
Audio module that emulates a single channel of the PSX SPU. A channel plays a
sound (from ADPCM-encoded data) with the volume controlled by an ADSR envelope.
The real SPU has a limit of 24 channels, while psxdmh allows an unlimited
number.

##### `envelope.h`, `envelope.cpp`
Audio module emulating the PSX SPU
[ADSR envelope](https://en.wikipedia.org/wiki/Envelope_(music)) generator. This
emulation is faithful to the original hardware with the exception that it
performs some simple smoothing of the volume changes where possible.

##### `reverb.h`, `reverb.cpp`
Audio module emulating the PSX SPU reverb effect. This emulation is very close
to the original hardware with the exceptions that the audio data is held as
floating point instead of integer, and no clipping is performed.

### Utility Group

_Files in this group are general-purpose classes and functions._

##### `command_line.h`, `command_line.cpp`
Generic command line parsing, including long and short names for options, and
options with  or without values.

##### `endian.h`
Detection of the endianness of the target platform, and functions to byte-swap
into little-endian order.

##### `enum_dir.h`, `enum_dir.cpp`
Enumeration of the contents of directories.

##### `message.h`, `message.cpp`
Simple message output manager.

##### `safe_file.h`, `safe_file.cpp`
Simple file reading and writing class with full error checking.

##### `utility.h`, `utility.cpp`
Miscellaneous utility classes and functions.
