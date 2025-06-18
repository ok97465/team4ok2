#pragma once

#include <memory>
#include <string>
#include "IMAPProvider.h"
#include "GoogleMapsProvider.h"
#include "SkyVectorVFRProvider.h"
#include "SkyVectorIFRLowProvider.h"
#include "SkyVectorIFRHighProvider.h"
#include "OpenStreetMapProvider.h"

enum MapType {
    GoogleMaps,
    SkyVector_VFR,
    SkyVector_IFR_Low,
    SkyVector_IFR_High,
    OpenStreetMap
};

class MAPFactory {
public:
    static std::unique_ptr<IMAPProvider> Create(MapType type, const std::string& baseDir, bool fromInternet) {
        std::string dir = baseDir;
        switch (type) {
            case GoogleMaps:
                return std::make_unique<GoogleMapsProvider>(dir, fromInternet);
            case SkyVector_VFR:
                return std::make_unique<SkyVectorVFRProvider>(dir, fromInternet);
            case SkyVector_IFR_Low:
                return std::make_unique<SkyVectorIFRLowProvider>(dir, fromInternet);
            case SkyVector_IFR_High:
                return std::make_unique<SkyVectorIFRHighProvider>(dir, fromInternet);
            case OpenStreetMap:
                return std::make_unique<OpenStreetMapProvider>(dir, fromInternet);
            default:
                return nullptr;
        }
    }
};