#pragma once
#include <System.Classes.hpp>

class TLoadApiDataThread : public TThread {
protected:
    void __fastcall Execute();
    void __fastcall NotifyUI();
public:
    __fastcall TLoadApiDataThread();
};