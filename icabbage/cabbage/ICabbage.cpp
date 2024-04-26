#include "ICabbage.h"
#include "IPlug_include_in_plug_src.h"


ICabbage::ICabbage(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets)), server(9999, "127.0.0.1")
{
  this->GetParam(kGain)->InitGain("Gain", -70., -70, 0.);
  
#ifdef DEBUG
  SetEnableDevTools(true);
#endif
  
  // Hard-coded paths must be modified!
  mEditorInitFunc = [&]() {
#ifdef OS_WIN
    LoadFile(R"(C:\Users\oli\Dev\iPlug2\Examples\ICabbage\resources\web\index.html)", nullptr);
#else
    LoadFile("index.html", GetBundleID());
#endif
    
    EnableScroll(false);
  };


    server.setOnClientMessageCallback([this](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket & webSocket, const ix::WebSocketMessagePtr & msg) {
        // The ConnectionState object contains information about the connection,
        // at this point only the client ip address and the port.
        std::cout << "Remote ip: " << connectionState->getRemoteIp() << std::endl;
        if (msg->type == ix::WebSocketMessageType::Open)
        {
            std::cout << "New connection" << std::endl;
            std::cout << "id: " << connectionState->getId() << std::endl;
            std::cout << "Uri: " << msg->openInfo.uri << std::endl;
            std::cout << "Headers:" << std::endl;
            for (auto it : msg->openInfo.headers)
            {
                std::cout << "\t" << it.first << ": " << it.second << std::endl;
            }
        }
        else if (msg->type == ix::WebSocketMessageType::Message)
        {
            //all incoming messages
            auto value = 1-(std::abs(std::stod(msg->str))/70.0);
            GetParam(0)->SetNormalized(value);
            SendParameterValueFromDelegate(0, value, true);
            std::cout << "Received: " << msg->str << " value:" << value << std::endl;
        }
    });
    
    
    auto res = server.listen();
    if (!res.first)
    {
        // Error handling
        return 1;
    }

    server.disablePerMessageDeflate();

    // Run the server in the background. Server can be stoped by calling server.stop()
    server.start();
    
}

ICabbage::~ICabbage(){
    server.stop();
}

void ICabbage::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->DBToAmp();
  
  sample maxVal = 0.;
  
  mOscillator.ProcessBlock(inputs[0], nFrames); // comment for audio in

  for (int s = 0; s < nFrames; s++)
  {
    outputs[0][s] = inputs[0][s] * mGainSmoother.Process(gain);
    outputs[1][s] = outputs[0][s]; // copy left
    
    maxVal += std::fabs(outputs[0][s]);
  }
  
  mLastPeak = static_cast<float>(maxVal / (sample) nFrames);
}

void ICabbage::OnReset()
{
  auto sr = GetSampleRate();
  mOscillator.SetSampleRate(sr);
  mGainSmoother.SetSmoothTime(20., sr);
}

bool ICabbage::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == kMsgTagButton1)
    Resize(512, 335);
  else if(msgTag == kMsgTagButton2)
    Resize(1024, 335);
  else if(msgTag == kMsgTagButton3)
    Resize(1024, 768);
  else if (msgTag == kMsgTagBinaryTest)
  {
    auto uint8Data = reinterpret_cast<const uint8_t*>(pData);
    DBGMSG("Data Size %i bytes\n",  dataSize);
    DBGMSG("Byte values: %i, %i, %i, %i\n", uint8Data[0], uint8Data[1], uint8Data[2], uint8Data[3]);
  }

  return false;
}

void ICabbage::OnIdle()
{
  if (mLastPeak > 0.01)
    SendControlValueFromDelegate(kCtrlTagMeter, mLastPeak);
}

void ICabbage::OnParamChange(int paramIdx)
{
    DBGMSG("gain %f\n", GetParam(paramIdx)->Value());
    auto client = server.getClients();
    if(client.size()>0)
    {
        auto socket = *(client.begin());
        socket->sendText(std::to_string(GetParam(paramIdx)->Value()));
    }
}

void ICabbage::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  msg.PrintMsg();
  SendMidiMsg(msg);
}
