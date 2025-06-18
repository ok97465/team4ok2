#pragma once

#include "IMAPProvider.h"
#include "KeyholeConnection.h"
#include "FilesystemStorage.h"

class OpenStreetMapProvider : public IMAPProvider {
    std::string cacheDir;
    bool inet; // 인터넷에서 가져오는지 여부
public:
    OpenStreetMapProvider(const std::string& dir, bool inet = true);
    ~OpenStreetMapProvider() override;
    void FetchTile(TilePtr tile, KeyholeConnection* conn) override;
    std::string GetCacheDir() const override;
    std::string GetURI() const override;
};