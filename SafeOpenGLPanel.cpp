#include <vcl.h>
#pragma hdrstop
#include "SafeOpenGLPanel.h"

#pragma package(smart_init)

void __fastcall TSafeOpenGLPanel::Paint()
{
    std::lock_guard<std::mutex> lock(g_glMutex);
    TOpenGLPanel::Paint();
}

void __fastcall TSafeOpenGLPanel::Resize()
{
    std::lock_guard<std::mutex> lock(g_glMutex);
    TOpenGLPanel::Resize();
}
