#include "GoogleMapsProvider.h"

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
    printf("[GoogleMapsProvider][%s] ----> tile x(%d), y(%d), level(%d) \n", __func__, tile->GetX(), tile->GetY(), tile->GetLevel());

    gefetch_error res = gefetch_fetch_image_googlemaps(conn->GetGEFetch(), tile->GetX(), tile->GetY(), tile->GetLevel());
    if ((res == GEFETCH_NOT_FOUND) || (res == GEFETCH_INVALID_ZOOM)) {
        tile->Null();
        printf("[%s] tile->Null\n", __func__);
        return;
    }
    else if (res != GEFETCH_OK) {
        sleep(1);
        throw Exception("gefetch_fetch_image_googlemaps() failed");
    }

    RawBuffer* buf = new RawBuffer(gefetch_get_data_ptr(conn->GetGEFetch()), gefetch_get_data_size(conn->GetGEFetch()));

    printf("[%s] buf size(%d)\n", __func__, buf->Size());    
    try {
        tile->Load(buf, 1);
        printf("[%s] tile->load\n", __func__);
    } catch (...) {
        delete buf;
        throw;
    }
}

std::string GoogleMapsProvider::GetCacheDir() const {
    printf("[GoogleMapsProvider][%s] cacheDir(%s) \n", __func__, cacheDir.c_str());
    return cacheDir;
}

std::string GoogleMapsProvider::GetURI() const {
    printf("[GoogleMapsProvider][%s]\n", __func__);
    return GOOGLE_URL;
}