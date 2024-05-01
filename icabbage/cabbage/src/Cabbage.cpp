#include "Cabbage.h"
#include "CabbageProcessor.h"

Cabbage::Cabbage(CabbageProcessor& p, std::string file): processor(p), csdFile(file)
{
    
};

Cabbage::~Cabbage()
{
    if (csound)
    {
        csCompileResult = false;
        csound = nullptr;
        csoundParams = nullptr;
    }
}

std::vector<nlohmann::json> Cabbage::parseCsdForWidgets(std::string csdFile)
{
    std::vector<nlohmann::json> widgets;
    std::ifstream file(csdFile);
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string csdContents = oss.str();
    
    std::istringstream iss(csdContents);
    std::string line;
    while (std::getline(iss, line)) 
    {
        nlohmann::json j;
        if(getWidgetType(line) == "rslider")
        {
            j = CabbageWidgetDescriptors::getRotarySlider();
            updateJsonFromSyntax(j, line);
            std::cout << j.dump(4);
            widgets.push_back(j);
        }
    }
    
    return widgets;
}

int Cabbage::getNumberOfParameters(const std::string& csdFile)
{
    std::vector<nlohmann::json> widgets = parseCsdForWidgets(csdFile);
    int numParams = 0;
    for(auto& w : widgets)
    {
        if(w.contains("automatable") && w["automatable"] == 1)
            numParams++;
    }
    
    return numParams;
}

void Cabbage::updateJsonFromSyntax(nlohmann::json& jsonObj, const std::string& syntax) {
    std::vector<Identifier> tokens = tokeniseLine(syntax);

    // Parse tokens and update JSON object
    for (size_t i = 0; i < tokens.size(); ++i) 
    {
        if (tokens[i].name == "bounds")
        {
            if(tokens[i].numericArgs.size() == 4)
            {
                int top = tokens[i].numericArgs[0];
                int left = tokens[i].numericArgs[1];
                int width = tokens[i].numericArgs[2];
                int height = tokens[i].numericArgs[3];
                jsonObj["top"] = top;
                jsonObj["left"] = left;
                jsonObj["width"] = width;
                jsonObj["height"] = height;
            }
            else
            {
                std::cout << "The bounds() identifier takes 4 parameters" << std::endl;
            }
        } 
        else if (tokens[i].name == "text")
        {
//            std::string text = tokens[++i];
//            jsonObj["text"] = text;
        } 
        else if (tokens[i].name == "channel")
        {
            std::string channel = tokens[i].stringArgs[0];
            jsonObj["channel"] = channel;
        } 
        else if (tokens[i].name == "range")
        {
            if(tokens[i].numericArgs.size()>=3)
            {
                double min = tokens[i].numericArgs[0];
                double max = tokens[i].numericArgs[1];
                double value = tokens[i].numericArgs[2];
                jsonObj["min"] = min;
                jsonObj["max"] = max;
                jsonObj["value"] = value;
                
                if(tokens[i].numericArgs.size()==5)
                {
                    double sliderSkew = tokens[i].numericArgs[3];
                    double increment = tokens[i].numericArgs[4];
                    jsonObj["sliderSkew"] = sliderSkew;
                    jsonObj["increment"] = increment;
                }
            }
            else
            {
                std::cout << "The range() identifier takes at least 3 parameters" << std::endl;
            }
        }
        else if (tokens[i].name == "colour")
        {
//            int r = std::stoi(tokens[++i]);
//            int g = std::stoi(tokens[++i]);
//            int b = std::stoi(tokens[++i]);
//            std::stringstream ss;
//            ss << "#" << std::hex << r << g << b;
//            jsonObj["colour"] = ss.str();
        } 
        else if (tokens[i].name == "fontColour")
        {
//            std::string fontColour = tokens[++i];
//            jsonObj["fontColour"] = fontColour;
        } 
        else if (tokens[i].name == "trackerColour")
        {
//            std::string trackerColour = tokens[++i];
//            jsonObj["trackerColour"] = trackerColour;
        }
        // Add more conditions for other properties as needed
    }
}

