
/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#undef _CR

#include <plugin.h>
#include <readerwriterqueue.h>

struct CabbageOpcodeData
{
    //I could enum all identifiers, this would be quicker then search strings..
    enum MessageType{
        Value,
        String
    };
    std::string identifier = {};
    std::string channel = {};
    std::vector<float> numericData;
    std::vector<std::string> stringData;
    MessageType type;
    MYFLT value = 0;

};

template <std::size_t N>
struct CabbageOpcodes
{
    moodycamel::ReaderWriterQueue<CabbageOpcodeData>** vt = nullptr;
    moodycamel::ReaderWriterQueue<CabbageOpcodeData> queue;
    char* name = NULL;
    char* identifier = NULL;
    MYFLT* value = {};
    MYFLT lastValue = 0;
    MYFLT* str = {};
        
    static moodycamel::ReaderWriterQueue<CabbageOpcodeData>* getGlobalvariable(csnd::Csound* csound, moodycamel::ReaderWriterQueue<CabbageOpcodeData>** vt)
    {
        if (vt != nullptr)
        {
            return *vt;
        }
        else
        {
            csound->create_global_variable("cabbageOpcodeData", sizeof(moodycamel::ReaderWriterQueue<CabbageOpcodeData>*));
            vt = (moodycamel::ReaderWriterQueue<CabbageOpcodeData>**)csound->query_global_variable("cabbageOpcodeData");
            *vt = new moodycamel::ReaderWriterQueue<CabbageOpcodeData>(100);
            return *vt;
        }
    }
    
    CabbageOpcodeData getValueIdentData(csnd::Param<N>& args, bool init, int nameIndex, int identIndex)
    {
        CabbageOpcodeData data;
        if(init)
        {
            if(args.str_data(nameIndex).size == 0)
                name = {};
            else
                name = args.str_data(nameIndex).data;
        }

        data.identifier = "value";
        data.channel = name;
        return data;
    }
    

    CabbageOpcodeData getIdentData(csnd::Param<N>& args, bool init, int nameIndex, int identIndex)
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
        
        if(String(name).isNotEmpty())
            data.channel = name;
        if(String(identifier).isNotEmpty())
            data.identifier = Identifier(identifier);

        return data;
    }
    
};
