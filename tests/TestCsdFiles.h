#pragma once
#include <string>

namespace TestCsdFiles {

const std::string basicOscillator = R"(
<Cabbage>[
{"type": "form", "caption": "Dummy Example", "size": {"width": 360.0, "height": 460.0}, "guiMode": "queue", "pluginId": "def1"}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
    aout oscili 0.1, 440
    outs aout, aout
endin
</CsInstruments>
<CsScore>
i 1 0 10
</CsScore>
</CsoundSynthesizer>
)";

const std::string rotarySliders = R"(
<Cabbage>[
{"type": "form", "caption": "Slider Example", "size": {"width": 360.0, "height": 460.0}, "guiMode": "queue", "pluginId": "def1"},
{"type": "rotarySlider", "bounds": {"left": 20.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channel": "harmonic1", "range": {"min": 0.0, "max": 1.0,  "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}},
{"type": "rotarySlider", "bounds": {"left": 100.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channel": "harmonic2", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}},
{"type": "rotarySlider", "bounds": {"left": 180.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channel": "harmonic3", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}},
{"type": "rotarySlider", "bounds": {"left": 260.0, "top": 20.0, "width": 80.0, "height": 80.0}, "channel": "harmonic4", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}},
{"type": "rotarySlider", "bounds": {"left": 20.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channel": "harmonic5", "range": {"min": 0.0, "max": 1.0, "defaultValue": 0.0, "skew": 1.0, "increment": 0.001}},
{"type": "rotarySlider", "bounds": {"left": 100.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channel": "harmonic6", "range": {"min": 0.0, "max": 1.0,"defaultValue": 0.0, "skew": 1.0, "increment": 0.001}},
{"type": "rotarySlider", "bounds": {"left": 180.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channel": "harmonic7", "range": {"min": 0.0, "max": 1.0,"defaultValue": 0.0, "skew": 1.0, "increment": 0.001}},
{"type": "rotarySlider", "bounds": {"left": 260.0, "top": 100.0, "width": 80.0, "height": 80.0}, "channel": "harmonic8", "range": {"min": 0.0, "max": 1.0,"defaultValue": 0.0, "skew": 1.0, "increment": 0.001}}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
; Initialize the global variables. 
ksmps = 16
nchnls = 2
0dbfs = 1

; Rory Walsh 2021 
;
; License: CC0 1.0 Universal
; You can copy, modify, and distribute this file, 
; even for commercial purposes, all without asking permission. 

instr 1   
    printk2 cabbageGetValue:k("harmonic1")
    printk2 cabbageGetValue:k("harmonic2")
    printk2 cabbageGetValue:k("harmonic3")
    printk2 cabbageGetValue:k("harmonic4")
    printk2 cabbageGetValue:k("harmonic5")
    printk2 cabbageGetValue:k("harmonic6")
    printk2 cabbageGetValue:k("harmonic7")
    printk2 cabbageGetValue:k("harmonic8")   
endin       

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 z
</CsScore>
</CsoundSynthesizer>

)";
} // namespace TestCsdFiles 