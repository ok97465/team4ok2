#pragma once
#include <System.Classes.hpp>
#include <mutex>
class TOpenGLPanel;

extern std::mutex g_glMutex;

class TEarthViewRenderThread : public TThread {
private:
    TOpenGLPanel* FPanel;
protected:
    void __fastcall Execute() override;
public:
    __fastcall TEarthViewRenderThread(TOpenGLPanel* panel);
};
