//
//  CabbageMIDIOpcode.cpp
//  Cabbage
//
//  Created by Rory on 09/02/2023.
//


#include "CabbageSetOpcodes.h"



int CabbageSetValue::init()
{
    std::cout << "cabbageSetValue\n";
    return 0;
}

int CabbageSetValue::kperf()
{
    vt = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getGlobalvariable(csound, vt);
    //this is k-rate only so set init back to true in order to get the correct channel name on each k-cycle
    CabbageOpcodeData data = getValueIdentData(args, true, 0, 1);
    data.value = args[1];
    data.type = CabbageOpcodeData::MessageType::Value;
    varData->enqueue(data);
        
    
    return 0;
}
