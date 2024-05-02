//
//  CabbageMIDIOpcode.cpp
//  Cabbage
//
//  Created by Rory on 09/02/2023.
//


#include "CabbageSetOpcodes.h"
#include "../CabbageParser.h"


int CabbageSetValue::setAttribute(int /*pass*/)
{

    vt = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getGlobalvariable(csound, vt);
    //this is k-rate only so set init back to true in order to get the correct channel name on each k-cycle
    CabbageOpcodeData data = getValueIdentData(args, true, 0, 1);
    data.value = args[1];
    data.type = CabbageOpcodeData::MessageType::Value;
    varData->enqueue(data);
    
    return IS_OK;
}

int CabbageSetIdentifier::setAttribute(int /*pass*/)
{

    vt = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getGlobalvariable(csound, vt);
    //this is k-rate only so set init back to true in order to get the correct channel name on each k-cycle
    CabbageOpcodeData data = getIdentData(args, true, 1, 2);

    int trigger = int(args[0]);
    
    if(trigger == 0)
        return IS_OK;
    else
    {
        if(in_count() == 3)
        {
            //if only three inputs, it's in the form cabbageSet kTrig, "channel", "bounds(10, 10, 60, 60)"
            //so parse the string and assign tokens..
            data.type = CabbageOpcodeData::MessageType::Identifier;
        }
        else
        {
            for ( int i = 3 ; i < in_count(); i++)
            {
                //data..args.append(args[i]);
            }
        }
    }
    
    
    varData->enqueue(data);
    
    return IS_OK;
}
