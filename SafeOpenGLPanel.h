#pragma once
#include "Components/OpenGLv0.5BDS2006/Component/OpenGLPanel.h"
#include <mutex>

extern std::mutex g_glMutex;

class TSafeOpenGLPanel : public TOpenGLPanel {
public:
    __fastcall TSafeOpenGLPanel(TComponent* Owner) : TOpenGLPanel(Owner) {}
protected:
    void __fastcall Paint() override;
    void __fastcall Resize() override;
};
