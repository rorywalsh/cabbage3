<Cabbage>
form caption("Untitled") size(500, 300), pluginId("def1")
filebutton bounds(12, 216, 119, 34), channel("filebutton1"), text("Load File")
gentable bounds(10, 6, 480, 200), channel("gentable1"), colour("#ffffff"), tableNumber(99), backgroundColour("#000000"), outlineColour("#333333"), text("Please load a file"), fontSize(10), samples()
checkbox bounds(138, 218, 136, 20), channel("autoUpdateTable"), fontSize(10)
</Cabbage>
<CsoundSynthesizer>
<CsOptions>
-n -d -+rtmidi=NULL -M0 -m0d 
</CsOptions> 
<CsInstruments>
; Initialize the global variables. 
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
   SFile, kTriggerLoad cabbageGetValue "filebutton1"
   kAutoUpdate cabbageGetValue "autoUpdateTable"
   cabbageSet kTriggerLoad, "gentable1", sprintfk({{"file":"%s"}},SFile)

    k1 jspline 200, 0.01, 2
    k2 jspline 200, 0.02, 3
    k3 jspline 200, 0.03, 4
    k4 jspline 200, 0.04, 5
    k5 jspline 200, 0.05, 6
    
    if metro(10) == 1 && kAutoUpdate == 1 then
        event "i", "UpdateTable", 0, 0.1, k1, k2, k3, k4, k5
    endif

endin

instr UpdateTable
    iTable	ftgen	99, 0,   2048, 10, p4, p5, p6, p7, p8
    cabbageSet "gentable1", "tableNumber(99)"
endin
</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f99 0 1024 10 1 .1 .1 .1
i1 0 z
;i2 4 1
f0 z
</CsScore>
</CsoundSynthesizer>