#include "hex_font.h"
#include <cstring>
#include <windows.h>

bool BuildConsolasHexAtlas(unsigned char atlas[HEX_FONT_HEIGHT][HEX_FONT_WIDTH * HEX_FONT_GLYPHS])
{
    HFONT font = CreateFontA(HEX_FONT_HEIGHT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, FF_DONTCARE, "Consolas");
    if (!font)
        return false;

    HDC hdc = CreateCompatibleDC(NULL);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = HEX_FONT_WIDTH * HEX_FONT_GLYPHS;
    bmi.bmiHeader.biHeight = HEX_FONT_HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    unsigned char* bits = nullptr;
    HBITMAP dib = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    HGDIOBJ oldBmp = SelectObject(hdc, dib);
    HGDIOBJ oldFont = SelectObject(hdc, font);

    RECT rc = {0, 0, HEX_FONT_WIDTH * HEX_FONT_GLYPHS, HEX_FONT_HEIGHT};
    HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(hdc, &rc, black);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255,255,255));

    const char glyphs[HEX_FONT_GLYPHS] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    for (int i = 0; i < HEX_FONT_GLYPHS; ++i)
    {
        TextOutA(hdc, i * HEX_FONT_WIDTH, 0, &glyphs[i], 1);
    }

    for (int y = 0; y < HEX_FONT_HEIGHT; ++y)
    {
        for (int x = 0; x < HEX_FONT_WIDTH * HEX_FONT_GLYPHS; ++x)
        {
            atlas[y][x] = bits[(HEX_FONT_HEIGHT - 1 - y) * (HEX_FONT_WIDTH * HEX_FONT_GLYPHS) * 4 + x * 4];
        }
    }

    SelectObject(hdc, oldFont);
    SelectObject(hdc, oldBmp);
    DeleteObject(dib);
    DeleteDC(hdc);
    DeleteObject(font);
    return true;
}
