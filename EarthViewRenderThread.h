#pragma once
#include <System.Classes.hpp>
#include <mutex>
class TOpenGLPanel;

extern std::mutex g_glMutex;

class EarthView;
class TileManager;

class TEarthViewRenderThread : public TThread {
private:
    TOpenGLPanel* FPanel;
    EarthView* FEarthView;
    TileManager* FTileManager;
    void __fastcall RenderTask();
    void __fastcall NotifyUI();
protected:
    void __fastcall Execute() override;
public:
    __fastcall TEarthViewRenderThread(TOpenGLPanel* panel,
                                      EarthView* earthView,
                                      TileManager* tileManager);
};
