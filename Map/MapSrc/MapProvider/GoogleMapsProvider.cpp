#include "GoogleMapsProvider.h"

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

#define GOOGLE_URL "http://mt1.google.com"

GoogleMapsProvider::GoogleMapsProvider(const std::string& dir, bool fromInternet)
    : cacheDir(dir), inet(fromInternet), conn(nullptr), storage(nullptr) {}    

GoogleMapsProvider::~GoogleMapsProvider() {
    if(conn) delete conn;
}

void GoogleMapsProvider::SetStorage(FilesystemStorage* s) { storage = s; }

void GoogleMapsProvider::Init() {
    if(inet) {
        conn = new KeyholeConnection(GOOGLE_URL, this);
        if (storage && conn) {
            conn->SetSaveStorage(storage);
            storage->SetNextLoadStorage(conn);
        }    
    }
}

void GoogleMapsProvider::FetchTile(TilePtr tile) {
    if (conn) {
        gefetch_error res = gefetch_fetch_image_googlemaps(conn->GetGEFetch(), tile->GetX(), tile->GetY(), tile->GetLevel());
        if ((res == GEFETCH_NOT_FOUND) || (res == GEFETCH_INVALID_ZOOM)) {
            tile->Null();
            return;
        }
        else if (res != GEFETCH_OK) {
            sleep(1);
            throw Exception("gefetch_fetch_image_googlemaps() failed");
        }
        RawBuffer* buf = new RawBuffer(gefetch_get_data_ptr(conn->GetGEFetch()), gefetch_get_data_size(conn->GetGEFetch()));
        try {
            tile->Load(buf, conn->HasSaveStorage());
        } catch (...) {
            delete buf;
            throw;
        }
    }
}

std::string GoogleMapsProvider::GetCacheDir() const {
    return cacheDir;
}