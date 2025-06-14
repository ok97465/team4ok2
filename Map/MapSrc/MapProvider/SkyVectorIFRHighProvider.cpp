#include "SkyVectorIFRHighProvider.h"

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

#define SKYVECTOR_URL            "http://t.skyvector.com"
#define SKYVECTOR_CHART_IFR_HIGH "304"
#define SKYVECTOR_KEY             "V7pMh4xRihf1nr61"
#define SKYVECTOR_EDITION         "2504"

SkyVectorIFRHighProvider::SkyVectorIFRHighProvider(const std::string& dir, bool fromInternet)
    : cacheDir(dir), inet(fromInternet), conn(nullptr), storage(nullptr){}

SkyVectorIFRHighProvider::~SkyVectorIFRHighProvider() {
    delete conn;
}

void SkyVectorIFRHighProvider::SetStorage(FilesystemStorage* s) { storage = s; }

void SkyVectorIFRHighProvider::Init() {
    if(inet) {
        conn = new KeyholeConnection(SKYVECTOR_URL, SKYVECTOR_KEY, SKYVECTOR_CHART_IFR_HIGH, SKYVECTOR_EDITION, this);
        if (storage && conn) {
            conn->SetSaveStorage(storage);
            storage->SetNextLoadStorage(conn);
        }
    }
}

void SkyVectorIFRHighProvider::FetchTile(TilePtr tile) {
    gefetch_error res = gefetch_fetch_image_skyvector(
        conn->GetGEFetch(),
        conn->GetKey().c_str(), conn->GetChart().c_str(), conn->GetEdition().c_str(),
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
        tile->Load(buf, conn->HasSaveStorage());
    } catch (...) {
        delete buf;
        throw;
    }
}

std::string SkyVectorIFRHighProvider::GetCacheDir() const {
    return cacheDir;
}