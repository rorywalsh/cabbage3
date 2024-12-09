<Cabbage>[
    {"type":"form","caption":"Slider Example","size":{"height":460,"width":360},"guiMode":"queue","pluginId":"def1","colour":{"fill":"#999999"}},
    {"type":"rotarySlider","channel":"harmonic1","bounds":{"left":20,"top":20,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}},
    {"type":"rotarySlider","channel":"harmonic2","bounds":{"left":100,"top":20,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}},
    {"type":"rotarySlider","channel":"harmonic3","bounds":{"left":180,"top":20,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}},
    {"type":"rotarySlider","channel":"harmonic4","bounds":{"left":260,"top":20,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}},
    {"type":"rotarySlider","channel":"harmonic5","bounds":{"left":20,"top":100,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}},
    {"type":"rotarySlider","channel":"harmonic6","bounds":{"left":100,"top":100,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}},
    {"type":"rotarySlider","channel":"harmonic7","bounds":{"left":180,"top":100,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}},
    {"type":"rotarySlider","channel":"harmonic8","bounds":{"left":260,"top":100,"width":80,"height":80},"filmStrip":{"file":"rSlider.png","frames":{"count":64,"height":120,"width":120}},"value":0,"range":{"min":0,"max":1,"defaultValue":0,"skew":1,"increment":0.001}}
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

giWave ftgen 1, 0, 4096, 10, 1, .2, .1, .2, .1

instr 1

    
    a1 oscili tonek(cabbageGetValue:k("harmonic1"), 10), 50, giWave
    a2 oscili tonek(cabbageGetValue:k("harmonic2"), 10), 100, giWave
    a3 oscili tonek(cabbageGetValue:k("harmonic3"), 10), 150, giWave
    a4 oscili tonek(cabbageGetValue:k("harmonic4"), 10), 200, giWave
    a5 oscili tonek(cabbageGetValue:k("harmonic5"), 10), 250, giWave
    a6 oscili tonek(cabbageGetValue:k("harmonic6"), 10), 300, giWave
    a7 oscili tonek(cabbageGetValue:k("harmonic7"), 10), 350, giWave
    a8 oscili tonek(cabbageGetValue:k("harmonic8"), 10), 400, giWave
    
    cabbageSetValue "harmonic1", abs(jspline:k(.9, .1, .3))
    cabbageSetValue "harmonic2", abs(jspline:k(.9, .1, .3))
    cabbageSetValue "harmonic3", abs(jspline:k(.9, .1, .3))
    cabbageSetValue "harmonic4", abs(jspline:k(.9, .1, .3))
    cabbageSetValue "harmonic5", abs(jspline:k(.9, .1, .3))
    cabbageSetValue "harmonic6", abs(jspline:k(.9, .1, .3))
    cabbageSetValue "harmonic7", abs(jspline:k(.9, .1, .3))
    cabbageSetValue "harmonic8", abs(jspline:k(.9, .1, .3))

    
    aMix = a1+a2+a3+a4+a5+a6+a7+a8
    out aMix*.1, aMix*.1
endin       

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 z
</CsScore>
</CsoundSynthesizer>
