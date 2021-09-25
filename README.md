# PlayStation Doom Music Hack

psxdmh is a command-line tool to extract music and sound effects from the
PlayStation versions of _Doom_ and _Final Doom_. While psxdmh strives for
authenticity in the extraction process, it also allows for some enhancement to
improve the quality of the audio.

The PlayStation versions of the _Doom_ games had brilliant background music, and
I was sorely disappointed when I couldn't find it on the game CDs. After many,
many hours analyzing the data files I figured out that the music was generated
dynamically by a MIDI-style player from sound samples and song definitions. The
difficult part was that it used an unknown custom format.

Gradually I figured out how the data was organized within the files, and began
writing psxdmh to help with the analysis. The next stage in its evolution was to
add the ability to play the music. This took a lot of work before it generated
high-quality output, and along the way I learned far more about audio processing
than I ever imagined I would.


## Getting Started

**IMPORTANT: psxdmh requires data files from the original PlayStation _Doom_ or
_Final Doom_ game CDs to work. _Final Doom_ is preferable as it contains the
data for all 30 songs, whereas _Doom_ only has the data for the first 20
songs.**

Projects are provided to build psxdmh for macOS (Xcode 11) in the `xcode`
directory, and for Windows (Visual Studio 2019) in the `visual_studio`
directory. Building for a Unix-style system should be fairly easy using the
macOS support as a base.

Note that debug builds of psxdmh run _much_ slower than the release versions, so
make sure to use a release build for any serious music extraction.


## Command-Line Usage

### Data Files

As mentioned above, psxdmh needs data files from the original PlayStation game
CDs to work. The data files required are:
- The WMD file. On the game CDs this is `PSXDOOM/MUSIC/DOOMSND.WMD`.
- All the LCD files. These are split across the `PSXDOOM/MUSIC`,
`PSXDOOM/SNDMAPS1`, `PSXDOOM/SNDMAPS2`, and `PSXDOOM/SNDMAPS3` directories on
the game CDs.

There are a lot of LCD files, and they contain a lot of duplication. psxdmh is
able to condense them into a single LCD file with the `pack-data` action. This
makes it much easier to work with the data, and I would recommend this as your
first step. For example, to create a minimal LCD file named `psxdmh.lcd`:

```
psxdmh pack-data <path_to_game_cd>/PSXDOOM psxdmh.lcd
```

For ease of use place the resulting LCD file together with the WMD file in a
directory. This path can then be supplied to other psxdmh actions.

### Extracting Songs

The _Doom_ CD contains data for 110 songs numbered 0 - 109, and _Final Doom_ has
an extra 10 taking the last song up to number 119. The first 90 songs are
actually sound effects. This command will extract these in WAV format, and the
files will be automatically named to describe each sound effect:

```
psxdmh song 0-89 <path_to_data_files>
```

To extract all the music (assuming _Final Doom_), with the files automatically
named according to the level where the music is first used:

```
psxdmh song 90-119 <path_to_data_files>
```

There are many options that affect how the music is processed when it is
extracted. For example, the volume can be raised to use the full range available
(normalized):

```
psxdmh -n song 90 <path_to_data_files>
```

The reverb effect and volume can be manually specified:

```
psxdmh -r studio-large -R -6 song 90 <path_to_data_files>
```

To extract the music at highest quality I use the options below which do the
following:
- Normalizes then slightly reduces the volume.
- Adjusts the length of silence before and after each song.
- Limits the length of silent periods within the songs.
- Uses a wide sinc resampling window to give the best audio quality when
changing the pitch of sounds.
- Sets the reverb effect to `studio-large` at -6 dB.
- Slightly expands the stereo effect.
- Applies fixes for faults in the original patches (clicks and pops).

```
psxdmh -n -v -0.5 -i 0.1 -o 3.0 -g 3.0 -w 17 -r studio-large -R -6 -x 0.2 -P song 90-119 <path_to_data_files>
```

The full range of available options are described below.

### Extracting Tracks

Each song is made up of one to three tracks playing together, where each track
uses one instrument. For example, one track may be playing an organ and another
drums. psxdmh allows the extraction of individual tracks from songs.

> **Tip:** To see how many tracks a song has use the `dump-song` action
described below.

Track extraction is similar to song extraction. For example, to extract the
organ sound (track 0) from song 90:

```
psxdmh track 90 0 <path_to_data_files> 90-organ.wav
```

Or to extract just the drums, which are in track 2 (first sound at 0:14):

```
psxdmh track 90 2 <path_to_data_files> 90-drums.wav
```

### Extracting Patches

Songs are constructed from the playing of instruments, and instruments are
constructed from a basic sound or patch. Both songs and patches are numbered in
similar ranges, however don't confuse them as there is no direct correspondence
between the numbers.

Patches with IDs of 0 to 85 are used for sound effects. Patches after this (up
to 121 in _Doom_ or 143 in _Final Doom_) are used as instruments in the songs.

For example, to extract all patches used for sound effects:

```
psxdmh patch 0-85 <path_to_data_files>
```

To extract the patch used for the organ instrument in song 90:

```
psxdmh patch 87 <path_to_data_files> organ.wav
```

> Tip: To work out which patches are used as instrument in a song use the
`dump-song` action described below.

Some patches (such as those used for sound effects) don't repeat, while many
used as instruments in the music do. Repeating patches can be played any number
of times when they are extracted. This is the only audio-related option that
affects patch extraction:

```
psxdmh patch -p 10 87 <path_to_data_files> organ-10.wav
```

