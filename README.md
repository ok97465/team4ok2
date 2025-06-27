"# team4" 

# ADS-B Display System

## 항공기 아이콘 및 리더 수정 가이드

### 1. 항공기 아이콘 시스템

#### 현재 구조
- **스프라이트 파일**: `Symbols/sprites-RGBA.png`
- **그리드 구성**: 11x8 = 88개의 스프라이트
- **스프라이트 크기**: 72x72 픽셀
- **로딩 함수**: `ntds2d.cpp`의 `MakeAirplaneImages()`

#### 아이콘 선택 로직
새로 추가된 `SelectAircraftIcon()` 함수는 다음 기준으로 아이콘을 선택합니다:

```cpp
int SelectAircraftIcon(const TADS_B_Aircraft* aircraft) 
{
    // 헬리콥터: 인덱스 1
    // 군용기: 인덱스 2  
    // 고고도 항공기 (>30,000ft): 인덱스 3
    // 저고도 항공기 (<10,000ft): 인덱스 4
    // 고속 항공기 (>500kts): 인덱스 5
    // 저속 항공기 (<100kts): 인덱스 6
    // 기본: 인덱스 0
}
```

#### 새로운 아이콘 추가 방법

1. **스프라이트 이미지 준비**:
   - 72x72 픽셀 크기
   - RGBA 형식 (투명도 지원)
   - PNG 파일로 저장

2. **스프라이트 시트에 추가**:
   - `Symbols/sprites-RGBA.png` 파일을 편집
   - 11x8 그리드의 빈 슬롯에 새 아이콘 배치
   - 인덱스 번호 확인

3. **선택 로직 수정**:
   - `SelectAircraftIcon()` 함수에서 새로운 조건 추가
   - 적절한 인덱스 반환

### 2. 리더(Leader) 시스템

#### 현재 구조
- **기본 함수**: `DrawLeader()` - 단순 직선
- **배치 렌더링**: `DrawAirplaneLinesInstanced()`
- **구조체**: `AirplaneLineInstance`

#### 새로운 리더 스타일

다음 함수들이 추가되었습니다:

```cpp
// 화살표가 있는 리더
void DrawLeaderArrow(float x1, float y1, float x2, float y2, float arrowSize = 8.0f);

// 점선 리더  
void DrawLeaderDashed(float x1, float y1, float x2, float y2);

// 두꺼운 선 리더
void DrawLeaderThick(float x1, float y1, float x2, float y2, float thickness = 3.0f);
```

#### 리더 스타일 적용 방법

1. **DisplayGUI.cpp 수정**:
   ```cpp
   // 기존 코드
   DrawAirplaneLinesInstanced(m_lineBatch);
   
   // 새로운 스타일 적용
   for (const auto& line : m_lineBatch) {
       DrawLeaderArrow(line.x1, line.y1, line.x2, line.y2);
   }
   ```

2. **조건부 스타일 적용**:
   ```cpp
   // 항공기 타입에 따른 다른 리더 스타일
   if (aircraft_is_military(Data->ICAO, NULL)) {
       DrawLeaderThick(line.x1, line.y1, line.x2, line.y2, 4.0f);
   } else {
       DrawLeaderArrow(line.x1, line.y1, line.x2, line.y2);
   }
   ```

### 3. 커스터마이징 예제

#### 항공기 타입별 색상 변경
```cpp
// DisplayGUI.cpp에서
if (aircraft_is_helicopter(Data->ICAO, NULL)) {
    color[0] = 0.0f; color[1] = 1.0f; color[2] = 0.0f; // 녹색
} else if (aircraft_is_military(Data->ICAO, NULL)) {
    color[0] = 1.0f; color[1] = 0.0f; color[2] = 0.0f; // 빨간색
} else {
    color[0] = 1.0f; color[1] = 1.0f; color[2] = 1.0f; // 흰색
}
```

#### 고도에 따른 크기 조정
```cpp
if (Data->HaveAltitude) {
    if (Data->Altitude > 30000) {
        inst.scale = 1.0f; // 고고도는 작게 (멀리 있어서)
    } else if (Data->Altitude < 10000) {
        inst.scale = 2.0f; // 저고도는 크게 (가까이 있어서)
    } else {
        inst.scale = 1.5f; // 중간 고도는 기본 크기
    }
}
```

### 4. 성능 최적화

#### 배치 렌더링 활용
- `m_planeBatch`, `m_lineBatch` 벡터 사용
- `DrawAirplaneImagesInstanced()` 함수로 한 번에 렌더링
- 개별 `glBegin/glEnd` 호출 최소화

#### 조건부 렌더링
```cpp
// 화면 밖 항공기는 렌더링하지 않음
if (ScrX >= -100 && ScrX <= ClientWidth + 100 && 
    ScrY >= -100 && ScrY <= ClientHeight + 100) {
    m_planeBatch.push_back(inst);
}
```

### 5. 디버깅 및 테스트

#### 아이콘 인덱스 확인
```cpp
// 디버그 정보 출력
printf("Aircraft %s: Icon=%d, Alt=%.0f, Speed=%.0f\n", 
       Data->HexAddr, inst.imageNum, Data->Altitude, Data->Speed);
```

#### 시각적 테스트
- 다양한 항공기 타입으로 테스트
- 고도/속도 범위별 아이콘 확인
- 리더 스타일 가시성 검증

---

## 기존 README 내용 
