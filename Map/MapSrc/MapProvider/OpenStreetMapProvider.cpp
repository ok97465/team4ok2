#include "OpenStreetMapProvider.h"
#include <cstdio>
#include <stdexcept>
#include "LogHandler.h"

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

//#define OSM_URL "http://tile.openstreetmap.org"
#define OSM_URL "https://api.maptiler.com/maps/openstreetmap"
#define API_KEY "@2x.jpg?key=jPzOiONYSlDVXCeO7jIa"


OpenStreetMapProvider::OpenStreetMapProvider(const std::string& dir, bool fromInternet)
    : inet(fromInternet) {
    std::string baseDir = dir;
    baseDir += "..\\OpenStreetMap";
    baseDir += fromInternet ? "_Live\\" : "\\";
    cacheDir = baseDir;
} 

OpenStreetMapProvider::~OpenStreetMapProvider() {

}

void OpenStreetMapProvider::FetchTile(TilePtr tile, KeyholeConnection* conn) {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[OpenStreetMapProvider] tile x(%d), y(%d), level(%d)", 
                tile->GetX(), tile->GetY(), tile->GetLevel());

    gefetch_error res = gefetch_fetch_image_openstreetmap(conn->GetGEFetch(), API_KEY, tile->GetX(), tile->GetY(), tile->GetLevel());
    if ((res == GEFETCH_NOT_FOUND) || (res == GEFETCH_INVALID_ZOOM)) {
        tile->Null();
        LOG_DEBUG_F(LogHandler::CAT_MAP, "[%s] tile->Null (NOT_FOUND or INVALID_ZOOM)", __func__);
        return;
    }
    else if (res != GEFETCH_OK) {
        sleep(1);
        throw Exception("gefetch_fetch_image_openstreetmap() failed");
    }

    RawBuffer* buf = new RawBuffer(gefetch_get_data_ptr(conn->GetGEFetch()), gefetch_get_data_size(conn->GetGEFetch()));

    LOG_DEBUG_F(LogHandler::CAT_MAP, "[%s] buf size(%d)", __func__, buf->Size());
    try {
        if(tile->IsNull() == 1) {
            LOG_WARNING_F(LogHandler::CAT_MAP, "[%s] tile is null", __func__);
            delete buf;
            return;
        }
        tile->Load(buf, 1);
        LOG_DEBUG_F(LogHandler::CAT_MAP, "[%s] tile->load success", __func__);
    } catch (...) {
        LOG_ERROR_F(LogHandler::CAT_MAP, "[%s] tile->load fail", __func__);
        delete buf;
        throw;
    }
}

std::string OpenStreetMapProvider::GetCacheDir() const {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[OpenStreetMapProvider] returning cache dir: %s", cacheDir.c_str());
    return cacheDir;
}

std::string OpenStreetMapProvider::GetURI() const {
    LOG_DEBUG_F(LogHandler::CAT_MAP, "[OpenStreetMapProvider][%s]", __func__);
    return OSM_URL;
}