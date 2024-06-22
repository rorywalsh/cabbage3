<Cabbage>
form caption("Untitled") size(500, 300), pluginId("def1")
filebutton bounds(10, 230, 100, 20) channel("filebutton1"), text("Load File")
gentable bounds(10, 10, 300, 200), channel("gentable1"), colour("#ffffff"), tableNumber(99), backgroundColour("#0295cf"), outlineWidth(1), outlineColour("#ffffff"), text("Please load a file")
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
   SFile cabbageGetValue "filebutton1"
   printf SFile, changed:k(SFile)
   cabbageSet changed:k(SFile), "gentable1", sprintfk({{file("%s")}}, SFile)
endin

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f99 0 65536 10 1
i1 0 z
;i2 4 1
f0 z
</CsScore>
</CsoundSynthesizer>