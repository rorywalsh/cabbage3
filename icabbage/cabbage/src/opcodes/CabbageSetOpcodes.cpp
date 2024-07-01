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

int CabbageSetPerfStrings::setIdentifier(int /*pass*/)
{
    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);
    
    auto data = getIdentData(args, true, 1, 2);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    int trigger = int(args[0]);
    
    if(trigger == 0)
    {
        return IS_OK;
    }
    else
    {
        if(in_count() > 3)
        {
            data.type = CabbageOpcodeData::MessageType::Identifier;
            data.cabbageCode+=("(\""+std::string(args.str_data(2).data)+"\")");
            varData->enqueue(data);
        }
    }
    varData->enqueue(data);
}

int CabbageSetInitStrings::setIdentifier(int /*pass*/)
{

    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);

    if(in_count() > 2)
    {
        auto data = getIdentData(args, true, 0, 1);
        data.type = CabbageOpcodeData::MessageType::Identifier;
        data.cabbageCode+=("(\""+std::string(args.str_data(2).data)+"\")");
        varData->enqueue(data);
    }
    else
    {
        auto data = getIdentData(args, true, 0, 1);
        data.type = CabbageOpcodeData::MessageType::Identifier;
        varData->enqueue(data);
    }
    
    
    return IS_OK;
}
