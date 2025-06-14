#include "SkyVectorVFRProvider.h"

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

#define SKYVECTOR_URL            "http://t.skyvector.com"
#define SKYVECTOR_CHART_VPS      "301"
#define SKYVECTOR_KEY             "V7pMh4xRihf1nr61"
#define SKYVECTOR_EDITION         "2504"

SkyVectorVFRProvider::SkyVectorVFRProvider(const std::string& dir, bool fromInternet)
    : cacheDir(dir), inet(fromInternet), conn(nullptr), storage(nullptr) {} 

SkyVectorVFRProvider::~SkyVectorVFRProvider() {
    delete conn;
}

void SkyVectorVFRProvider::SetStorage(FilesystemStorage* s) { storage = s; }

void SkyVectorVFRProvider::Init() {
    if(!inet) return;    
    
    conn = new KeyholeConnection(SKYVECTOR_URL, SKYVECTOR_KEY, SKYVECTOR_CHART_VPS, SKYVECTOR_EDITION, this);
    if (storage && conn) {
        conn->SetSaveStorage(storage);
        storage->SetNextLoadStorage(conn);
    }
}

void SkyVectorVFRProvider::FetchTile(TilePtr tile) {
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

std::string SkyVectorVFRProvider::GetCacheDir() const {
    return cacheDir;
}