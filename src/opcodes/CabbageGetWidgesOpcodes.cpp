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

#include "Cabbage.h"
#include "CabbageGetWidgesOpcodes.h"

//=====================================================================================
// SWidgetIds[] cabbageGetWidgets
// Returns an array of widget IDs. Uses the top-level id if it exists,
// otherwise falls back to channels[0].id.
//=====================================================================================
int CabbageGetWidgets::getWidgets()
{
    auto *hostData = static_cast<cabbage::Engine *>(csound->host_data());
    const auto &widgets = hostData->getWidgets();

    csnd::Vector<STRINGDAT> &out = outargs.vector_data<STRINGDAT>(0);
    out.init(csound, static_cast<int>(widgets.size()), this->insdshead);

    int index = 0;
    for (const auto &widget : widgets)
    {
        std::string id;
        if (widget.contains("id") && widget["id"].is_string())
        {
            id = widget["id"].get<std::string>();
        }
        else if (widget.contains("channels") && widget["channels"].is_array() &&
                 !widget["channels"].empty() && widget["channels"][0].contains("id") &&
                 widget["channels"][0]["id"].is_string())
        {
            id = widget["channels"][0]["id"].get<std::string>();
        }

        out[index].size = static_cast<int>(id.size() + 1);
        out[index].data = csound->strdup((char *)id.c_str());
        index++;
    }

    return IS_OK;
}
