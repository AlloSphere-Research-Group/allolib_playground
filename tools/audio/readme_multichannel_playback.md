# Multichannel file player

This application plays back multiple audio files mapped to speakers
according to a configuration file.

The configuration file is called "multichannel_playback.toml" by default,
but the name can also be passed as the first command line argument.

The configuration file must provide a "rootDir" which is the path where the
audio files are located. Then you must provide any number of file elements
specifying an audio file name, the output channels (that must match the number
of channels in the audio file) and the gain:

```
rootDir = "files/"
[[file]]
name = "test.wav"
outChannels = [0, 1]
gain = 0.9
[[file]]
name = "test_mono.wav"
outChannels = [1]
gain = 1.2
```

You can also have a file loop by adding ```loop=true```.
