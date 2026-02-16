# One day of vibecoding an FM synth ...

I wanted something that was like the Opsix, and like Six Sines, but:
-Much simpler than the Opsix
-Had filters, like the Opsix, and not like Six Sines

Because of that, the sound routes sequentially down. The volume sliders at the end of each mod matrix row will let that oscillator be heard without any filtering, though, if you want. In this way it both processes sequentially and parallel.

This is the result of one day of doing this (it's actually the first time I have vibecoded anything for any reason). I purposefully have made the envelopes very simple - there's just a single one. I really don't use envelopes in a complicated way too much. There also is no modulation (... this is the top priority, if I attempt to make this for a whole second day).

To build:
Make sure you have all required things to run JUCE, put the JUCE folder from a JUCE download in the same level as this folder, and then just run ./build.sh .

This is probably very important: This is only tested to build on Fedora 43, running the VST in Reaper. 
