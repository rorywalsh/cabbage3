
/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#undef _CR

#include <readerwriterqueue.h>
#include <plugin.h>
#include "json.hpp"

#define IS_OK 0
#define NOT_OK 1

struct CabbageOpcodeData
{
    enum MessageType{
        Value,
        Identifier
    };
    
    enum PassType{
        Init,
        Perf
    };
    
    
    
    std::string identifierText = {};
    std::string channel = {};
    std::vector<float> numericData;
    std::vector<std::string> stringData;
    bool hasStringArgs() const {
        return !stringData.empty();
    }
    MessageType type;
    MYFLT value = 0;

};

template <std::size_t NumInputParams>
struct CabbageOpcodes
{
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>** od = nullptr;
    moodycamel::ReaderWriterQueue<CabbageOpcodeData> queue;
    std::vector<nlohmann::json>** wd = nullptr;
    char* name = NULL;
    char* identifier = NULL;
    MYFLT* value = {};
    MYFLT lastValue = 0;
    MYFLT* str = {};
        
    static std::vector<nlohmann::json>* getWidgetDataGlobalvariable(csnd::Csound* csound, std::vector<nlohmann::json>** wd)
    {
        if (wd != nullptr)
        {
            return *wd;
        }
        else
        {
            csound->create_global_variable("cabbageWidgetData", sizeof(std::vector<nlohmann::json>*));
            wd = (std::vector<nlohmann::json>**)csound->query_global_variable("cabbageWidgetData");
            return *wd;
        }
    }
    
    static moodycamel::ReaderWriterQueue<CabbageOpcodeData>* getOpcodeDataGlobalvariable(csnd::Csound* csound, moodycamel::ReaderWriterQueue<CabbageOpcodeData>** od)
    {
        if (od != nullptr)
        {
            return *od;
        }
        else
        {
            csound->create_global_variable("cabbageOpcodeData", sizeof(moodycamel::ReaderWriterQueue<CabbageOpcodeData>*));
            od = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
            *od = new moodycamel::ReaderWriterQueue<CabbageOpcodeData>(100);
            return *od;
        }
    }
    
    static bool hasNullTerminator(const char* str, size_t length)
    {
        for (size_t i = 0; i < length; ++i) 
        {
            if (str[i] == '\0') 
            {
                return true;
            }
        }
        return false;
    }
    
    
    static std::string removeNullTerminator(const std::string& str) 
    {
        // Create a copy of the string
        std::string result = str;

        // If the last character is null terminator, remove it
        if (!result.empty() && result.back() == '\0') {
            result.pop_back();
        }

        return result;
    }
    
    bool hasNullTerminator(const std::string& str)
    {
        const char* cStr = str.c_str();
        size_t length = str.size();

        // Check if the character after the last element is a null terminator
        return cStr[length] == '\0';
    }
    
    CabbageOpcodeData getValueIdentData(csnd::Param<NumInputParams>& args, bool init, int nameIndex, int identIndex)
    {
        CabbageOpcodeData data;
        if(init)
        {
            if(args.str_data(nameIndex).size == 0)
                name = {};
            else
                name = args.str_data(nameIndex).data;
        }

        data.identifierText = "value";
        data.channel = name;
        return data;
    }
    

    CabbageOpcodeData getIdentData(csnd::Param<NumInputParams>& args, bool init, int nameIndex, int identIndex)
    {
        CabbageOpcodeData data;
        if(init)
        {
            if(args.str_data(nameIndex).size == 0)
                name = {};
            else
            {
                name = args.str_data(nameIndex).data;
            }

            if(args.str_data(identIndex).size == 0)
                identifier = {};
            else
                identifier = args.str_data(identIndex).data;
        }
        
        data.channel = name;
        data.identifierText = identifier;

        return data;
    }
    
};
