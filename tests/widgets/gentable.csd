<Cabbage>[
    {"type":"form","caption":"Gentable Example","size":{"width":400,"height":650},"guiMode":"queue","pluginId":"def1"},
    {"type":"genTable","channel":"gentable1","bounds":{"left":10,"top":7,"width":380,"height":200},"colour":{"background":"#c9c9c9","fill":"#d20000","stroke":{"colour":"#dddddd","width":1}}},
    {"type":"horizontalSlider","channel":"harm1","bounds":{"left":14,"top":212,"width":368,"height":22},"text":"Harm1","value":0,"colour":{"fill":"#cf0202","stroke":{"colour":"#525252","width":2},"tracker":{"background":"#ffffff","fill":"#93d200"}},"decimalPlaces":2,"range":{"min":0,"max":10,"defaultValue":0,"skew":1,"increment":0.01}},
    {"type":"horizontalSlider","bounds":{"left":14,"top":244,"width":368,"height":14},"channel":"harm2","range":{"min":0,"max":1,"value":0,"skew":1,"increment":0.01},"text":"Harm2"},
    {"type":"horizontalSlider","bounds":{"left":14,"top":276,"width":368,"height":14},"channel":"harm3","range":{"min":0,"max":1,"value":0,"skew":1,"increment":0.01},"text":"Harm3"},
    {"type":"horizontalSlider","bounds":{"left":14,"top":308,"width":368,"height":14},"channel":"harm4","range":{"min":0,"max":1,"value":0,"skew":1,"increment":0.01},"text":"Harm4"},
    {"type":"horizontalSlider","bounds":{"left":14,"top":340,"width":368,"height":14},"channel":"harm5","range":{"min":0,"max":1,"value":0,"skew":1,"increment":0.01},"text":"Harm5"},
    {"type":"checkBox","bounds":{"left":16,"top":380,"width":120,"height":20},"channel":"normal","text":"Normalise","value":1},
    {"type":"checkBox","channel":"fill","bounds":{"left":140,"top":380,"width":120,"height":20},"text":"Fill Table","value":1,"colour":{"off":{"fill":"#00000000","stroke":"#000000"},"on":{"fill":"#ff0c0c","stroke":"#000000"}}}
]</Cabbage>
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
; License: CC0 1.0 Universal
; You can copy, modify, and distribute this file, 
; even for commercial purposes, all without asking permission. 
giTable	ftgen	1, 0,   1024, 10, 1


//fill table with default values
schedule("UpdateTable", 0, 0, 1, 0, 0, 0, 0, 0)

instr	1
   
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
    giTable	ftgen	1, 0,   1024, 10*iNormal, p4, p5, p6, p7, p8
    cabbageSet	"gentable1", "tableNumber", 1	; update table display
endin

</CsInstruments>
<CsScore>
f1 0 1024 10 1
i1 0 [3600*24*7]
</CsScore>
</CsoundSynthesizer>
