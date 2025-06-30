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

void __fastcall TEarthViewRenderThread::Execute()
{
    while (!Terminated)
    {
        if (FEarthView && FPanel)
        {
            {
                std::lock_guard<std::mutex> lock(g_glMutex);
                FPanel->MakeOpenGLPanelCurrent();
                FEarthView->Animate();
                FEarthView->Render(Form1->DrawMap->Checked);
                if (FTileManager)
                    FTileManager->Cleanup();
                FPanel->MakeOpenGLPanelNotCurrent();
            }
            TThread::Synchronize(nullptr, [](){
                Form1->ObjectDisplay->Repaint();
            });
        }
        Sleep(30);
    }
}
