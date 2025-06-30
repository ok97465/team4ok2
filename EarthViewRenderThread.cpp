#include <vcl.h>
#pragma hdrstop
#include "EarthViewRenderThread.h"
#include "DisplayGUI.h"
#include <windows.h>

std::mutex g_glMutex;

__fastcall TEarthViewRenderThread::TEarthViewRenderThread(TOpenGLPanel* panel)
    : TThread(false), FPanel(panel)
{
    FreeOnTerminate = true;
}

void __fastcall TEarthViewRenderThread::Execute()
{
    while (!Terminated)
    {
        if (g_EarthView && FPanel)
        {
            {
                std::lock_guard<std::mutex> lock(g_glMutex);
                FPanel->MakeOpenGLPanelCurrent();
                g_EarthView->Animate();
                g_EarthView->Render(Form1->DrawMap->Checked);
                if (g_GETileManager)
                    g_GETileManager->Cleanup();
                FPanel->MakeOpenGLPanelNotCurrent();
            }
            TThread::Synchronize(nullptr, [](){
                Form1->ObjectDisplay->Repaint();
            });
        }
        Sleep(30);
    }
}
