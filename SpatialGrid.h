#ifndef SpatialGridH
#define SpatialGridH

#include <vector>
#include "Aircraft.h"

class SpatialGrid {
private:
    static const int GRID_WIDTH = 72;   // 5도 해상도 (360/5)
    static const int GRID_HEIGHT = 36;  // 5도 해상도 (180/5)
    std::vector<TADS_B_Aircraft*> FGrid[GRID_WIDTH][GRID_HEIGHT];
    void getGridIndices(const TADS_B_Aircraft* aircraft, int& x, int& y);
public:
    SpatialGrid();
    void clear();
    void add(TADS_B_Aircraft* aircraft);
    std::vector<TADS_B_Aircraft*> getNearbyAircraft(const TADS_B_Aircraft* aircraft);
};
#endif