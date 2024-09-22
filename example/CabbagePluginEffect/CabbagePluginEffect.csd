<Cabbage>[
    {"type":"form","caption":"Simple Instrument","size":[500,300],"pluginId":"RMSy"},
    {"type":"rotarySlider", "channel":"gain", "bounds":[10, 10, 100, 100], "range":[0, 2, 1, 1, 0.01], "text":"Gain", "trackerColour":[0, 255, 0, 255], "outlineColour":[0, 0, 0, 50], "textColour":[0, 0, 0, 255]}
]</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-dm0 -n -+rtmidi=NULL -M0 --midi-key=4 --midi-velocity=5
</CsOptions>
<CsInstruments>
; sr set by host
ksmps = 16
nchnls = 2
0dbfs = 1


instr 1

endin


</CsInstruments>  
<CsScore>
f99 0 1024 10 1 .1 .1 .1
i1 0 z
</CsScore>
</CsoundSynthesizer>