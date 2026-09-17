#include <M5Unified.h>
#include <M5_SAM2695.h>
#include <BLEMidi.h>
#include <Preferences.h>

extern const char *INSTRUMENT_NAMES[];
extern void setup_ble_midi_callbacks();

const char TEXT_FACTOR = 8;
const char LOOP_DELAY = 1;

Preferences g_preferences;
M5Canvas g_canvas;
bool g_is_touched = false;

M5_SAM2695 g_midi;
unsigned char g_tone = 0;
unsigned char g_volume = 63;

bool g_ble_do_scan = false;
bool g_ble_do_connect = false;
int g_ble_index = 0;
int g_ble_conn_index = -1;
int g_ble_devices = 0;

void paint_ble(int width)
{
    g_canvas.drawFastHLine(0, TEXT_FACTOR * 23, width, TFT_WHITE);
    g_canvas.setCursor(0, TEXT_FACTOR * 16);
    g_canvas.setTextSize(4);

    if (g_ble_do_scan == true)
    {
        g_canvas.printf("device: scan\n");
        return;
    }

    if (g_ble_devices == 0)
    {
        g_canvas.printf("device: n/a\n");
        return;
    }

    if (g_ble_do_connect == true)
        g_canvas.printf("device: conn\n");
    else
        g_canvas.printf("device: %i/%i\n", g_ble_index + 1, g_ble_devices);

    g_canvas.setCursor(0, TEXT_FACTOR * 21);
    g_canvas.setTextSize(2);

    if (BLEMidiClient.isConnected() && g_ble_index == g_ble_conn_index)
        g_canvas.printf("%s [conn]\n", BLEMidiClient.deviceName(g_ble_index));
    else
       g_canvas.printf("%s\n", BLEMidiClient.deviceName(g_ble_index));
}

void paint_bat(int width)
{
    g_canvas.setCursor(width - 36, 0);
    g_canvas.setTextSize(2);

    int charge_status = M5.Power.Axp2101.getChargeStatus();
    if (charge_status == 1)
        g_canvas.setTextColor(TFT_GREEN);
    if (charge_status == -1)
        g_canvas.setTextColor(TFT_RED);

    g_canvas.printf("%03d", M5.Power.getBatteryLevel());

    g_canvas.setTextColor(TFT_WHITE);
}

void paint()
{
    int width = g_canvas.width();
    int height = g_canvas.height();

    g_canvas.clearDisplay(TFT_BLACK);
    g_canvas.setTextColor(TFT_WHITE);

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

    paint_ble(width);

    paint_bat(width);

    M5.Display.startWrite();
    g_canvas.pushSprite(&M5.Display, 0, 0);
    M5.Display.endWrite();
}

void apply_values()
{
    g_midi.setInstrument(0, 0, g_tone);
    g_midi.setMasterVolume(g_volume);

    g_preferences.putUChar("g_tone", g_tone);
    g_preferences.putUChar("g_volume", g_volume);
}

bool handle_touch()
{
    int display_width = M5.Display.width();
    int display_height = M5.Display.height();

    if (g_is_touched)
    {
        if (M5.Touch.getCount() == 0)
            g_is_touched = false;
        return false;
    }

    g_is_touched = M5.Touch.getCount() != 0;
    if (!g_is_touched)
        return false;

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

        if (g_ble_do_scan == false && g_ble_do_connect == false && TEXT_FACTOR * 16 <= touch.y && touch.y < TEXT_FACTOR * 24)
        {
            if (0 <= touch.x && touch.x < 64)
                g_ble_do_scan = true;
            if (64 <= touch.x && touch.x < display_width - 64)
                g_ble_do_connect = true;
            if (display_width - 64 <= touch.x && touch.x < display_width)
                g_ble_index++;
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

    if (g_ble_devices <= g_ble_index)
        g_ble_index = 0;

    return true;
}

void handle_ble_scan()
{
    if (!g_ble_do_scan)
        return;

    g_ble_devices = 0;
    g_ble_index = 0;
    g_ble_conn_index = -1;

    g_ble_devices = BLEMidiClient.scan();
    g_ble_do_scan = false;
}

void handle_ble_connect()
{
    if (g_ble_devices == 0 || g_ble_do_connect == false)
        return;

    BLEMidiClient.connect(g_ble_index);
    g_ble_conn_index = g_ble_index;
    g_ble_do_connect = false;
}

void loop()
{
    M5.update();

    if (handle_touch())
    {
        apply_values();
    }

    paint();
    delay(LOOP_DELAY);
}

void task_ble(void *_)
{
    while (true)
    {
        handle_ble_scan();
        handle_ble_connect();
        delay(1);
    }
}

void setup()
{
    g_preferences.begin("ble-midi", false);
    g_tone = g_preferences.getUChar("g_tone", 0);
    g_volume = g_preferences.getUChar("g_volume", 63);

    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Power.begin();
    Serial.begin(115200);

    g_canvas.createSprite(M5.Display.width(), M5.Display.height());
    g_midi.begin(&Serial2, MIDI_BAUD, 16, 17);

    setup_ble_midi_callbacks();
    BLEMidiClient.begin("kp54");

    apply_values();
    paint();
    xTaskCreatePinnedToCore(task_ble, "task_ble", 2048, NULL, 1, NULL, 1);
}
