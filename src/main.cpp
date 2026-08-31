#include <M5Unified.h>

extern const char* INSTRUMENT_NAMES[];
const char TEXT_FACTOR = 8;

char g_tone = 0;
M5Canvas canvas;

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    canvas.createSprite(M5.Display.width(), M5.Display.height());
}

void paint(char tone)
{
    canvas.clearDisplay(TFT_BLACK);
    canvas.setCursor(0, 0);

    canvas.setTextSize(4);
    canvas.printf("tone: %i\n", tone + 1);

    canvas.setCursor(0, TEXT_FACTOR * (4 + 1));
    canvas.setTextSize(3);
    canvas.printf("%s\n", INSTRUMENT_NAMES[tone]);
    canvas.drawFastHLine(0, TEXT_FACTOR * 8, canvas.width(), TFT_WHITE);

    M5.Display.startWrite();
    canvas.pushSprite(&M5.Display, 0, 0);
    M5.Display.endWrite();
}

void loop()
{
    paint(g_tone);

    g_tone++;
    if (127 < g_tone)
        g_tone = 0;

    delay(100);
}
