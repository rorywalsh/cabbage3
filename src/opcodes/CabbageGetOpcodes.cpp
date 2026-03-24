/*
 * Copyright (c) 2024 Rory Walsh
 *
 * This file is part of Cabbage3
 *
 * Cabbage3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cabbage3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cabbage3.  If not, see <https://www.gnu.org/licenses/>.
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
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    const std::string widgetChannel = args.str_data(0).data;
    int indents = in_count() == 2 ? args[1] : 4;
    for (auto &widget : hostData->getWidgets())
    {
        if (cabbage::Engine::hasChannel(widget, widgetChannel))
        {
            csound->message(widget.dump(indents));
        }
    }

    return IS_OK;
}

// cabbageDump kTrig, "channel" [, iIndent]
int CabbageDumpWithTrigger::dump(int)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    const std::string widgetChannel = args.str_data(1).data;
    int indents = in_count() == 2 ? args[2] : 4;
    // trigger printing
    if (args[0] == 1)
    {
        for (auto &widget : hostData->getWidgets())
        {
            if (cabbage::Engine::hasChannel(widget, widgetChannel))
            {
                csound->message(widget.dump(indents));
            }
        }
    }
    return IS_OK;
}

//=====================================================================================
// k1 cabbageGetValue "channel"
// i1 cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValue::getValue(int /*init*/)
{
    //    std::cout << " Opcode called on thread: " << std::this_thread::get_id() << std::endl;
    if (in_count() == 0)
        return NOTOK;

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void **)&value, inargs.str_data(0).data,
                                            CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        outargs[0] = *value;
    }

    return IS_OK;
}

//=====================================================================================
// k1, kTrig cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValueWithTrigger::getValue(int /*mode*/)
{
    if (in_count() == 0)
        return NOTOK;

    if (in_count() > 1)
        triggerOnPerfPass = inargs[1];

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void **)&value, inargs.str_data(0).data,
                                            CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        numberOfPasses = (numberOfPasses < 3 ? numberOfPasses + 1 : 3);

        if (*value != currentValue)
        {
            currentValue = *value;
            outargs[1] = 1;
            outargs[0] = currentValue;
        }
        else
        {
            if (numberOfPasses == 2 && triggerOnPerfPass > 0) // test first k-pass
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
// SOut cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValueString::getValue(int rate)
{
    if (in_count() == 0)
        return NOTOK;

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void **)&value, inargs.str_data(0).data,
                                            CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        if (!currentString)
        {
            currentString = csound->strdup((((STRINGDAT *)value)->data));
        }

        if (strcmp(currentString, ((STRINGDAT *)value)->data) != 0)
        {
            currentString = csound->strdup(((STRINGDAT *)value)->data);
        }

        if (rate == CabbageOpcodeData::PassType::Init)
        {
            outargs.str_data(0).size = ((STRINGDAT *)value)->size;
            outargs.str_data(0).data = (((STRINGDAT *)value)->data);
        }
        else // seems I need to use csound->strdup at k-time...
        {
            outargs.str_data(0).size = int(strlen(currentString)) + 1;
            outargs.str_data(0).data = currentString;
        }
    }

    return IS_OK;
}

//=====================================================================================
// SOut, kTrig cabbageGetValue "channel"
//=====================================================================================
int CabbageGetValueStringWithTrigger::getValue(int rate)
{
    if (in_count() == 0)
        return NOTOK;

    int trigOnInit = 0;
    std::string channel = inargs.str_data(0).data;

    if (in_count() == 2)
        trigOnInit = inargs[1];

    if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void **)&value, channel.c_str(),
                                            CSOUND_STRING_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
    {
        if (!currentString)
        {
            currentString = csound->strdup((((STRINGDAT *)value)->data));
        }

        if (strcmp(currentString, ((STRINGDAT *)value)->data) != 0)
        {
            currentString = csound->strdup(((STRINGDAT *)value)->data);
            outargs[1] = 1;
        }
        else
        {
            if (trigOnInit && rate != CabbageOpcodeData::PassType::Init)
                outargs[1] = 1;
            else
                outargs[1] = 0;
        }

        outargs.str_data(0).size = int(strlen(currentString)) + 1;
        outargs.str_data(0).data = currentString;
    }
    else
    {
        return NOTOK;
    }

    return IS_OK;
}

