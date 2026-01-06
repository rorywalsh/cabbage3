<Cabbage>
form caption("Checkbox Test") size(400, 300), guiMode("queue"), pluginId("chkb")
keyboard bounds(8, 158, 381, 95)

checkbox bounds(10, 10, 100, 30) channel("testCheckbox") text("Test Checkbox")
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
    kCheckbox cabbageGetValue "testCheckbox"
    printk2 kCheckbox
endin

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 [60*60*24*7]
</CsScore>
</CsoundSynthesizer>