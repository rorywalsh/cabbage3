<Cabbage>
[
{"type": "form", "caption": "Gentable Example", "size": {"width": 400.0, "height": 650.0}, "guiMode": "queue", "colour": [2.0, 145.0, 209.0], "pluginId": "def1"},
{"type": "gentable", "bounds": {"left": 13.0, "top": 7.0, "width": 370.0, "height": 200.0}, "channel": "gentable1", "outlineThickness": 3.0, "tableNumber": 1.0, "tableGridColour": [155.0, 155.0, 155.0, 255.0], "tableBackgroundColour": [0.0, 0.0, 0.0, 0.0], "0": [147.0, 210.0, 0.0, 255.0]},
{"type": "hslider", "bounds": {"left": 14.0, "top": 212.0, "width": 368.0, "height": 20.0}, "channel": "harm1", "range": {"min": 0.0, "max": 1.0, "value": 1.0, "skew": 1.0, "increment": 0.01}, "textColour": [255.0, 255.0, 255.0, 255.0], "text": "Harm1"},
{"type": "hslider", "bounds": {"left": 14.0, "top": 244.0, "width": 368.0, "height": 20.0}, "channel": "harm2", "range": {"min": 0.0, "max": 1.0, "value": 0.0, "skew": 1.0, "increment": 0.01}, "textColour": [255.0, 255.0, 255.0, 255.0], "text": "Harm2"},
{"type": "hslider", "bounds": {"left": 14.0, "top": 276.0, "width": 368.0, "height": 20.0}, "channel": "harm3", "range": {"min": 0.0, "max": 1.0, "value": 0.0, "skew": 1.0, "increment": 0.01}, "textColour": [255.0, 255.0, 255.0, 255.0], "text": "Harm3"},
{"type": "hslider", "bounds": {"left": 14.0, "top": 308.0, "width": 368.0, "height": 20.0}, "channel": "harm4", "range": {"min": 0.0, "max": 1.0, "value": 0.0, "skew": 1.0, "increment": 0.01}, "textColour": [255.0, 255.0, 255.0, 255.0], "text": "Harm4"},
{"type": "hslider", "bounds": {"left": 14.0, "top": 340.0, "width": 368.0, "height": 20.0}, "channel": "harm5", "range": {"min": 0.0, "max": 1.0, "value": 0.0, "skew": 1.0, "increment": 0.01}, "textColour": [255.0, 255.0, 255.0, 255.0], "text": "Harm5"},
{"type": "checkbox", "bounds": {"left": 16.0, "top": 380.0, "width": 120.0, "height": 20.0}, "channel": "normal", "text": "Normalise", "value": 1.0, "1": [255.0, 255.0, 255.0, 255.0]},
{"type": "checkbox", "bounds": {"left": 140.0, "top": 380.0, "width": 120.0, "height": 20.0}, "channel": "fill", "text": "Fill Table", "value": 1.0, "1": [255.0, 255.0, 255.0, 255.0]},
{"type": "texteditor", "bounds": {"left": 16.0, "top": 410.0, "width": 367.0, "height": 219.0}, "channel": "infoText", "wrap": 1.0, "text": ["A 'GEN' table widget will display the contents of a sound function table. In this example, a basic sine wave is stored in function table 1, which is defined in the CsScore section. Whenever a slider is changed, instr 1 will trigger the 'UpdateTable' instrument, which in turns creates a new function shape accordingly. It writes the shape to the same table number as defined when declaring the gentable widget.", "Although a gentable can be passed new function tables at run-time", "it might incur a slight performance hit especially if the new function table is a larger. It is a better idea is to just copy the contents of one table to another using the table copy opcodes in Csound."]}
]
</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-d -n -m0d
</CsOptions>
<CsInstruments>
;sr is set by the host
ksmps 		= 	32
nchnls 		= 	2
0dbfs		=	1


; Rory Walsh 2021 
;
; License: CC0 1.0 Universal
; You can copy, modify, and distribute this file, 
; even for commercial purposes, all without asking permission. 


instr	1

    SText  = "A 'GEN' table widget will display the contents of a sound function table. In this example, a basic sine wave is stored in function table 1, which is defined in the CsScore section. Whenever a slider is changed, instr 1 will trigger the 'UpdateTable' instrument, which in turns creates a new function shape accordingly. It writes the shape to the same table number as defined when declaring the gentable widget.\n\nAlthough a gentable can be passed new function tables at run-time, it might incur a slight performance hit especially if the new function table is a larger. It is a better idea is to just copy the contents of one table to another using the table copy opcodes in Csound."
    cabbageSet "infoText", "text", SText
    
    ;toggle fill
    kFill, kTrig cabbageGetValue "fill"
    cabbageSet kTrig, "gentable1", "fill", kFill 

    k1 chnget "harm1"
    k2 chnget "harm2"
    k3 chnget "harm3"
    k4 chnget "harm4"
    k5 chnget "harm5"

    aEnv linen 1, 1, p3, 1
    a1 oscili .2, 200, 1
    outs a1, a1

    kChanged changed k1, k2, k3, k4, k5
    if kChanged==1 then
        ;if a slider changes trigger instrument 2 to update table
        event "i", "UpdateTable", 0, .01, k1, k2, k3, k4, k5
    endif

endin

instr UpdateTable
    iNormal = (chnget:i("normal")==0 ? -1 : 1)
    iTable	ftgen	1, 0,   1024, 10*iNormal, p4, p5, p6, p7, p8
    cabbageSet	"gentable1", "tableNumber", 1	; update table display
endin

</CsInstruments>
<CsScore>
f1 0 1024 10 1
i1 0 [3600*24*7]
</CsScore>
</CsoundSynthesizer>