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

#include "CabbageJsonOpcodes.h"

namespace
{

std::string safeString(const STRINGDAT &s)
{
    if (s.data == nullptr || s.size == 0)
        return std::string();
    size_t len = strnlen(s.data, static_cast<size_t>(s.size) + 1);
    return std::string(s.data, len);
}

bool isIndex(const std::string &key)
{
    return !key.empty() && std::all_of(key.begin(), key.end(), ::isdigit);
}

// Walk a dot-notation path ("nodes.0.params.rate"). Numeric segments index
// arrays. Returns nullptr when any step is missing or mistyped.
const nlohmann::json *findPath(const nlohmann::json &doc, const std::string &path)
{
    if (path.empty())
        return nullptr;
    const nlohmann::json *current = &doc;
    size_t start = 0;
    while (true)
    {
        size_t dot = path.find('.', start);
        std::string key = (dot == std::string::npos) ? path.substr(start) : path.substr(start, dot - start);
        if (key.empty())
            return nullptr;
        if (current->is_array() && isIndex(key))
        {
            size_t idx = std::stoul(key);
            if (idx >= current->size())
                return nullptr;
            current = &(*current)[idx];
        }
        else if (current->is_object() && current->contains(key))
        {
            current = &(*current)[key];
        }
        else
        {
            return nullptr;
        }
        if (dot == std::string::npos)
            return current;
        start = dot + 1;
    }
}

std::string jsonToString(const nlohmann::json &v)
{
    if (v.is_string())
        return v.get<std::string>();
    if (v.is_null())
        return std::string();
    return v.dump();
}

double jsonToNumber(const nlohmann::json &v)
{
    if (v.is_number())
        return v.get<double>();
    if (v.is_boolean())
        return v.get<bool>() ? 1.0 : 0.0;
    if (v.is_string())
    {
        std::string s = v.get<std::string>();
        size_t first = s.find_first_not_of(" \t\r\n");
        size_t last = s.find_last_not_of(" \t\r\n");
        if (first == std::string::npos)
            return 0.0;
        s = s.substr(first, last - first + 1);
        char *end = nullptr;
        double d = std::strtod(s.c_str(), &end);
        if (end != nullptr && *end == '\0')
            return d;
    }
    return 0.0;
}

const char *jsonTypeName(const nlohmann::json &v)
{
    if (v.is_string())
        return "string";
    if (v.is_number())
        return "number";
    if (v.is_boolean())
        return "boolean";
    if (v.is_array())
        return "array";
    if (v.is_object())
        return "object";
    return "null";
}

struct ParsedDoc
{
    bool ok = false;
    nlohmann::json doc;
};

ParsedDoc parseDoc(const std::string &text)
{
    ParsedDoc p;
    if (text.empty())
        return p;
    try
    {
        p.doc = nlohmann::json::parse(text);
        p.ok = true;
    }
    catch (const nlohmann::json::exception &e)
    {
        lattice::logWarning << "cabbageJson: invalid JSON document: " << e.what();
    }
    return p;
}

} // namespace

//=====================================================================================
// SVal cabbageJsonGet SJson, SPath
//=====================================================================================
int CabbageJsonGetString::get(bool /*perf*/)
{
    if (in_count() != 2)
        return IS_OK;
    std::string docText = safeString(inargs.str_data(0));
    std::string path = safeString(inargs.str_data(1));
    if (docText != lastDoc || path != lastPath)
    {
        lastDoc = docText;
        lastPath = path;
        lastResult.clear();
        ParsedDoc p = parseDoc(docText);
        if (p.ok)
        {
            const nlohmann::json *v = findPath(p.doc, path);
            if (v != nullptr)
                lastResult = jsonToString(*v);
        }
    }
    outargs.str_data(0).size = int(lastResult.size()) + 1;
    outargs.str_data(0).data = csound->strdup(const_cast<char *>(lastResult.c_str()));
    return IS_OK;
}

