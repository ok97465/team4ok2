#pragma once

#include "IMAPProvider.h"
#include "KeyholeConnection.h"
#include "FilesystemStorage.h"

class SkyVectorVFRProvider : public IMAPProvider {
    std::string cacheDir;
    bool inet; // 인터넷에서 가져오는지 여부    
public:
    SkyVectorVFRProvider(const std::string& dir, bool inet = true);
    ~SkyVectorVFRProvider() override;
    void FetchTile(TilePtr tile, KeyholeConnection* conn) override;
    std::string GetCacheDir() const override;
    std::string GetURI() const override;    
};