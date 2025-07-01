#pragma hdrstop
#include "SpatialGrid.h"

SpatialGrid::SpatialGrid() {}
void SpatialGrid::clear() {
    for (int x = 0; x < GRID_WIDTH; ++x) {
        for (int y = 0; y < GRID_HEIGHT; ++y) {
            FGrid[x][y].clear();
        }
    }
}
void SpatialGrid::getGridIndices(const TADS_B_Aircraft* aircraft, int& x, int& y) {
    // 5도 해상도로 설정 (성능과 정확도의 균형)
    x = static_cast<int>((aircraft->Longitude + 180.0) / 5.0);
    y = static_cast<int>((aircraft->Latitude + 90.0) / 5.0);
    if (x < 0) x = 0; if (x >= GRID_WIDTH) x = GRID_WIDTH - 1;
    if (y < 0) y = 0; if (y >= GRID_HEIGHT) y = GRID_HEIGHT - 1;
}
void SpatialGrid::add(TADS_B_Aircraft* aircraft) {
    if (!aircraft->HaveLatLon) return;
    int x, y;
    getGridIndices(aircraft, x, y);
    FGrid[x][y].push_back(aircraft);
}
std::vector<TADS_B_Aircraft*> SpatialGrid::getNearbyAircraft(const TADS_B_Aircraft* aircraft) {
    std::vector<TADS_B_Aircraft*> nearby;
    if (!aircraft->HaveLatLon) return nearby;
    int centerX, centerY;
    getGridIndices(aircraft, centerX, centerY);
    for (int x = centerX - 1; x <= centerX + 1; ++x) {
        for (int y = centerY - 1; y <= centerY + 1; ++y) {
            int currentX = (x + GRID_WIDTH) % GRID_WIDTH;
            if (y < 0 || y >= GRID_HEIGHT) continue;
            nearby.insert(nearby.end(), FGrid[currentX][y].begin(), FGrid[currentX][y].end());
        }
    }
    return nearby;
}