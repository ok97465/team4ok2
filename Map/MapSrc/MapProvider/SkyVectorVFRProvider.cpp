#include "SkyVectorVFRProvider.h"
#include "LogHandler.h"

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

#define SKYVECTOR_URL            "http://t.skyvector.com"
#define SKYVECTOR_CHART_VPS      "301"
#define SKYVECTOR_KEY             "V7pMh4xRihf1nr61"
#define SKYVECTOR_EDITION         "2506"

SkyVectorVFRProvider::SkyVectorVFRProvider(const std::string& dir, bool fromInternet)
    : inet(fromInternet) {
    std::string baseDir = dir;
    baseDir += "..\\VFR_Map";
    baseDir += fromInternet ? "_Live\\" : "\\";
    cacheDir = baseDir;
}

SkyVectorVFRProvider::~SkyVectorVFRProvider() {

}

void SkyVectorVFRProvider::FetchTile(TilePtr tile, KeyholeConnection* conn) {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[SkyVectorVFRProvider][%s] tile x(%d), y(%d), level(%d)", __func__, tile->GetX(), tile->GetY(), tile->GetLevel());

    gefetch_error res = gefetch_fetch_image_skyvector(
        conn->GetGEFetch(),
        SKYVECTOR_KEY, SKYVECTOR_CHART_VPS, SKYVECTOR_EDITION,
        tile->GetX(), tile->GetY(), tile->GetLevel()
    );
    if ((res == GEFETCH_NOT_FOUND) || (res == GEFETCH_INVALID_ZOOM)) {
        tile->Null();
        return;
    }
    else if (res != GEFETCH_OK) {
        sleep(1);
        throw Exception("gefetch_fetch_image_skyvector() failed");
    }
    RawBuffer* buf = new RawBuffer(gefetch_get_data_ptr(conn->GetGEFetch()), gefetch_get_data_size(conn->GetGEFetch()));
    try {
        tile->Load(buf, 1);
    } catch (...) {
        delete buf;
        throw;
    }
}

std::string SkyVectorVFRProvider::GetCacheDir() const {
    return cacheDir;
}

std::string SkyVectorVFRProvider::GetURI() const {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[SkyVectorVFRProvider][%s]", __func__);
    return SKYVECTOR_URL;
}