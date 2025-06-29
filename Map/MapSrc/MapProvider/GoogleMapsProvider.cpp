#include "GoogleMapsProvider.h"
#include <cstdio>
#include <stdexcept>
#include "LogHandler.h"

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

#define GOOGLE_URL "http://mt1.google.com"

GoogleMapsProvider::GoogleMapsProvider(const std::string& dir, bool fromInternet)
    : inet(fromInternet) {
    std::string baseDir = dir;
    baseDir += "..\\GoogleMap";
    baseDir += fromInternet ? "_Live\\" : "\\";
    cacheDir = baseDir;
} 

GoogleMapsProvider::~GoogleMapsProvider() {

}

void GoogleMapsProvider::FetchTile(TilePtr tile, KeyholeConnection* conn) {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[GoogleMapsProvider] tile x(%d), y(%d), level(%d)", 
                tile->GetX(), tile->GetY(), tile->GetLevel());

    gefetch_error res = gefetch_fetch_image_googlemaps(conn->GetGEFetch(), tile->GetX(), tile->GetY(), tile->GetLevel());
    if ((res == GEFETCH_NOT_FOUND) || (res == GEFETCH_INVALID_ZOOM)) {
        tile->Null();
        LOG_DEBUG_F(LogHandler::CAT_MAP, "Tile not found: x=%d, y=%d, level=%d", 
                    tile->GetX(), tile->GetY(), tile->GetLevel());
        return;
    }
    else if (res != GEFETCH_OK) {
        sleep(1);
        throw Exception("gefetch_fetch_image_googlemaps() failed");
    }

    RawBuffer* buf = new RawBuffer(gefetch_get_data_ptr(conn->GetGEFetch()), gefetch_get_data_size(conn->GetGEFetch()));

    // 큰 타일만 로깅 (50KB 이상)
    if (buf->Size() > 50000) {
        LOG_DEBUG_F(LogHandler::CAT_MAP, "Large tile loaded: %d bytes", buf->Size());
    }
    try {
        tile->Load(buf, 1);
        // 타일 로딩 완료 로그 - 일반 DEBUG 레벨로 변경
        LOG_DEBUG(LogHandler::CAT_MAP, "[GoogleMapsProvider] tile loaded successfully");
    } catch (...) {
        delete buf;
        throw;
    }
}

std::string GoogleMapsProvider::GetCacheDir() const {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[GoogleMapsProvider] returning cache dir: %s", cacheDir.c_str());
    return cacheDir;
}

std::string GoogleMapsProvider::GetURI() const {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[GoogleMapsProvider][%s]", __func__);
    return GOOGLE_URL;
}