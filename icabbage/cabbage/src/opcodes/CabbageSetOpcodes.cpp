//
//  CabbageMIDIOpcode.cpp
//  Cabbage
//
//  Created by Rory on 09/02/2023.
//


#include "CabbageSetOpcodes.h"
#include "../CabbageParser.h"

//=====================================================================================
// cabbageSetValue "channel", xValue
//=====================================================================================
int CabbageSetValue::setValue(int /*pass*/)
{
    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);

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
        data.value = args[1];
        data.type = CabbageOpcodeData::MessageType::Value;
        varData->enqueue(data);
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
    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);
    
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
            data.cabbageCode+=("(\""+std::string(args.str_data(3).data)+"\")");
            varData->enqueue(data);
        }
    }
    varData->enqueue(data);
}

//=====================================================================================
// cabbageSet "channel", "identifier("arg")
// cabbageSet "channel", "identifier", "arg"
//=====================================================================================
int CabbageSetInitString::setIdentifier(int pass)
{

    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);

    if(in_count() > 2)
    {
        auto data = getIdentData(csound, args, true, 0, 1);
        data.type = CabbageOpcodeData::MessageType::Identifier;
        if(containsIllegalChars(args.str_data(2).data))
        {
            _log(args.str_data(2).data);
            csound->message("Cabbage Warning: Ill-formatted arguments passed to channel:\""+data.channel+"\" Check for brackets within strings..");
            return IS_OK;
        }
        
        data.cabbageCode+=("(\""+std::string(args.str_data(2).data)+"\")");
        
        varData->enqueue(data);
    }
    else
    {
        auto data = getIdentData(csound, args, true, 0, 1);
        data.type = CabbageOpcodeData::MessageType::Identifier;
        varData->enqueue(data);
    }
    
    
    return IS_OK;
}

//=====================================================================================
// cabbageSet kTrig, "channel", "identifier", kArg1, kArg2, kArg3, etc..
//=====================================================================================
int CabbageSetPerfMYFLT::setIdentifier(int /*pass*/)
{
    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);
    
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
            std::string params;
            for( int i = 2 ; i < in_count() ; i++)
            {
                params += std::to_string(args[i]) + (i<in_count()-1 ? "," : "");
            }
            data.cabbageCode+=("("+params+")");
            varData->enqueue(data);
        }
        else
            csound->init_error("Not enough input arguments\n");
    }
    varData->enqueue(data);
}

//=====================================================================================
// cabbageSet "channel", "identifier", iArg1, iArg2, iArg3, etc..
//=====================================================================================
int CabbageSetInitMYFLT::setIdentifier(int /*pass*/)
{

    od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>* varData = CabbageOpcodes::getOpcodeDataGlobalvariable(csound, od);
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
        data.cabbageCode+=("("+params+")");
        varData->enqueue(data);
    }
    else
        csound->init_error("Not enough input arguments\n");
    
    
    return IS_OK;
}
