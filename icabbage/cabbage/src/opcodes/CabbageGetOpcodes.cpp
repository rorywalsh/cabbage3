//
//  CabbageMIDIOpcode.cpp
//  Cabbage
//
//  Created by Rory on 09/02/2023.
//

#include <sstream>
#include "CabbageGetOpcodes.h"
#include "CabbageParser.h"

int CabbageGetValue::getValue(int /*init*/)
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

int CabbageGetMYFLT::getIdentifier(int init)
{
    
    return IS_OK;
}

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
                auto str = CabbageParser::removeQuotes(widget[data.identifierText].get<std::string>());
                outargs.str_data(0).size = int(strlen(str.c_str() )) + 1;
                outargs.str_data(0).data = csound->strdup(str.data());
            }
                
        }
        

    }
    
    return IS_OK;
}