//=====================================================================================
// SVal, kTrig cabbageJsonGet SJson, SPath
//=====================================================================================
int CabbageJsonGetStringWithTrigger::kperf()
{
    if (in_count() != 2)
        return IS_OK;
    std::string docText = safeString(inargs.str_data(0));
    std::string path = safeString(inargs.str_data(1));
    std::string result;
    if (docText == lastDoc && path == lastPath)
    {
        result = lastResult;
        outargs[1] = 0;
    }
    else
    {
        lastDoc = docText;
        lastPath = path;
        ParsedDoc p = parseDoc(docText);
        if (p.ok)
        {
            const nlohmann::json *v = findPath(p.doc, path);
            if (v != nullptr)
                result = jsonToString(*v);
        }
        outargs[1] = (result != lastResult) ? 1 : 0;
        lastResult = result;
    }
    outargs.str_data(0).size = int(lastResult.size()) + 1;
    outargs.str_data(0).data = csound->strdup(const_cast<char *>(lastResult.c_str()));
    return IS_OK;
}

//=====================================================================================
// kval cabbageJsonGet SJson, SPath / ival cabbageJsonGet SJson, SPath
//=====================================================================================
int CabbageJsonGetNumber::get()
{
    if (in_count() != 2)
        return IS_OK;
    std::string docText = safeString(inargs.str_data(0));
    std::string path = safeString(inargs.str_data(1));
    if (docText != lastDoc || path != lastPath)
    {
        lastDoc = docText;
        lastPath = path;
        lastResult = 0.0;
        ParsedDoc p = parseDoc(docText);
        if (p.ok)
        {
            const nlohmann::json *v = findPath(p.doc, path);
            if (v != nullptr)
                lastResult = jsonToNumber(*v);
        }
    }
    outargs[0] = lastResult;
    return IS_OK;
}

//=====================================================================================
// SArr[] cabbageJsonGet SJson, SPath (i-time only)
//=====================================================================================
int CabbageJsonGetStringArray::get()
{
    if (in_count() != 2)
        return IS_OK;
    std::vector<std::string> items;
    ParsedDoc p = parseDoc(safeString(inargs.str_data(0)));
    if (p.ok)
    {
        const nlohmann::json *v = findPath(p.doc, safeString(inargs.str_data(1)));
        if (v != nullptr && v->is_array())
        {
            for (const auto &el : *v)
                items.push_back(jsonToString(el));
        }
    }
    csnd::Vector<STRINGDAT> &out = outargs.vector_data<STRINGDAT>(0);
    out.init(csound, static_cast<int>(items.size()), this->insdshead);
    for (size_t i = 0; i < items.size(); ++i)
    {
        out[i].size = static_cast<int>(items[i].size() + 1);
        out[i].data = csound->strdup(const_cast<char *>(items[i].c_str()));
    }
    return IS_OK;
}

//=====================================================================================
// kArr[] cabbageJsonGet SJson, SPath (i-time only)
//=====================================================================================
int CabbageJsonGetNumberArray::get()
{
    if (in_count() != 2)
        return IS_OK;
    std::vector<double> items;
    ParsedDoc p = parseDoc(safeString(inargs.str_data(0)));
    if (p.ok)
    {
        const nlohmann::json *v = findPath(p.doc, safeString(inargs.str_data(1)));
        if (v != nullptr && v->is_array())
        {
            for (const auto &el : *v)
                items.push_back(jsonToNumber(el));
        }
    }
    csnd::Vector<MYFLT> &out = outargs.myfltvec_data(0);
    out.init(csound, static_cast<int>(items.size()), this->insdshead);
    for (size_t i = 0; i < items.size(); ++i)
        out[i] = items[i];
    return IS_OK;
}

//=====================================================================================
// iHas/kHas cabbageJsonHas SJson, SPath
//=====================================================================================
int CabbageJsonHas::check()
{
    if (in_count() != 2)
        return IS_OK;
    outargs[0] = 0;
    ParsedDoc p = parseDoc(safeString(inargs.str_data(0)));
    if (p.ok && findPath(p.doc, safeString(inargs.str_data(1))) != nullptr)
        outargs[0] = 1;
    return IS_OK;
}

