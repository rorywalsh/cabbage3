/*
 * Copyright (c) 2024 Rory Walsh
 *
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 */

#include "Cabbage.h"
#include "CabbageSetOpcodes.h"
#include "../CabbageParser.h"

//=====================================================================================
// cabbageSetValue "channel", xValue, [kTrig]
//=====================================================================================
int CabbageSetValue::setValue(int /*pass*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());

    int trigger = 1;

    if (in_count() == 3)
        trigger = args[2];

    if (trigger == 1)
    {
        std::string channel = args.str_data(0).data;
        MYFLT newValue = args[1];
        
        if (csound->get_csound()->GetChannelPtr(csound->get_csound(), (void **)&value, channel.c_str(),
                                                CSOUND_CONTROL_CHANNEL | CSOUND_OUTPUT_CHANNEL) == CSOUND_SUCCESS)
        {
            *value = newValue;
        }

        // Always enqueue - let updateChannelCache handle deduplication
        CabbageOpcodeData data = getValueIdentData(args, true, 0, 1);
        data.cabbageJson["value"] = newValue;
        data.type = CabbageOpcodeData::MessageType::Value;
        hostData->updateChannelCache(data);

        kCycles = 0;
    }

    return IS_OK;
}

//=====================================================================================
// cabbageSet kTrig, "channel", "identifier("arg")
// cabbageSet kTrig, "channel", "identifier", "Sarg"
//=====================================================================================
int CabbageSetPerfString::setIdentifier(int /*pass*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    const int argIndex = 2;
    auto data = getIdentData(csound, args, true, 1, argIndex);
    data.type = CabbageOpcodeData::MessageType::Identifier;

    if (!testForValidNumberOfInputs(in_count(), argIndex + 1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }

    int trigger = int(args[0]);

    if (trigger == 0)
    {
        return IS_OK;
    }
    else
    {
        if (in_count() - argIndex == 1)
            updateWidgetJson(data.cabbageJson, args, argIndex, data.identifier, CabbageOpcodeData::ArgType::String);
        else
            updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::String);

        // Only enqueue if value has changed
        if (hostData->isValueDifferent(data))
        {
            hostData->updateChannelCache(data);
        }
    }

    return IS_OK;
}

//=====================================================================================
// cabbageSet "channel", "identifier("arg")
// cabbageSet "channel", "identifier", "arg"
//=====================================================================================
int CabbageSetInitString::setIdentifier(int /*pass*/)
{

    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    const int argIndex = 1;
    auto data = getIdentData(csound, args, true, 0, argIndex);
    data.type = CabbageOpcodeData::MessageType::Identifier;

    if (!testForValidNumberOfInputs(in_count(), 2))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }

    if (in_count() == 3)
        updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::String);
    else
        updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::String);

    hostData->updateChannelCache(data);

    return IS_OK;
}

//=====================================================================================
// cabbageSet kTrig, "channel", "identifier", kArg1
//=====================================================================================
int CabbageSetPerfMYFLT::setIdentifier(int /*pass*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    auto data = getIdentData(csound, args, true, 1, 2);
    data.type = CabbageOpcodeData::MessageType::Identifier;

    const int argIndex = 2;

    if (!testForValidNumberOfInputs(in_count(), argIndex + 1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }

    const int trigger = int(args[0]);

    if (trigger == 0)
    {
        return IS_OK;
    }
    else
    {
        updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::Scalar);

        // If updating the value identifier, also update the Csound channel
        if (data.identifier == "value")
        {
            hostData->setControlChannel(args.str_data(1).data, args[argIndex + 1]);
        }

        // Only enqueue if value has changed
        if (hostData->isValueDifferent(data))
        {
            hostData->updateChannelCache(data);
        }
    }

    return IS_OK;
}

//=====================================================================================
// cabbageSet "channel", "identifier", iArg1
//=====================================================================================
int CabbageSetInitMYFLT::setIdentifier(int /*pass*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    auto data = getIdentData(csound, args, true, 0, 1);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    const int argIndex = 1;

    if (!testForValidNumberOfInputs(in_count(), argIndex + 1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }

    if (in_count() == 3)
        updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::Scalar);
    else
        updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::Scalar);

    hostData->updateChannelCache(data);

    return IS_OK;
}

//=====================================================================================
// cabbageSet kTrig, "channel", "identifier", kArg1
//=====================================================================================
int CabbageSetPerfMYFLTArray::setIdentifier(int /*pass*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    auto data = getIdentData(csound, args, true, 1, 2);
    data.type = CabbageOpcodeData::MessageType::Identifier;

    const int argIndex = 2;

    if (!testForValidNumberOfInputs(in_count(), argIndex + 1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }

    const int trigger = int(args[0]);

    if (trigger == 0)
    {
        return IS_OK;
    }
    else
    {
        updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::Array);
        if (hostData->isValueDifferent(data))
            hostData->updateChannelCache(data);
    }

    return IS_OK;
}

//=====================================================================================
// cabbageSet "channel", "identifier", iArg1[]
//=====================================================================================
int CabbageSetInitMYFLTArray::setIdentifier(int /*pass*/)
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    auto data = getIdentData(csound, args, true, 0, 1);
    data.type = CabbageOpcodeData::MessageType::Identifier;
    const int argIndex = 1;

    if (!testForValidNumberOfInputs(in_count(), argIndex + 1))
    {
        csound->init_error("Not enough input arguments\n");
        return NOTOK;
    }

    updateWidgetJson(data.cabbageJson, args, argIndex + 1, data.identifier, CabbageOpcodeData::ArgType::Scalar);

    // If updating the value identifier, also update the Csound channel
    if (data.identifier == "value")
    {
        hostData->setControlChannel(args.str_data(0).data, args[argIndex + 1]);
    }

    hostData->updateChannelCache(data);

    return IS_OK;
}