std::vector<Cabbage::Identifier> Cabbage::tokeniseLine(const std::string& syntax)
{
    std::vector<Identifier> identifiers;

    // Regular expressions for different parts of the syntax
    std::regex identifierRegex("\\s*([a-zA-Z]+)\\s*\\(([^)]*)\\)");
    std::regex numericArgRegex("-?\\d*\\.?\\d+");
    std::regex stringArgRegex("\"([^\"]*)\"");

    std::smatch match;
    std::string::const_iterator searchStart(syntax.cbegin());

    // Find all identifiers in the syntax
    while (std::regex_search(searchStart, syntax.cend(), match, identifierRegex)) {
        Identifier identifier;
        identifier.name = match[1].str();

        // Parse numeric arguments
        std::string numericArgsStr = match[2].str();
        std::sregex_iterator numArgIter(numericArgsStr.begin(), numericArgsStr.end(), numericArgRegex);
        std::sregex_iterator end;
        while (numArgIter != end) {
            identifier.numericArgs.push_back(std::stod(numArgIter->str()));
            ++numArgIter;
        }

        // Parse string arguments
        std::sregex_iterator strArgIter(numericArgsStr.begin(), numericArgsStr.end(), stringArgRegex);
        while (strArgIter != end) {
            StringArgument strArg;
            std::string strArgValue = strArgIter->str();
            // Remove quotes and split by whitespace
            std::istringstream iss(strArgValue);
            std::string token;
            while (iss >> token) {
                identifier.stringArgs.push_back(token);
                ++strArgIter;
            }
        }

        identifiers.push_back(identifier);
        searchStart = match.suffix().first;
    }

    return identifiers;
}

std::string Cabbage::getWidgetType(const std::string& line)
{
    std::istringstream iss(line);
    std::string firstWord;
    iss >> std::ws >> firstWord;
    return firstWord;
}

bool Cabbage::setupCsound()
{
    csound = std::make_unique<Csound>();
    csound->SetHostImplementedMIDIIO(true);
    csound->SetHostImplementedAudioIO(1, 0);
    csound->SetHostData(this);
    
    csound->CreateMessageBuffer(0);
    csound->SetExternalMidiInOpenCallback(CabbageProcessor::OpenMidiInputDevice);
    csound->SetExternalMidiReadCallback(CabbageProcessor::ReadMidiData);
    csound->SetExternalMidiOutOpenCallback(CabbageProcessor::OpenMidiOutputDevice);
    csound->SetExternalMidiWriteCallback(CabbageProcessor::WriteMidiData);
    csoundParams = nullptr;
    csoundParams = std::make_unique<CSOUND_PARAMS> ();
    
    csoundParams->displays = 0;
    
    
    csound->SetOption((char*)"-n");
    csound->SetOption((char*)"-d");
    csound->SetOption((char*)"-b0");
    
    //    csoundParams->nchnls_override = numCsoundOutputChannels;
    //    csoundParams->nchnls_i_override = numCsoundInputChannels;
    
    csoundParams->sample_rate_override = 44100;
    csound->SetParams(csoundParams.get());
    //    compileCsdFile(csdFile);
    
    //csdFile = "/Users/rwalsh/Library/CloudStorage/OneDrive-Personal/Csoundfiles/addy.csd";
    std::filesystem::path file = csdFile;
    
    bool exists = std::filesystem::is_directory(file.parent_path());
    if(exists)
    {
        csCompileResult = csound->Compile (csdFile.c_str());
        
        if (csdCompiledWithoutError())
        {
            csdKsmps = csound->GetKsmps();
            csSpout = csound->GetSpout();
            csSpin = csound->GetSpin();
            csScale = csound->Get0dBFS();
        }
        else
        {
            //Csound could not compile your file?
            while (csound->GetMessageCnt() > 0)
            {
                std::string message(csound->GetFirstMessage());
                std::cout << message << std::endl;
                csound->PopFirstMessage();
            }
            return false;
        }
        

        widgets = parseCsdForWidgets(csdFile);
        for(auto& w : widgets)
        {
            if(w.contains("automatable") && w["automatable"] == 1)
            {
                if(w["type"] == "rslider")
                {
                    std::cout << w.dump(4) << std::endl;
                    processor.GetParam(numberOfParameters)->InitDouble(w["channel"].get<std::string>().c_str(),
                                                             w["value"].get<float>(),
                                                             w["min"].get<float>(),
                                                             w["max"].get<float>(),
                                                             w["increment"].get<float>(),
                                                             std::string(w["channel"].get<std::string>()+"Label1").c_str(),
                                                             iplug::IParam::EFlags::kFlagsNone,
                                                             "",
                                                             iplug::IParam::ShapePowCurve(w["sliderSkew"].get<float>()));
                    parameterChannels.push_back(CabbageParser::removeQuotes(w["channel"].get<std::string>()));
                    numberOfParameters++;
                }
            }
        }
                

        csnd::plugin<CabbageGetValue>((csnd::Csound*)csound->GetCsound(), "cabbageGetValue", "k", "S", csnd::thread::ik);

        return true;
    }
    else
        return false;
    
}
