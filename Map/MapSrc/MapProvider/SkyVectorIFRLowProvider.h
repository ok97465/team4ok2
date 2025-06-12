#pragma once

#include "IMAPProvider.h"
#include "KeyholeConnection.h"
#include "FilesystemStorage.h"

class SkyVectorIFRLowProvider : public IMAPProvider {
    std::string cacheDir;
    KeyholeConnection* conn;
    FilesystemStorage* storage; // 외부에서 주입
    bool inet; // 인터넷에서 가져오는지 여부    
public:
    SkyVectorIFRLowProvider(const std::string& dir, bool inet = true);
    ~SkyVectorIFRLowProvider() override;
    void Init() override;
    void FetchTile(TilePtr tile) override;
    std::string GetCacheDir() const override;
    void SetStorage(FilesystemStorage* s); // 추가
    KeyholeConnection* GetKeyholeConnection() const { return conn; }    
};