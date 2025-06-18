//---------------------------------------------------------------------------

#ifndef KeyholeConnectionH
#define KeyholeConnectionH
#include <gefetch.h>
#include <string>
#include "SimpleTileStorage.h"
#include <functional>

/**
 * Connection to Google server.
 *
 * Handles and encapsulates all network stuff, authentification and
 * downloading of tiles from Google servers. Of course, only handles
 * loading of tiles.
 *
 * @deprecated should be restructured and merged to GoogleLayer
 */
class KeyholeConnection: public SimpleTileStorage {
public:
    KeyholeConnection(const std::string& url);
    ~KeyholeConnection();

    std::function<void(TilePtr, KeyholeConnection*)> fetchTileCallback;
    void SetFetchTileCallback(std::function<void(TilePtr, KeyholeConnection*)> cb);
    gefetch_t GetGEFetch() const { return m_GEFetch; }

private:
    gefetch_t	m_GEFetch;
    std::string url;
    void Process(TilePtr tile);
};


//---------------------------------------------------------------------------
#endif