//=====================================================================================
// iLen/kLen cabbageJsonLen SJson, SPath
//=====================================================================================
int CabbageJsonLen::check()
{
    if (in_count() != 2)
        return IS_OK;
    outargs[0] = 0;
    ParsedDoc p = parseDoc(safeString(inargs.str_data(0)));
    if (p.ok)
    {
        const nlohmann::json *v = findPath(p.doc, safeString(inargs.str_data(1)));
        if (v != nullptr && (v->is_array() || v->is_object()))
            outargs[0] = static_cast<MYFLT>(v->size());
    }
    return IS_OK;
}

//=====================================================================================
// SType cabbageJsonType SJson, SPath
//=====================================================================================
int CabbageJsonType::get(bool /*perf*/)
{
    if (in_count() != 2)
        return IS_OK;
    std::string docText = safeString(inargs.str_data(0));
    std::string path = safeString(inargs.str_data(1));
    if (docText != lastDoc || path != lastPath)
    {
        lastDoc = docText;
        lastPath = path;
        lastResult = "missing";
        ParsedDoc p = parseDoc(docText);
        if (p.ok)
        {
            const nlohmann::json *v = findPath(p.doc, path);
            if (v != nullptr)
                lastResult = jsonTypeName(*v);
        }
    }
    outargs.str_data(0).size = int(lastResult.size()) + 1;
    outargs.str_data(0).data = csound->strdup(const_cast<char *>(lastResult.c_str()));
    return IS_OK;
}

//=====================================================================================
// Shared builder for cabbageJsonSet
//=====================================================================================
static bool setPathValue(nlohmann::json &doc, const std::string &path, const nlohmann::json &value)
{
    if (path.empty())
        return false;
    try
    {
        nlohmann::json *current = &doc;
        size_t start = 0;
        while (true)
        {
            size_t dot = path.find('.', start);
            std::string key = (dot == std::string::npos) ? path.substr(start) : path.substr(start, dot - start);
            bool last = (dot == std::string::npos);
            if (key.empty())
                return false;
            if (current->is_array() && isIndex(key))
            {
                size_t idx = std::stoul(key);
                if (idx >= current->size())
                    return false;
                if (last)
                {
                    (*current)[idx] = value;
                    return true;
                }
                current = &(*current)[idx];
            }
            else if (current->is_object())
            {
                if (last)
                {
                    (*current)[key] = value;
                    return true;
                }
                if (!current->contains(key))
                    (*current)[key] = nlohmann::json::object();
                current = &(*current)[key];
                if (!current->is_object() && !current->is_array())
                    return false;
            }
            else
            {
                return false;
            }
            start = dot + 1;
        }
    }
    catch (const nlohmann::json::exception &e)
    {
        lattice::logWarning << "cabbageJsonSet: " << e.what();
        return false;
    }
}

//=====================================================================================
// SJson cabbageJsonSet SJson, SPath, SVal
//=====================================================================================
int CabbageJsonSetString::set()
{
    if (in_count() != 3)
        return IS_OK;
    std::string result;
    ParsedDoc p = parseDoc(safeString(inargs.str_data(0)));
    if (p.ok)
    {
        std::string sval = safeString(inargs.str_data(2));
        if (setPathValue(p.doc, safeString(inargs.str_data(1)), sval))
            result = p.doc.dump();
        else
            lattice::logWarning << "cabbageJsonSet: cannot set path (type clash or index out of range)";
    }
    outargs.str_data(0).size = int(result.size()) + 1;
    outargs.str_data(0).data = csound->strdup(const_cast<char *>(result.c_str()));
    return IS_OK;
}

//=====================================================================================
// SJson cabbageJsonSet SJson, SPath, kVal
//=====================================================================================
int CabbageJsonSetNumber::set()
{
    if (in_count() != 3)
        return IS_OK;
    std::string result;
    ParsedDoc p = parseDoc(safeString(inargs.str_data(0)));
    if (p.ok)
    {
        if (setPathValue(p.doc, safeString(inargs.str_data(1)), inargs[2]))
            result = p.doc.dump();
        else
            lattice::logWarning << "cabbageJsonSet: cannot set path (type clash or index out of range)";
    }
    outargs.str_data(0).size = int(result.size()) + 1;
    outargs.str_data(0).data = csound->strdup(const_cast<char *>(result.c_str()));
    return IS_OK;
}
