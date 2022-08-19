# Spatial Sequencer

The Spatial Sequencer application is designed to read sequences that determine
audio files and their movement through space.

You can run it passing a folder name on the command line.
The GUI will allow playing all synth sequences contained within that folder.

The sequences shoudl have a format like:

```
@ 0 3000.0 AudioObject "TRACK A.wav" "A" 1.0     0 1 0    1 0 0 0   1.0
@ 0 3000.0 AudioObject "TRACK B.wav" "B" 1.0     0 0 -1   1 0 0 0   1.0
@ 0 3000.0 AudioObject "TRACK C.wav" "C" 1.0     -1 0 0   1 0 0 0   1.0
@ 0 3000.0 AudioObject "TRACK D.wav" "D" 1.0     0 0 1    1 0 0 0   1.0
@ 0 3000.0 AudioObject "TRACK E.wav" "E" 1.0     1 0 0    1 0 0 0   1.0
@ 0 3000.0 AudioObject "TRACK F.wav" "F" 1.0     0 1 0    1 0 0 0   1.0

# start end   voice      audiofile   seq  gain    pos      quat     size
```

Each line represents an audio file, with start time and duration. The voice 
type must be "AudioObject" or a voice that inherits from it.
The next field is the filename, followed by the
preset sequence (that determines the position changes, see below). Next is gain
folowed by the starting xyz position, quaternion and size.

The preset sequencer file in the fifth field contains a set of positions 
associated with time:

```
+0:/_pose:0,1,0:0.0
+0.0:/_pose:0,1,0:60.0
+60.0:/_pose:0,0,-1:1.0
+1.0:/_pose:0,-1,0:1.0
+1.0:/_pose:0,0,1:1.0
+1.0:/_pose:0,1,0:1.0
+1.0:/_pose:0,0,-1:2.0
+2.0:/_pose:0,-1,0:2.0
+2.0:/_pose:0,0,1:2.0
+2.0:/_pose:0,1,0:2.0
+2.0:/_pose:0,0,-1:3.0
```

All lines must start with '+' followed by the delta time. The delta time is the
time that must elapse since the execution of the previous line before triggering
the current line. The second component on each line (after the ':') sets a
voice's position (The '_pose' parameter), which can have 3 values or 7 for xyz
plus quaternion. The final value after the last ':' determines the "morph time",
which is the time it will take to get to the new pose. If this value is greater
than the next line's delta time, the morph will be interrupted at its current
value to trigger the next event.
