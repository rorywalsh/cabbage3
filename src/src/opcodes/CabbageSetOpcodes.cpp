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
int CabbageSetValue::setValue(int /*pass*/)
{
    auto* hostData = static_cast<Cabbage*>(csound->host_data());

    if(csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, args.str_data(0).data,
                                                   CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        *value = args[1];
    }
 
    //this needs to be throttled as sending some many messages
    //to the UI will choke it. Only update the widget's value
    //every 32 k-cycles.
    if(kCycles==32)
    {
        CabbageOpcodeData data = getValueIdentData(args, true, 0, 1);
        data.cabbageJson["value"] = args[1];
        data.type = CabbageOpcodeData::MessageType::Value;
        hostData->opcodeData.enqueue(data);
        kCycles = 0;
    }
    kCycles++;
    
    return IS_OK;
}

//=====================================================================================
// cabbageSet kTrig, "channel", "identifier("arg")
// cabbageSet kTrig, "channel", "identifier", "arg"
//=====================================================================================
int CabbageSetPerfString::setIdentifier(int /*pass*/)
{
    auto* hostData = static_cast<Cabbage*>(csound->host_data());

    
    auto data = getIdentData(csound, args, true, 1, 2);
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
            data.cabbageJson+=(":"+std::string(args.str_data(3).data)+"\"");
            cabAssert(false, "not implemented yet..");
            hostData->opcodeData.enqueue(data);
        }
    }
    hostData->opcodeData.enqueue(data);
    
    return IS_OK;
}

//=====================================================================================
// cabbageSet "channel", "identifier("arg")
// cabbageSet "channel", "identifier", "arg"
//=====================================================================================
int CabbageSetInitString::setIdentifier(int pass)
{

    auto* hostData = static_cast<Cabbage*>(csound->host_data());
    
    if(in_count() > 2)
    {
        auto data = getIdentData(csound, args, true, 0, 1);
        data.type = CabbageOpcodeData::MessageType::Identifier;
        if(containsIllegalChars(args.str_data(2).data))
        {
            csound->message("Cabbage Warning: Ill-formatted arguments passed to channel:\""+data.channel+"\" Check for brackets within strings..");
            return IS_OK;
        }
        
        data.cabbageJson+=("(\""+std::string(args.str_data(2).data)+"\")");
        
        hostData->opcodeData.enqueue(data);
    }
    else
    {
        auto data = getIdentData(csound, args, true, 0, 1);
        data.type = CabbageOpcodeData::MessageType::Identifier;
        hostData->opcodeData.enqueue(data);
    }
    
    
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
            
            //need to push to new vector before assigning to JSON object
            std::vector<MYFLT> values;
            for( int i = 3 ; i < in_count() ; i++)
            {
                values.push_back(args[i]);
            }
            
            data.cabbageJson[identifier] = values;
            hostData->opcodeData.enqueue(data);
            return IS_OK;
        }
        else
            csound->init_error("Not enough input arguments\n");
    }
}

//=====================================================================================
// cabbageSet "channel", "identifier", iArg1, iArg2, iArg3, etc..
//=====================================================================================
int CabbageSetInitMYFLT::setIdentifier(int /*pass*/)
{
    auto* hostData = static_cast<Cabbage*>(csound->host_data());
    auto data = getIdentData(csound, args, true, 0, 1);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    
    if(in_count() > 2)
    {
        data.type = CabbageOpcodeData::MessageType::Identifier;
        std::string params;
        for( int i = 2 ; i < in_count() ; i++)
        {
            params += std::to_string(args[i]) + (i<in_count()-1 ? "," : "");
        }
        data.cabbageJson+=("("+params+")");
        hostData->opcodeData.enqueue(data);
    }
    else
        csound->init_error("Not enough input arguments\n");
    
    
    return IS_OK;
}