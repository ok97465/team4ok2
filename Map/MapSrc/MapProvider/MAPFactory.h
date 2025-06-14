#pragma once

#include <memory>
#include <string>
#include "IMAPProvider.h"
#include "GoogleMapsProvider.h"
#include "SkyVectorVFRProvider.h"
#include "SkyVectorIFRLowProvider.h"
#include "SkyVectorIFRHighProvider.h"

enum MapType {
    GoogleMaps,
    SkyVector_VFR,
    SkyVector_IFR_Low,
    SkyVector_IFR_High
};

class MAPFactory {
public:
    static std::unique_ptr<IMAPProvider> Create(MapType type, const std::string& baseDir, bool fromInternet) {
        std::string dir = baseDir;
        switch (type) {
            case GoogleMaps:
                dir += "..\\GoogleMap";
                dir += fromInternet ? "_Live\\" : "\\";
                return std::make_unique<GoogleMapsProvider>(dir, fromInternet);
            case SkyVector_VFR:
                dir += "..\\VFR_Map";
                dir += fromInternet ? "_Live\\" : "\\";
                return std::make_unique<SkyVectorVFRProvider>(dir, fromInternet);
            case SkyVector_IFR_Low:
                dir += "..\\IFR_Low_Map";
                dir += fromInternet ? "_Live\\" : "\\";
                return std::make_unique<SkyVectorIFRLowProvider>(dir, fromInternet);
            case SkyVector_IFR_High:
                dir += "..\\IFR_High_Map";
                dir += fromInternet ? "_Live\\" : "\\";
                return std::make_unique<SkyVectorIFRHighProvider>(dir, fromInternet);
            default:
                return nullptr;
        }
    }
};