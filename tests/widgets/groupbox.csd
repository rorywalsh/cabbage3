<Cabbage>[
    {"type":"form","caption":"Label Example","size":{"width":580,"height":500},"pluginId":"def1"},
    {"type":"groupbox","channel":"groupbox1","bounds":{"left":10,"top":10,"width":500,"height":480}, "text":"I'm a groupbox"}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d -+rtmidi=NULL -M0 -m0d --midi-key=4 --midi-velocity-amp=5
</CsOptions>e
<CsInstruments>
; Initialize the global variables. 
ksmps = 32
nchnls = 2
0dbfs = 1


; Rory Walsh 2021 
;
; License: CC0 1.0 Universal
; You can copy, modify, and distribute this file, 
; even for commercial purposes, all without asking permission. 

instr 1

endin


</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
i1 0 z
</CsScore>
</CsoundSynthesizer>
