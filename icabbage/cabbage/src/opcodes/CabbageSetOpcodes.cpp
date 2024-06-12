//
//  CabbageMIDIOpcode.cpp
//  Cabbage
//
//  Created by Rory on 09/02/2023.
//


#include "CabbageSetOpcodes.h"
#include "../CabbageParser.h"


int CabbageSetValue::setValue(int /*pass*/)
{

    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);
    //this is k-rate only so set init back to true in order to get the correct channel name on each k-cycle
    CabbageOpcodeData data = getValueIdentData(args, true, 0, 1);
    data.value = args[1];
    data.type = CabbageOpcodeData::MessageType::Value;
    varData->enqueue(data);
    
    return IS_OK;
}

int CabbageSet::setIdentifier(int /*pass*/)
{

    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);
    //this is k-rate only so set init back to true in order to get the correct channel name on each k-cycle
    if(in_count() > 2)
    {
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
    }
    else
    {
        auto data = getIdentData(args, true, 0, 1);
        data.type = CabbageOpcodeData::MessageType::Identifier;
        std::cout << "identText:" << args.str_data(1).data;
        data.stringData.emplace_back(args.str_data(1).data);
        varData->enqueue(data);
    }
    
    
    
    
    return IS_OK;
}
