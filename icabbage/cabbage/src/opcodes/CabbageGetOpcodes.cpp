//
//  CabbageMIDIOpcode.cpp
//  Cabbage
//
//  Created by Rory on 09/02/2023.
//

#include <sstream>
#include "CabbageGetOpcodes.h"
#include "CabbageParser.h"
#include "CabbageUtils.h"

int CabbageGetValue::getValue(int init)
{
    if(in_count() == 0)
        return NOTOK;
    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, inargs.str_data(0).data,
                                            CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        outargs[0] = *value;
    }
    
    return IS_OK;
}


int CabbageGetValueWithTrigger::getValue(int mode)
{
    if(in_count() == 0)
        return NOTOK;
    
    if(in_count() > 1)
        triggerOnPerfPass = inargs[1];

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, inargs.str_data(0).data,
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

int CabbageGetValueString::getValue(int rate)
{
    if(in_count() == 0)
            return NOTOK;

        if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, inargs.str_data(0).data,
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

int CabbageGetValueStringWithTrigger::getValue(int rate)
{
    if(in_count() == 0)
        return NOTOK;
        
    int trigOnInit = 0;

    if(in_count() == 2)
        trigOnInit = inargs[1];

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, inargs.str_data(0).data,
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

int CabbageGetMYFLT::getIdentifier(int init)
{
    wd = (std::vector<nlohmann::json>**)csound->query_global_variable("cabbageWidgetData");
    std::vector<nlohmann::json>* varData = CabbageOpcodes::getWidgetDataGlobalvariable(csound, wd);
    
    if(in_count()==2)
    {
        CabbageOpcodeData data = getIdentData(inargs, true, 0, 1);
        for(auto &widget : *varData)
        {
            auto channel = CabbageParser::removeQuotes(widget["channel"].get<std::string>());
            if(channel == data.channel)
            {
                //EvaluateJavaScript << data.identifierText << ":" << widget[data.identifierText].get<float>();
                outargs[0] = widget[data.cabbageCode].get<float>();
            }
        }
    }
    else
    {
        //if only a channel string is passed in, then get the current value of that channel
        
        CabbageOpcodeData data = getIdentData(inargs, true, 0, 0);
        for(auto &widget : *varData)
        {
            auto channel = CabbageParser::removeQuotes(widget["channel"].get<std::string>());
            if(channel == data.channel)
            {
                if (csound->get_csound()->GetChannelPtr(csound->get_csound(), &value, inargs.str_data(0).data,
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
int CabbageGetString::getIdentifier(int init)
{
    wd = (std::vector<nlohmann::json>**)csound->query_global_variable("cabbageWidgetData");
    std::vector<nlohmann::json>* varData = CabbageOpcodes::getWidgetDataGlobalvariable(csound, wd);
    
    if(in_count()==2) // irate version
    {
        CabbageOpcodeData data = getIdentData(inargs, true, 0, 1);
        for(auto &widget : *varData)
        {
            auto channel = CabbageParser::removeQuotes(widget["channel"].get<std::string>());
            if(channel == data.channel)
            {
                auto str = CabbageParser::removeQuotes(widget[data.cabbageCode].get<std::string>());
                outargs.str_data(0).size = int(strlen(str.c_str())+1);
                outargs.str_data(0).data = csound->strdup(str.data());
            }
                
        }
        

    }
    
    return IS_OK;
}

int CabbageGetStringWithTrigger::getIdentifier(int init)
{
    wd = (std::vector<nlohmann::json>**)csound->query_global_variable("cabbageWidgetData");
    std::vector<nlohmann::json>* varData = CabbageOpcodes::getWidgetDataGlobalvariable(csound, wd);
    
    if(in_count()==2)
    {
        CabbageOpcodeData data = getIdentData(inargs, true, 0, 1);
        for(auto &widget : *varData)
        {
            auto channel = widget["channel"].get<std::string>();
            if(channel == data.channel)
            {
                std::cout << widget.dump(4);
                auto str = widget[data.cabbageCode].get<std::string>();

                if(!currentString){
                    currentString = (char*)str.c_str();
                    outargs[1] = 0;
                }
                else if(currentString != str){
                    outargs[1] = 1;
                    currentString = (char*)str.c_str();
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