### Dumping Data Files

psxdmh can display details about the contents of WMD files (instruments and
songs), LCD files (patches), and detailed information about individual songs.

The dump-related actions take either a directory where data files can be found
or a data file itself. For example, to dump a WMD file either of the following
commands can be used:

```
psxdmh dump-wmd <path_to_data_files>
psxdmh dump-wmd <wmd_file>
```

Dumping an LCD file is similar. Note that supplying a path will merge the
contents of all LCD files found and dump the combined result, while supplying a
file name dumps just that one file:

```
psxdmh dump-lcd <path_to_data_files>
psxdmh dump-lcd <lcd_file>
```

Dumping a song shows the individual tracks making up the song, the instruments
used on each track, the patches used by the instruments, and all the actual
music events during the song such as notes on and off and pitch bending:

```
psxdmh dump-song 97 <path_to_data_files>
```

### Options

##### Volume Adjustment Options
- `-v <dB>`, `--volume=<dB>` Set the amplification of the output in dB (default
0). This can be combined with the `-n` option in which case this volume
adjustment occurs after the normalization.
- `-n`, `--normalize` Normalize the level of the audio to use the full range.
This option writes the audio to a temporary file, and requires approximately
twice the space of the completed WAV file.

##### Playback Options
- `-r <preset>`, `--reverb-preset=<preset>` Set which reverb effect to use
(default `auto`). Valid values are `studio-small`, `studio-medium`,
`studio-large`, `half-echo`, `space-echo`, `hall`, `room`, `off`, and `auto`.
Selecting `off` disables the effect. Selecting `auto` will set the reverb preset
and volume to the values used by the game level where the song first appears.
- `-R <dB>`, `--reverb-volume=<dB>` Set the volume of the reverb effect in dB
(default -6). This option has no effect if the reverb preset is set to `off` or
`auto`.
- `-p <count>`, `--play-count=<count>` Set the number of times a repeating song,
track, or patch is played (default 1).

##### Silence Adjustment Options
- `-i <time>`, `--intro=<time>` Enforce a silent period of exactly the given
time at the start of a song (default off). This will add or remove silence as
required to give the specified amount.
- `-o <time>`, `--outro=<time>` Enforce a silent period of exactly the given
time at the end of a song (default off). This will add or remove silence as
required to give the specified amount.
- `-g <time>`, `--maximum-gap=<time>` Limit the length of silent periods within
songs or tracks to the given time (default off). Some songs, such as song 95,
contain excessively long silences. This option can be used to reduce these gaps
to a more reasonable length.

##### Audio Repair and Adjustment Options
- `-x <width>`, `--stereo-expansion=<width>` Adjust the width of the stereo
effect (default 0.0). A value of -1.0 reduces the audio to near mono, 0.0 leaves
it unchanged, and 1.0 pushes any uncentred sound to the far left or far right.
- `-P`, `--repair-patches` Repair patches with major audio faults such as
clicks, pops, and excessive noise where possible. Songs 94, 97, 98, 102, 106,
113, and 114 all use patches that are repaired by this option.
- `-u`, `--unlimited` Real PlayStation hardware has a limit to how much it can
raise the pitch of sounds. Several songs, such as song 95, contain notes that
try to exceed this limit. Setting this option removes the limit.

##### Output Options
- `-s <rate>`, `--sample-rate=<rate>` Set the output sample rate (default 44100
for songs and tracks, 11025 for patches).
- `-h <frequency>`, `--high-pass=<frequency>` Attenuate frequencies lower than
the given frequency in the output (default 30). A value of 0 disables the
filter.
- `-l <frequency>`, `--low-pass=<frequency>` Attenuate frequencies higher than
the given frequency in the output (default 15000). A value of 0 disables the
filter.
- `-w <size>`, `--sinc-window=<size>` Set the size of the sinc resampling window
(default 7). This controls the audio quality when the pitch of a sound is
changed. A value of 7 gives high-quality results. Higher values give slightly
better results at the expense of more processing time. A value of 3 gives
satisfactory results for most songs and is faster, though some songs will
contain audible artifacts.

##### Miscellaneous Options
- `-Q`, `--quiet` Display only errors.
- `-V`, `--verbose` Display extended information.
- `--version` Display version and license information.
- `--help` Display help text.


## Further Information

A guide to how the psxdmh source code is arranged and how it works can be found
in [doc/source.md](doc/source.md).

Details of how the various songs are used in the _Doom_ and _Final Doom_ games
refer to [doc/music.md](doc/music.md).

For some quirks and curiosities relating to the music take a look at
[doc/curiosities.md](doc/curiosities.md).

The home page for psxdmh is
[https://www.muryan.com/psxdmh](https://www.muryan.com/psxdmh).


## Credits

Thanks to the following people:
- [Aubrey Hodges](http://www.aubreyhodges.com/) for composing the music!
- [Nocash](https://problemkaputt.de/psx-spx.htm) for the description of the
PSX SPU algorithms for ADPCM decoding, the ADSR envelope, and reverb.
- Robert Bristow-Johnson for the Butterworth filter coefficient formulae in
"[Cookbook formulae for audio EQ biquad filter coefficients](https://webaudio.github.io/Audio-EQ-Cookbook/Audio-EQ-Cookbook.txt)".
- Elbryan42 for the recordings of the PSX Doom music. These were invaluable when
comparing the results of psxdmh to the real thing.


## License

The source code for psxdmh is released under the GPL v3. See
[LICENSE.txt](LICENSE.txt) for details.
