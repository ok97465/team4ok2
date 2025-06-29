//---------------------------------------------------------------------------


#pragma hdrstop

#include "KeyholeConnection.h"
#include "LogHandler.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

KeyholeConnection::KeyholeConnection(const std::string& url)
    : url(url), m_GEFetch(nullptr) {
    m_GEFetch = gefetch_init(url.c_str());
    if (!m_GEFetch)
        throw Exception("gefetch_init() failed");
}

KeyholeConnection::~KeyholeConnection() noexcept {
    if (m_GEFetch)
        gefetch_cleanup(m_GEFetch);
}

void KeyholeConnection::SetFetchTileCallback(std::function<void(TilePtr, KeyholeConnection*)> cb) {
	LOG_DEBUG_F(LogHandler::CAT_MAP, "[KeyholeConnection][%s]", __func__);
	fetchTileCallback = cb;
}    

void KeyholeConnection::Process(TilePtr tile) {
     if (fetchTileCallback) {
		LOG_DEBUG_F(LogHandler::CAT_MAP, "[KeyholeConnection][%s] tile x(%d), y(%d), level(%d)", __func__, tile->GetX(), tile->GetY(), tile->GetLevel());
		fetchTileCallback(tile, this); 
    } else {
		LOG_WARNING_F(LogHandler::CAT_MAP, "[KeyholeConnection][%s] callback is null for tile x(%d), y(%d), level(%d)", __func__, tile->GetX(), tile->GetY(), tile->GetLevel());
        tile->Null();
    }
}