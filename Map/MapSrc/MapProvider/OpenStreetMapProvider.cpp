#include "OpenStreetMapProvider.h"

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
    printf("[OpenStreetMapProvider][%s] ----> tile x(%d), y(%d), level(%d) \n", __func__, tile->GetX(), tile->GetY(), tile->GetLevel());

    gefetch_error res = gefetch_fetch_image_openstreetmap(conn->GetGEFetch(), API_KEY, tile->GetX(), tile->GetY(), tile->GetLevel());
    if ((res == GEFETCH_NOT_FOUND) || (res == GEFETCH_INVALID_ZOOM)) {
        tile->Null();
        printf("[%s] tile->Null\n", __func__);
        return;
    }
    else if (res != GEFETCH_OK) {
        sleep(1);
        throw Exception("gefetch_fetch_image_openstreetmap() failed");
    }

    RawBuffer* buf = new RawBuffer(gefetch_get_data_ptr(conn->GetGEFetch()), gefetch_get_data_size(conn->GetGEFetch()));

    printf("[%s] buf size(%d)\n", __func__, buf->Size());
    try {
        if(tile->IsNull() == 1) {
            printf("[%s] tile is null\n", __func__);
            delete buf;
            return;
        }
        tile->Load(buf, 1);
        printf("[%s] tile->load\n", __func__);
    } catch (...) {
        printf("[%s] tile->load fail\n", __func__);
        delete buf;
        throw;
    }
}

std::string OpenStreetMapProvider::GetCacheDir() const {
    printf("[OpenStreetMapProvider][%s] cacheDir(%s) \n", __func__, cacheDir.c_str());
    return cacheDir;
}

std::string OpenStreetMapProvider::GetURI() const {
    printf("[OpenStreetMapProvider][%s]\n", __func__);
    return OSM_URL;
}