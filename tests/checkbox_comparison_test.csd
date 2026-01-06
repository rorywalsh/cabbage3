<Cabbage>
[
    {
        "type": "form",
        "caption": "Checkbox Default Comparison",
        "size": {"width": 500, "height": 250},
        "guiMode": "queue",
        "pluginId": "chkcomp"
    },
    {
        "type": "checkBox",
        "bounds": {"left": 20, "top": 40, "width": 200, "height": 30},
        "channels": [
            {
                "id": "uncheckedBox",
                "event": "valueChanged",
                "range": {"min": 0, "max": 1, "defaultValue": 0, "skew": 1, "increment": 1}
            }
        ],
        "text": "Should be unchecked (defaultValue: 0)",
        "corners": 2
    },
    {
        "type": "checkBox",
        "bounds": {"left": 20, "top": 90, "width": 200, "height": 30},
        "channels": [
            {
                "id": "checkedBox",
                "event": "valueChanged",
                "range": {"min": 0, "max": 1, "defaultValue": 1, "skew": 1, "increment": 1}
            }
        ],
        "text": "Should be checked (defaultValue: 1)",
        "corners": 2
    },
    {
        "type": "label",
        "bounds": {"left": 20, "top": 140, "width": 460, "height": 50},
        "text": "Top checkbox should be unchecked, bottom should be checked.\nWith our fix, checkboxes now respect their defaultValue.",
        "fontSize": 12
    }
]
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
    kUnchecked cabbageGetValue "uncheckedBox"
    kChecked cabbageGetValue "checkedBox"
    printk2 kUnchecked, 10
    printk2 kChecked, 10
endin

</CsInstruments>
<CsScore>
;causes Csound to run for about 7000 years...
f0 z
;starts instrument 1 and runs it for a week
i1 0 [60*60*24*7]
</CsScore>
</CsoundSynthesizer>