#include "splash.h"

#include "splash_logo.h"
#include <nds.h>
#include <string.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define COLOR(r, g, b) ((u16)(RGB15((r), (g), (b)) | BIT(15)))

// The splash is shown for at least this long (any key or a touch skips ahead)
#define SPLASH_HOLD_TICKS (BUS_CLOCK * 5 / 2)

static const u16 cream = COLOR(31, 29, 23);
static const u16 dim = COLOR(20, 19, 15);
static const u16 black = COLOR(0, 0, 0);

static u16* topBitmap;
static u16* bottomBitmap;

static const u8 fontRows[36][7] = {
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30},
    {14, 17, 16, 16, 16, 17, 14}, {30, 17, 17, 17, 17, 17, 30},
    {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
    {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17},
    {14, 4, 4, 4, 4, 4, 14}, {7, 2, 2, 2, 2, 18, 12},
    {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
    {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17},
    {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16},
    {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
    {15, 16, 16, 14, 1, 1, 30}, {31, 4, 4, 4, 4, 4, 4},
    {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10, 4},
    {17, 17, 17, 21, 21, 21, 10}, {17, 17, 10, 4, 10, 17, 17},
    {17, 17, 10, 4, 4, 4, 4}, {31, 1, 2, 4, 8, 16, 31},
    {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},
    {14, 17, 1, 2, 4, 8, 31}, {30, 1, 1, 14, 1, 1, 30},
    {2, 6, 10, 18, 31, 2, 2}, {31, 16, 16, 30, 1, 1, 30},
    {14, 16, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8},
    {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 1, 14},
};

static void fillRect(u16* bitmap, int x, int y, int width, int height, u16 color)
{
    for (int row = y; row < y + height; row++) {
        u16* pixel = bitmap + row * SCREEN_WIDTH + x;
        for (int column = 0; column < width; column++)
            pixel[column] = color;
    }
}

static const u8* getGlyph(char character, u8 punctuation[7])
{
    if (character >= 'a' && character <= 'z')
        character -= 'a' - 'A';
    if (character >= 'A' && character <= 'Z')
        return fontRows[character - 'A'];
    if (character >= '0' && character <= '9')
        return fontRows[26 + character - '0'];

    memset(punctuation, 0, 7);
    return punctuation;
}

static void drawText(u16* bitmap, int x, int y, const char* text, u16 color, int scale)
{
    while (*text != '\0') {
        u8 punctuation[7];
        const u8* glyph = getGlyph(*text++, punctuation);
        for (int row = 0; row < 7; row++) {
            for (int column = 0; column < 5; column++) {
                if ((glyph[row] & (1 << (4 - column))) == 0)
                    continue;
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++)
                        bitmap[(y + row * scale + dy) * SCREEN_WIDTH + x + column * scale + dx] = color;
                }
            }
        }
        x += 6 * scale;
        if (x >= SCREEN_WIDTH - 5 * scale)
            break;
    }
}

static void drawCenteredText(u16* bitmap, int y, const char* text, u16 color, int scale)
{
    int width = (int)strlen(text) * 6 * scale - scale;
    drawText(bitmap, (SCREEN_WIDTH - width) / 2, y, text, color, scale);
}

// The lairLynx Studios logo in cream on black, with the credit below it
static void drawTopScreen(void)
{
    fillRect(topBitmap, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, black);

    int left = (SCREEN_WIDTH - SPLASH_LOGO_WIDTH) / 2;
    for (int row = 0; row < SPLASH_LOGO_HEIGHT; row++) {
        for (int column = 0; column < SPLASH_LOGO_WIDTH; column++) {
            unsigned coverage = splashLogo[row * SPLASH_LOGO_WIDTH + column];
            if (coverage < 8)
                continue;
            topBitmap[(12 + row) * SCREEN_WIDTH + left + column] = COLOR(31 * coverage / 255, 29 * coverage / 255, 23 * coverage / 255);
        }
    }

    drawCenteredText(topBitmap, 128, "MADE BY", dim, 1);
    drawCenteredText(topBitmap, 142, "LAIRLYNX STUDIOS", cream, 2);
}

static void drawBottomScreen(void)
{
    fillRect(bottomBitmap, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, black);
    drawCenteredText(bottomBitmap, 88, "MADE USING BLOCKSDS", cream, 2);
}

static void fade(bool toBlack)
{
    for (int level = 1; level <= 16; level++) {
        setBrightness(3, toBlack ? -level : level - 16);
        swiWaitForVBlank();
    }
}

void screensFadeIn(void)
{
    fade(false);
}

void splashBegin(void)
{
    setBrightness(3, -16); // stay black until the splash is drawn

    videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG2_ACTIVE);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    lcdMainOnTop();

    int topBackground = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    int bottomBackground = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    topBitmap = (u16*)bgGetGfxPtr(topBackground);
    bottomBitmap = (u16*)bgGetGfxPtr(bottomBackground);

    drawTopScreen();
    drawBottomScreen();

    swiWaitForVBlank();
    swiWaitForVBlank();
    fade(false);

    // Time the splash from the moment it is fully visible
    cpuStartTiming(0);
}

void splashEnd(void)
{
    while (cpuGetTiming() < SPLASH_HOLD_TICKS) {
        swiWaitForVBlank();
        scanKeys();
        if (keysDown() & (KEY_A | KEY_B | KEY_START | KEY_TOUCH))
            break;
    }
    cpuEndTiming();

    fade(true);
}
