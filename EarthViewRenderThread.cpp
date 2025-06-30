#include <vcl.h>
#pragma hdrstop
#include "EarthViewRenderThread.h"
#include "DisplayGUI.h"
#include <windows.h>

std::mutex g_glMutex;

__fastcall TEarthViewRenderThread::TEarthViewRenderThread(TOpenGLPanel* panel,
                                                         EarthView* earthView,
                                                         TileManager* tileManager)
    : TThread(false), FPanel(panel), FEarthView(earthView),
      FTileManager(tileManager)
{
    // manage destruction manually in the form destructor
    FreeOnTerminate = false;
}

void __fastcall TEarthViewRenderThread::RenderTask()
{
    if (FEarthView && FPanel)
    {
        std::lock_guard<std::mutex> lock(g_glMutex);
        FPanel->MakeOpenGLPanelCurrent();

        if (Form1->DrawMap->Checked)
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        else
            glClearColor(0.37f, 0.37f, 0.37f, 0.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        FEarthView->Animate();
        FEarthView->Render(Form1->DrawMap->Checked);
        if (FTileManager)
            FTileManager->Cleanup();

        FPanel->MakeOpenGLPanelNotCurrent();
    }
}

void __fastcall TEarthViewRenderThread::NotifyUI()
{
    if (Form1 && Form1->ObjectDisplay)
        Form1->ObjectDisplay->Invalidate();
}

void __fastcall TEarthViewRenderThread::Execute()
{
    while (!Terminated)
    {
        RenderTask();
        TThread::Synchronize(nullptr, NotifyUI);
        Sleep(30);
    }
}
