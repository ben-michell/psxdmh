# Doom Music Quirks and Curiosities

## "Danny Won, Hey"

Song 100 (D09 - Deimos Anomaly) is entirely constructed from a slowed-down sound
sample of someone saying "Danny Won, Hey". To hear this, extract the raw patch
as follows:

```
psxdmh -s 44100 patch 107 <path_to_data_files> danny.wav
```

The Danny referred to may be Danny Lewis (aka Technoman), who was credited as
the artist for the "Club Doom" song.


## Playback Rate Limitation

The PlayStation SPU has a limitation of how far it can raise the pitch of a
sound, which is by a factor of four. From the natural playback rate of 44,100 Hz
this means the limit is 176,400 Hz.

Some songs contain notes which try to use a playback rate higher than this. When
this happens the SPU will limit the rate to its maximum, meaning we don't hear
the song the way it was defined. By default psxdmh respects this limit, however
this can be bypassed with the `-u` option.

Songs that breach this limit can be found with the `dump-song` action. Times
where the song is exceeding the SPU limit are flagged with an asterisk, and
following the dump of the music events is a list of the playback rates used by
each track, along with a warning message when the SPU limit was exceeded.

To hear an example of this, song 95 has an obvious section near the start (from
0:15 to 0:22). Compare the two versions extracted as follows:

```
psxdmh song 95 <path_to_data_files> 95-normal.wav
psxdmh -u song 95 <path_to_data_files> 95-unlimited.wav
```

The full list of songs that try to exceed this limit are:

##### 92 (D03 - Toxin Refinery)
- Track 0: Slight difference from 3:38 to 3:41.

##### 93 (D04 - Command Control)
- Track 1: Distinct difference from 7:26 to 7:33.

##### 95 (D06 - Central Processing)
- Track 1: Distinct difference from 0:15 to 0:22, 1:06 to 1:19, and 1:40 to
1:42.

##### 98 (D10 - Containment Area)
- Track 1: Slight difference at 1:15 and 3:59.
- Track 2: Distinct difference from 3:42 to 3:49.

##### 108 (D13 - Command Center)
- Track 0: Distinct difference from 0:00 to 0:11, and slight difference from
3:51 to 3:56.

##### 110 (F05 - Catwalk)
- Track 0: Slight difference from 10:20 to 10:27.

##### 112 (F01 - Attack)
- Track 0: Slight difference from 7:14 to 7:19.

##### 114 (F07 - Geryon)
- Track 1: No audible difference.

##### 115 (F10 - Paradox)
- Track 1: No audible difference.

Note that the times listed are for the songs in their natural form, that is,
without any silence processing (the `-i`, `-o`, and `-g` options).

The question remains as to whether these songs were meant to sound the way they
were played on the PlayStation, or whether they were meant to use the unlimited
playback rates.


## Short Tracks

A few songs contain a low-pitch rumbly background noise that stops shortly after
the beginning of the song, never to restart. Comparing these songs to the
versions released by Aubrey Hodges as the
"[Doom Playstation: Official Soundtrack](https://aubreyhodges.bandcamp.com/album/doom-playstation-official-soundtrack)"
(which was recorded from a developer version of the PlayStation) has these
background noises continuing throughout the songs.

The songs affected are:

##### 94: D05 - Phobos Lab
PlayStation stops after 0:28, soundtrack version plays throughout.

##### 95: D06 - Central Processing
PlayStation stops after 0:10, soundtrack version plays throughout.

##### 98: D10 - Containment Area
PlayStation stops after 0:10, soundtrack version plays throughout.

In all cases there is no obvious fault in the music data. All the short tracks
repeat at the same timestamp as the other tracks in the songs, which rules out
timing issues such as a different tempo.

It does seem the short tracks were meant to continue playing though. For
example, in the case of song 95 the prolonged periods of silence wouldn't be
silent if the short track had continued to play.


## Non-Repeating Music

All music repeats with the exception of songs 117 (F08 - Minos) and 118 (F02 -
Virgil) from _Final Doom_. Perhaps it is no coincidence that these are the only
two songs where the tracks within each song are different lengths.

All other music repeats in its entirety. That is, no music contains a
non-repeated intro.


## Mono Songs

While most songs have some degree of stereo, there are some that are entirely
mono. That is, all sounds are played evenly balanced between the left and right
channels. These are:
- 96: D07 - Computer Station
- 97: D08 - Phobos Anomaly
- 98: D10 - Containment Area
- 99: D12 - Deimos Lab
- 100: D09 - Deimos Anomaly
- 103: D22 - Limbo
- 104: D11 - Refinery
- 106: D18 - Pandemonium
- 107: D20 - Unholy Cathedral
- 110: F05 - Catwalk
- 114: F07 - Geryon
- 119: F04 - Combine

Any stereo effect you can hear in these songs is purely down to the reverb.


## Single-Use Songs

All the music from _Doom_ is used on more than one level. In contrast all the
new songs added to _Final Doom_ are used only once with the exception of song
111 (F09 - Nessus). These single-use songs are:
- 110: F05 - Catwalk
- 112: F01 - Attack
- 113: F03 - Canyon
- 114: F07 - Geryon
- 115: F10 - Paradox
- 116: F06 - Fistula
- 117: F08 - Minos
- 118: F02 - Virgil
- 119: F04 - Combine


## Clipped Patches

Sounds played by the PlayStation SPU are encoded with ADPCM. When decompressing
these sounds it is possible for the decoded audio to try to exceed the maximum
amplitude allowed, which results in clipping. While this would normally cause
distortion, there's only one sound in _Doom_ or _Final Doom_ that triggers this
clipping and that's patch 11 (BFG9000 Explosion). A little extra distortion
won't make any difference here...
