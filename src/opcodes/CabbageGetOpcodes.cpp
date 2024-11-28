/*
 * Copyright (c) 2024 Rory Walsh
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include <sstream>
#include "Cabbage.h"
#include "CabbageGetOpcodes.h"
#include "CabbageParser.h"
#include "CabbageUtils.h"

//=====================================================================================
// cabbageDump "channel" [, iIndent]
int CabbageDump::dump(int)
{
    auto* hostData = static_cast<cabbage::Engine*>(csound->host_data());
    const std::string widgetChannel = args.str_data(0).data;
    int indents = in_count()==2 ? args[1] : 4;
    for(auto &widget : hostData->getWidgets())
    {
        auto channel = cabbage::Parser::removeQuotes(widget["channel"].get<std::string>());
        if(channel == widgetChannel)
        {
            csound->message(widget.dump(indents));
        }
    }    
}

// cabbageDump kTrig, "channel" [, iIndent]
int CabbageDumpWithTrigger::dump(int)
{
    auto* hostData = static_cast<cabbage::Engine*>(csound->host_data());
    const std::string widgetChannel = args.str_data(1).data;
    int indents = in_count()==2 ? args[2] : 4;
    //trigger printing
    if(args[0] == 1)
    {
        for(auto &widget : hostData->getWidgets())
        {
            auto channel = cabbage::Parser::removeQuotes(widget["channel"].get<std::string>());
            if(channel == widgetChannel)
            {
                csound->message(widget.dump(indents));
            }
        }
    }
}


//=====================================================================================
// k1 cabbageGetValue "channel"
// i1 cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValue::getValue(int init)
{
//    std::cout << " Opcode called on thread: " << std::this_thread::get_id() << std::endl;
    if(in_count() == 0)
        return NOTOK;
    
    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void**)&value, inargs.str_data(0).data,
                                            CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        outargs[0] = *value;

    }
    
    return IS_OK;
}

//=====================================================================================
// k1, kTrig cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValueWithTrigger::getValue(int mode)
{
    if(in_count() == 0)
        return NOTOK;
    
    if(in_count() > 1)
        triggerOnPerfPass = inargs[1];

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void**)&value, inargs.str_data(0).data,
                                            CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        numberOfPasses = (numberOfPasses < 3 ? numberOfPasses+1 : 3);

        if(*value != currentValue)
        {
            currentValue = *value;
            outargs[1] = 1;
            outargs[0] = currentValue;
        }
        else
        {
            if(numberOfPasses == 2 && triggerOnPerfPass>0)//test first k-pass
            {
                outargs[1] = 1;
            }
            else
                outargs[1] = 0;
        }
    }
    
    return IS_OK;
}

//=====================================================================================
// Not yet implemented...
// SOut cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValueString::getValue(int rate)
{    
    csound->init_error("Not yet implemented as none of the current widgets support \"channelType\":\"string\" \n");
    return NOTOK;
    
    if(in_count() == 0)
            return NOTOK;

        if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void**)&value, inargs.str_data(0).data,
                                                CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
        {
            if (!currentString)
            {
                currentString = csound->strdup((((STRINGDAT*)value)->data));
            }

            if (strcmp(currentString, ((STRINGDAT*)value)->data) != 0)
            {
                currentString = csound->strdup(((STRINGDAT*)value)->data);
            }

            if (rate == CabbageOpcodeData::PassType::Init)
            {
                outargs.str_data(0).size = ((STRINGDAT*)value)->size;
                outargs.str_data(0).data = (((STRINGDAT*)value)->data);
            }
            else //seems I need to use csound->strdup at k-time...
            {
                outargs.str_data(0).size = int(strlen(currentString)) + 1;
                outargs.str_data(0).data = currentString;
            }
        }
        
        
        return IS_OK;
}

//=====================================================================================
// Not yet implemented..
// SOut, kTrig cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValueStringWithTrigger::getValue(int rate)
{
    csound->init_error("Not yet implemented as none of the current widgets support \"channelType\":\"string\" \n");
    return NOTOK;
    
    if(in_count() == 0)
        return NOTOK;
        
    int trigOnInit = 0;

    if(in_count() == 2)
        trigOnInit = inargs[1];

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void**)&value, inargs.str_data(0).data,
                                            CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        const auto s = csound->strdup(inargs.str_data(0).data);
        if(!currentString){
            currentString = csound->strdup((((STRINGDAT*)value)->data));
        }
        
        if(strcmp(currentString, ((STRINGDAT*)value)->data) != 0)
        {
            
            currentString = csound->strdup(((STRINGDAT*)value)->data);
            outargs[1] = 1;
        }
        else
        {
            if (trigOnInit && rate!=CabbageOpcodeData::PassType::Init)
                outargs[1] = 1;
            else
                outargs[1] = 0;
        }
        
        outargs.str_data(0).size = int(strlen(currentString))+1;
        outargs.str_data(0).data = currentString;
    }


    return IS_OK;
}

//=====================================================================================
// iWidth cabbageGet "channel", "bounds.width"
// kVisible cabbageGet "channel", "visible"
//=====================================================================================
int CabbageGetMYFLT::getIdentifier(int init)
{
    
    auto* hostData = static_cast<cabbage::Engine*>(csound->host_data());
    
    if(in_count()==2)
    {
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);
        for(auto &widget : hostData->getWidgets())
        {
            auto channel = cabbage::Parser::removeQuotes(widget["channel"].get<std::string>());
            if(channel == data.channel)
            {
                //EvaluateJavaScript << data.identifierText << ":" << widget[data.identifierText].get<float>();
                outargs[0] = getJsonValue(widget, data.identifier).get<MYFLT>();
            }
        }
    }
    else if (in_count()==1)
    {
        //if only a channel string is passed in, then get the current value of that channel
        //this is basically the same as the chnget opcode
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 0);
        for(auto &widget : hostData->getWidgets())
        {
            auto channel = cabbage::Parser::removeQuotes(widget["channel"].get<std::string>());
            if(channel == data.channel)
            {
                if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void**)&value, inargs.str_data(0).data,
                                                        CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
                {
                    outargs[0] = *value;
                }
            }
        }
    }
    
    return IS_OK;
}

//=========================================================================================
// SText cabbageGet "channel", "identifier"
// SText cabbageGet "channel", "identifier.key"
//=========================================================================================
int CabbageGetString::getIdentifier(int init)
{
    auto* hostData = static_cast<cabbage::Engine*>(csound->host_data());
    
    if(in_count()==2) // irate version
    {
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);
        for(auto &widget : hostData->getWidgets())   
        {
            auto channel = cabbage::Parser::removeQuotes(widget["channel"].get<std::string>());
            if(channel == data.channel)
            {
                auto output = getJsonValue(widget, data.identifier).get<std::string>();
                outargs.str_data(0).size = int(strlen(output.c_str())+1);
                outargs.str_data(0).data = csound->strdup(output.data());
            }
        }
    }
    
    return IS_OK;
}

//=========================================================================================
//
//=========================================================================================
int CabbageGetStringWithTrigger::getIdentifier(int init)
{
    auto* hostData = static_cast<cabbage::Engine*>(csound->host_data());
    
    if(in_count()==2)
    {
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);
        for(auto &widget : hostData->getWidgets())
        {
            auto channel = widget["channel"].get<std::string>();
            if(channel == data.channel)
            {
                auto str = getJsonValue(widget, data.identifier).get<std::string>();

                if(currentString != str){
                    outargs[1] = 1;
                    currentString = str;
                }
                else
                    outargs[1] = 0;
                
                outargs.str_data(0).size = int(strlen(str.c_str())+1);
                outargs.str_data(0).data = csound->strdup(str.data());
            }
        }
    }
    
    return IS_OK;
}
