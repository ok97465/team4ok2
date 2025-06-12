//---------------------------------------------------------------------------

#ifndef KeyholeConnectionH
#define KeyholeConnectionH
#include <gefetch.h>
#include <string>
#include "IMAPProvider.h"
#include "SimpleTileStorage.h"
/*
#define GoogleMaps             0
#define SkyVector_VFR          1
#define SkyVector_IFR_Low      2
#define SkyVector_IFR_High     3
#define SkyVector              4

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
	// Google Maps용 생성자
    KeyholeConnection(const std::string& url, IMAPProvider* provider);
    // SkyVector 등 추가 정보가 필요한 지도용 생성자
    KeyholeConnection(const std::string& url, const std::string& key, const std::string& chart, const std::string& edition, IMAPProvider* provider);
    ~KeyholeConnection();

    gefetch_t GetGEFetch() const { return m_GEFetch; }
    const std::string& GetKey() const { return key; }
    const std::string& GetChart() const { return chart; }
    const std::string& GetEdition() const { return edition; }
    bool HasSaveStorage() const { return false; } // 필요시 구현
    // SimpleTileStorage 인터페이스 구현
    void Process(TilePtr tile) override;

private:
    gefetch_t	m_GEFetch;
    std::string url;
    std::string key, chart, edition;
    IMAPProvider* provider; // provider 포인터 보관	
/*	
	KeyholeConnection(int Type);

	virtual ~KeyholeConnection();

protected:

	void Process(TilePtr tile);

private:
	gefetch_t	m_GEFetch;
	int         ServerType;
	const char  *Key;
	const char  *Chart;
	const char  *Edition;
*/	
};

//---------------------------------------------------------------------------
#endif
