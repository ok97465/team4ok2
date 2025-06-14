#pragma once

#include <string>
#include "Tile.h"
#include "FilesystemStorage.h" // FilesystemStorage 사용을 위한 include 추가

class IMAPProvider {
public:
    virtual ~IMAPProvider() {}
    virtual void Init() = 0;
    virtual void FetchTile(TilePtr tile) = 0;
    virtual std::string GetCacheDir() const = 0;
    virtual void SetStorage(FilesystemStorage* s) = 0;
};