<Cabbage>
form caption("Untitled") size(500, 300), guiMode("queue"), pluginId("def1")


rslider bounds(11, 4, 107, 107), channel("freq"), range(0, 1, 0, 1, 0.001), value(0)
rslider bounds(251, 4, 104, 110), channel("amplitude"), range(0, 1, 0, 1, 0.001), value(0), colour("#895d5d")

rslider bounds(10, 160, 110, 110), channel("dummy"), range(0, 1, 0, 1, 0.001), value(0), colour("red")

</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d -+rtmidi=NULL -M0
</CsOptions> 
<CsInstruments>
; Initialize the global variables. 
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
    ; cabbageSetValue "freq", abs(oscil:k(1, .1))
    ; cabbageSet metro(20), "dummy", sprintfk("bounds(%d, %d, 110, 100)", abs(randi:k(200, 1)), abs(randi:k(200, 1)))
    a1 oscili chnget:k("amplitude"), chnget:k("freq")*400
    outs a1, a1
endin

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
i1 0 z
</CsScore>
</CsoundSynthesizer>