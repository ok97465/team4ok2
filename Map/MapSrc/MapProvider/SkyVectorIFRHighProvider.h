#pragma once

#include "IMAPProvider.h"
#include "KeyholeConnection.h"
#include "FilesystemStorage.h"

class SkyVectorIFRHighProvider : public IMAPProvider {
    std::string cacheDir;
    bool inet; // 인터넷에서 가져오는지 여부    
public:
    SkyVectorIFRHighProvider(const std::string& dir, bool inet = true);
    ~SkyVectorIFRHighProvider() override;
    void FetchTile(TilePtr tile, KeyholeConnection* conn) override;
    std::string GetCacheDir() const override;
    std::string GetURI() const override; 
};
