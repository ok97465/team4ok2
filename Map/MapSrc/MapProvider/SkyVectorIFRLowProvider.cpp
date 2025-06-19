#include "SkyVectorIFRLowProvider.h"

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

#define SKYVECTOR_URL            "http://t.skyvector.com"
#define SKYVECTOR_CHART_IFR_LOW  "302"
#define SKYVECTOR_KEY             "V7pMh4xRihf1nr61"
#define SKYVECTOR_EDITION         "2506"

SkyVectorIFRLowProvider::SkyVectorIFRLowProvider(const std::string& dir, bool fromInternet)
    : inet(fromInternet) {
    std::string baseDir = dir;
    baseDir += "..\\IFR_Low_Map";
    baseDir += fromInternet ? "_Live\\" : "\\";
    cacheDir = baseDir; 
}

SkyVectorIFRLowProvider::~SkyVectorIFRLowProvider() {

}

void SkyVectorIFRLowProvider::FetchTile(TilePtr tile, KeyholeConnection* conn) {
    printf("[SkyVectorIFRLowProvider][%s] ----> tile x(%d), y(%d), level(%d) \n", __func__, tile->GetX(), tile->GetY(), tile->GetLevel());

    gefetch_error res = gefetch_fetch_image_skyvector(
        conn->GetGEFetch(),
        SKYVECTOR_KEY, SKYVECTOR_CHART_IFR_LOW, SKYVECTOR_EDITION,
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

std::string SkyVectorIFRLowProvider::GetCacheDir() const {
    return cacheDir;
}

std::string SkyVectorIFRLowProvider::GetURI() const {
    printf("[SkyVectorIFRHighProvider][%s]\n", __func__);
    return SKYVECTOR_URL;
}