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
    FreeOnTerminate = true;
}

void __fastcall TEarthViewRenderThread::NotifyUI()
{
    Form1->ObjectDisplay->Repaint();
}

void __fastcall TEarthViewRenderThread::Execute()
{
    while (!Terminated)
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
            // Queue the repaint instead of blocking the thread with Synchronize
            // so termination doesn't deadlock while waiting for the mutex.
            TThread::Queue(nullptr, NotifyUI);
        }
        Sleep(30);
    }
}
