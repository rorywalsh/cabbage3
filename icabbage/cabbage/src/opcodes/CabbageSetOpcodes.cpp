//
//  CabbageMIDIOpcode.cpp
//  Cabbage
//
//  Created by Rory on 09/02/2023.
//

#include "Cabbage.h"
#include "CabbageSetOpcodes.h"
#include "../CabbageParser.h"


//=====================================================================================
// cabbageSetValue "channel", xValue
//=====================================================================================
int CabbageSetValue::setValue(int pass)
{
    auto* hostData = static_cast<Cabbage*>(csound->host_data());

    //csound->get_csound()->csoundSetControlChannel(args.str_data(0).data, args[1]);
 
    //this needs to be throttled as sending some many messages
    //to the UI will choke it. Only update the widget's value
    //every 32 k-cycles.
    if(kCycles==32)
    {
        CabbageOpcodeData data = getValueIdentData(args, true, 0, 1);
        data.cabbageJson["value"] = *value;
        data.type = CabbageOpcodeData::MessageType::Value;
        hostData->opcodeData.enqueue(data);
        kCycles = 0;
    }
    kCycles++;
    
    return IS_OK;
}

//=====================================================================================
// cabbageSet kTrig, "channel", "identifier("arg")
// cabbageSet kTrig, "channel", "identifier", "Sarg"
//=====================================================================================
int CabbageSetPerfString::setIdentifier(int /*pass*/)
{
    auto* hostData = static_cast<Cabbage*>(csound->host_data());
    const int argIndex = 2;
    auto data = getIdentData(csound, args, true, 1, argIndex);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    
    if(!testForValidNumberOfInputs(in_count(), argIndex+1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }
    
    int trigger = int(args[0]);
    
    if(trigger == 0)
    {
        return IS_OK;
    }
    else
    {
        if(in_count() - argIndex == 1)
            updateWidgetJson<std::string>(data.cabbageJson, args, argIndex, in_count(),data.identifier);
        else
            updateWidgetJson<std::string>(data.cabbageJson, args, argIndex+1, in_count(),data.identifier);
        hostData->opcodeData.enqueue(data);
    }
    
    
    return IS_OK;
}

//=====================================================================================
// cabbageSet "channel", "identifier("arg")
// cabbageSet "channel", "identifier", "arg"
//=====================================================================================
int CabbageSetInitString::setIdentifier(int pass)
{

    auto* hostData = static_cast<Cabbage*>(csound->host_data());
    const int argIndex = 1;
    auto data = getIdentData(csound, args, true, 0, argIndex);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    
    if(!testForValidNumberOfInputs(in_count(), 2))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }
    
    if(in_count() == 3)
        updateWidgetJson<std::string>(data.cabbageJson, args, argIndex+1, in_count(), data.identifier);
    else
        updateWidgetJson<std::string>(data.cabbageJson, args, argIndex+1, in_count(), data.identifier);
    
    hostData->opcodeData.enqueue(data);
    
    return IS_OK;
}

//=====================================================================================
// cabbageSet kTrig, "channel", "identifier", kArg1, kArg2, kArg3, etc..
//=====================================================================================
int CabbageSetPerfMYFLT::setIdentifier(int /*pass*/)
{
    auto* hostData = static_cast<Cabbage*>(csound->host_data());
    auto data = getIdentData(csound, args, true, 1, 2);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    
    const int argIndex = 2;
    
    if(!testForValidNumberOfInputs(in_count(), argIndex+1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }
    
    const int trigger = int(args[0]);
    
    if(trigger == 0)
    {
        return IS_OK;
    }
    else
    {
        updateWidgetJson<MYFLT>(data.cabbageJson, args, argIndex+1, in_count(),data.identifier);
        hostData->opcodeData.enqueue(data);
    }
    
    return IS_OK;

}

//=====================================================================================
// cabbageSet "channel", "identifier", iArg1, iArg2, iArg3, etc..
//=====================================================================================
int CabbageSetInitMYFLT::setIdentifier(int /*pass*/)
{
    auto* hostData = static_cast<Cabbage*>(csound->host_data());
    auto data = getIdentData(csound, args, true, 0, 1);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    const int argIndex = 1;
    
    if(!testForValidNumberOfInputs(in_count(), argIndex+1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }
    
    if(in_count() == 3)
        updateWidgetJson<MYFLT>(data.cabbageJson, args, argIndex+1, in_count(), data.identifier);
    else
        updateWidgetJson<MYFLT>(data.cabbageJson, args, argIndex+1, in_count(), data.identifier);
    
    hostData->opcodeData.enqueue(data);
    
    return IS_OK;
}
