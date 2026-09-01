#include <M5Unified.h>

extern const char *INSTRUMENT_NAMES[];
const char TEXT_FACTOR = 8;
const char LOOP_DELAY = 1;

unsigned char g_tone = 0;
unsigned char g_volume = 63;
bool g_is_touched = false;
M5Canvas g_canvas;

void paint()
{
    int width = g_canvas.width();
    int height = g_canvas.height();

    g_canvas.clearDisplay(TFT_BLACK);

    g_canvas.setCursor(0, 0);
    g_canvas.setTextSize(4);
    g_canvas.printf("tone: %i\n", g_tone + 1);
    g_canvas.setCursor(0, TEXT_FACTOR * 5);
    g_canvas.setTextSize(2);
    g_canvas.printf("%s\n", INSTRUMENT_NAMES[g_tone]);
    g_canvas.drawFastHLine(0, TEXT_FACTOR * 7, width, TFT_WHITE);

    g_canvas.setCursor(0, TEXT_FACTOR * 8);
    g_canvas.setTextSize(4);
    g_canvas.printf("volume: %i\n", g_volume);
    g_canvas.drawFastHLine(0, TEXT_FACTOR * 15, width, TFT_WHITE);

    M5.Display.startWrite();
    g_canvas.pushSprite(&M5.Display, 0, 0);
    M5.Display.endWrite();
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    g_canvas.createSprite(M5.Display.width(), M5.Display.height());
    paint();
}

void loop()
{
    M5.update();
    int display_width = M5.Display.width();
    int display_height = M5.Display.height();

    if (g_is_touched)
    {
        if (M5.Touch.getCount() == 0)
            g_is_touched = false;
        delay(LOOP_DELAY);
        return;
    }

    g_is_touched = M5.Touch.getCount() != 0;
    if (!g_is_touched)
        return;

    for (char i = 0; i < M5.Touch.getCount(); i++)
    {
        auto touch = M5.Touch.getDetail(i);

        if (0 <= touch.y && touch.y < TEXT_FACTOR * 8)
        {
            if (0 <= touch.x && touch.x < 64)
                g_tone--;
            if (display_width - 64 <= touch.x && touch.x < display_width)
                g_tone++;
        }

        if (TEXT_FACTOR * 8 <= touch.y && touch.y < TEXT_FACTOR * 16)
        {
            if (0 <= touch.x && touch.x < 64)
                g_volume--;
            if (display_width - 64 <= touch.x && touch.x < display_width)
                g_volume++;
        }
    }

    if (192 <= g_tone && g_tone < 256)
        g_tone = 127;
    if (128 <= g_tone && g_tone < 192)
        g_tone = 0;

    if (192 <= g_volume && g_volume < 256)
        g_volume = 0;
    if (128 <= g_volume && g_volume < 192)
        g_volume = 127;

    paint();
    delay(LOOP_DELAY);
}
