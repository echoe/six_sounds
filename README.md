# One day of vibecoding an FM synth ...

I wanted something that was like the Opsix, and like Six Sines, but was:

-Much simpler than the Opsix VST

-Had filters, like the Opsix, and not like Six Sines

-Worked on Linux

Because of that, I eventually got this synth out of the AI, with a lot of testing and checking through the process.

Notes:

The sound routes sequentially down. The volume sliders at the end of each mod matrix row will let that oscillator be heard without any filtering, though, if you want. In this way it can both process sequentially and in parallel.

Disclaimer:
This is the result of one day of doing this (it's actually the first time I have vibecoded anything for any reason). I purposefully have made the envelopes very simple - there's just a single one. I really don't use envelopes in a complicated way too much. There also is no modulation (... this is the top priority, if I attempt to make this for a whole second day).

I have only tested this on Linux really (I havent even test installed it on windows yet).

To install:
-It is a vst3 synth. drop it wherever your vst3 folder is.

To build:

Linux:
Make sure you have all required things to run JUCE on your machine; put the JUCE folder from a JUCE download in the same level as this folder; and then just run ./build.sh .

MacOS:
Download and add the JUCE folder as above, and install cmake:

brew install cmake

Remove the parts of the CMake file that call out Linux binaries specifically.
Then simply run the build.sh script.

Windows:
Download and add the JUCE folder as above. I decided to use VS Code, so install that, and then download and install the build tools for Visual Studio https://visualstudio.microsoft.com/downloads/ .

Open your six_sounds folder in VS Code. Select your compiler you installed, then just click build :)
