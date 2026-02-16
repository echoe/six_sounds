# Two days of vibecoding an FM synth ...

I wanted something that was like the Opsix, and like Six Sines, but was:

-Much simpler than the Opsix VST
-Had filters, like the Opsix, and not like Six Sines. Especially comb filters, they're great, but I also like Low-Pass Filters.
-Works on Linux natively!

Because of that, I eventually got this synth out of the AI, with a lot of testing and checking through the process. I also (despite my best efforts) did learn a little bit about how JUCE functions.

Notes:

The sound routes sequentially down. The volume sliders at the end of each mod matrix row will let that oscillator be heard without any filtering, though, if you want. In this way it can both process sequentially and in parallel.

The context knobs don't re-title themselves right now. They are:
Sine: -C1 is Phase. There are no other options.
LoPass: C1 is Cutoff, and C2 is Resonance. TRK controls the amount of key tracking.
Comb: C1 is Feedback, and C2 is delay direction (fastforward or back). TRK controls the amount of key tracking.

Disclaimers:
-This is the result of two days of doing this (it's actually the first time I have vibecoded anything for any reason).
-I have only tested this on Linux really (I havent even test installed it on windows yet), it's just much easier to build a thousand times and check everything there since I wrote a helper script to automatically install the VST into my .vst3 folder.
-I am not sure that everything works as shown. But it /appears/ to. Or it did at one point.

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

Open your six_sounds folder in VS Code.
Remove the parts of the CMake file that call out Linux binaries specifically (lines 6 to 8).
Select your compiler you installed, then just click build.