//=====================================================================================
// iWidth cabbageGet "channel", "bounds.width"
// kVisible cabbageGet "channel", "visible"
//=====================================================================================
int CabbageGetMYFLT::getIdentifier(int /*init*/)
{

    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    if (in_count() == 2)
    {
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);
        for (auto &widget : hostData->getWidgets())
        {
            if (cabbage::Engine::hasChannel(widget, data.channel))
            {
                auto val = getJsonValue(widget, data.identifier);
                if (val.is_null())
                {
                    csound->message("cabbageGet: property '" + data.identifier + "' not found on channel '" + data.channel + "'");
                    return NOTOK;
                }
                try { outargs[0] = val.get<MYFLT>(); }
                catch (const nlohmann::json::exception &e)
                {
                    csound->message(std::string("cabbageGet: type error for property '") + data.identifier + "': " + e.what());
                    return NOTOK;
                }
            }
        }
    }
    else if (in_count() == 1)
    {
        // if only a channel string is passed in, then get the current value of that channel
        // this is basically the same as the chnget opcode
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 0);
        for (auto &widget : hostData->getWidgets())
        {
            if (cabbage::Engine::hasChannel(widget, data.channel))
            {
                if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void **)&value, inargs.str_data(0).data,
                                                        CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) ==
                    CSOUND_SUCCESS)
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
int CabbageGetString::getIdentifier(int /*init*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    if (in_count() == 2) // irate version
    {
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);
        for (auto &widget : hostData->getWidgets())
        {
            if (cabbage::Engine::hasChannel(widget, data.channel))
            {
                auto val = getJsonValue(widget, data.identifier);
                if (val.is_null())
                {
                    csound->message("cabbageGet: property '" + data.identifier + "' not found on channel '" + data.channel + "'");
                    return NOTOK;
                }
                try
                {
                    auto output = val.get<std::string>();
                    outargs.str_data(0).size = int(strlen(output.c_str()) + 1);
                    outargs.str_data(0).data = csound->strdup(output.data());
                }
                catch (const nlohmann::json::exception &e)
                {
                    csound->message(std::string("cabbageGet: type error for property '") + data.identifier + "': " + e.what());
                    return NOTOK;
                }
            }
        }
    }

    return IS_OK;
}

//=========================================================================================
// SText[] cabbageGet "channel", "identifier"
//=========================================================================================
int CabbageGetStringArray::getIdentifier(int /*init*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    if (in_count() == 2) // irate version
    {
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);
        for (auto &widget : hostData->getWidgets())
        {
            if (cabbage::Engine::hasChannel(widget, data.channel))
            {
                auto val = getJsonValue(widget, data.identifier);
                if (val.is_null())
                {
                    csound->message("cabbageGet: property '" + data.identifier + "' not found on channel '" + data.channel + "'");
                    return NOTOK;
                }
                std::vector<std::string> items;
                try { items = val.get<std::vector<std::string>>(); }
                catch (const nlohmann::json::exception &e)
                {
                    csound->message(std::string("cabbageGet: type error for property '") + data.identifier + "': " + e.what());
                    return NOTOK;
                }
                csnd::Vector<STRINGDAT>& out = outargs.vector_data<STRINGDAT>(0);
                out.init(csound, static_cast<int>(items.size()), this->insdshead);
                int index = 0;
                for( auto& item : items)
                {
                    out[index].size = static_cast<int>(item.size() + 1); // +1 to include null terminator
                    out[index].data = csound->strdup((char*)item.c_str());
                    index++;
                }
            }
        }
    }

    return IS_OK;
}
//=========================================================================================
//
//=========================================================================================
int CabbageGetStringWithTrigger::getIdentifier(int /*init*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    if (in_count() == 2)
    {
        CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);
        for (auto &widget : hostData->getWidgets())
        {
            if (cabbage::Engine::hasChannel(widget, data.channel))
            {
                auto val = getJsonValue(widget, data.identifier);
                if (val.is_null())
                {
                    csound->message("cabbageGet: property '" + data.identifier + "' not found on channel '" + data.channel + "'");
                    return NOTOK;
                }
                std::string str;
                try { str = val.get<std::string>(); }
                catch (const nlohmann::json::exception &e)
                {
                    csound->message(std::string("cabbageGet: type error for property '") + data.identifier + "': " + e.what());
                    return NOTOK;
                }

                if (currentString != str)
                {
                    outargs[1] = 1;
                    currentString = str;
                }
                else
                    outargs[1] = 0;

                outargs.str_data(0).size = int(strlen(str.c_str()) + 1);
                outargs.str_data(0).data = csound->strdup(str.data());
            }
        }
    }

    return IS_OK;
}

//=========================================================================================
// iHasKey cabbageHasKey "channel", "key"
// kHasKey cabbageHasKey "channel", "key"
// Returns 1 if the named widget has the given property key (dot-notation supported), 0 otherwise.
//=========================================================================================
int CabbageWidgetHasKey::check()
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    outargs[0] = 0;

    if (in_count() != 2)
        return IS_OK;

    CabbageOpcodeData data = getIdentData(csound, inargs, true, 0, 1);

    for (auto &widget : hostData->getWidgets())
    {
        if (cabbage::Engine::hasChannel(widget, data.channel))
        {
            // Traverse dot-notation path to check existence
            auto keys = split(data.identifier, '.');
            const nlohmann::json *current = &widget;
            bool found = true;
            for (const auto &key : keys)
            {
                if (current->contains(key))
                    current = &(*current)[key];
                else
                {
                    found = false;
                    break;
                }
            }
            outargs[0] = found ? 1 : 0;
            return IS_OK;
        }
    }

    return IS_OK;
}
