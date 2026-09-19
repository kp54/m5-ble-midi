#include <M5Unified.h>
#include <M5_SAM2695.h>
#include <BLEMidi.h>
#include <Preferences.h>

extern const char *INSTRUMENT_NAMES[];
extern void setup_ble_midi_callbacks();

const unsigned short DISPLAY_WIDTH = 320;
const unsigned short DISPLAY_HEIGHT = 240;
const unsigned short SLOT_HEIGHT = 60;
const unsigned short TEXT_FACTOR = 8;

const unsigned char ACTIVE_TICKS = 30;
const unsigned char LOOP_DELAY_MS = 1;

Preferences g_preferences;
M5Canvas g_canvas;
unsigned char g_active_ticks = 0;
bool g_is_touched = false;

M5_SAM2695 g_midi;
unsigned char g_tone = 0;
signed char g_transpose = 0;
unsigned char g_volume = 63;

bool g_ble_do_scan = false;
bool g_ble_do_connect = false;
int g_ble_index = 0;
int g_ble_conn_index = -1;
int g_ble_devices = 0;
char g_ble_last_device[18] = "\0";

#define SLOT_TOP(s) (SLOT_HEIGHT * (s))
#define SLOT_BOTTOM(s) (SLOT_HEIGHT * ((s) + 1) - 1)
#define SLOT_TEXT(s, i) (SLOT_HEIGHT * (s) + TEXT_FACTOR * (i) + TEXT_FACTOR / 2)

#define IS_IN_SLOT(s, y) (SLOT_TOP((s)) <= (y) && (y) < SLOT_TOP((s) + 1))
#define IS_IN_LEFT(x) (0 <= (x) && (x) < 64)
#define IS_IN_CENTER(x) (64 <= (x) && (x) < DISPLAY_WIDTH - 64)
#define IS_IN_RIGHT(x) (DISPLAY_WIDTH - 64 <= (x) && (x) < DISPLAY_WIDTH)

void paint_ble(int fg_color)
{
    g_canvas.drawFastHLine(0, SLOT_BOTTOM(3), DISPLAY_WIDTH, fg_color);
    g_canvas.setCursor(0, SLOT_TEXT(3, 0));
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

    g_canvas.setCursor(0, SLOT_TEXT(3, 5));
    g_canvas.setTextSize(2);

    if (BLEMidiClient.isConnected() && g_ble_index == g_ble_conn_index)
        g_canvas.printf("%s [conn]\n", BLEMidiClient.deviceName(g_ble_index));
    else
        g_canvas.printf("%s\n", BLEMidiClient.deviceName(g_ble_index));
}

void paint_bat(int fg_color)
{
    g_canvas.setCursor(DISPLAY_WIDTH - 36, 0);
    g_canvas.setTextSize(2);

    int charge_status = M5.Power.Axp2101.getChargeStatus();
    if (charge_status == 1)
        g_canvas.setTextColor(TFT_GREEN);
    if (charge_status == -1)
        g_canvas.setTextColor(TFT_RED);

    g_canvas.printf("%03d", M5.Power.getBatteryLevel());

    g_canvas.setTextColor(fg_color);
}

void paint()
{
    int fg_color = 0 < g_active_ticks ? TFT_GREEN : TFT_WHITE;

    g_canvas.clearDisplay(TFT_BLACK);
    g_canvas.setTextColor(fg_color);

    g_canvas.setCursor(0, SLOT_TEXT(0, 0));
    g_canvas.setTextSize(4);
    g_canvas.printf("tone: %i\n", g_tone + 1);
    g_canvas.setCursor(0, SLOT_TEXT(0, 5));
    g_canvas.setTextSize(2);
    g_canvas.printf("%s\n", INSTRUMENT_NAMES[g_tone]);
    g_canvas.drawFastHLine(0, SLOT_BOTTOM(0), DISPLAY_WIDTH, fg_color);

    g_canvas.setCursor(0, SLOT_TEXT(1, 0));
    g_canvas.setTextSize(4);
    g_canvas.printf("transp: %d\n", g_transpose);
    g_canvas.drawFastHLine(0, SLOT_BOTTOM(1), DISPLAY_WIDTH, fg_color);

    g_canvas.setCursor(0, SLOT_TEXT(2, 0));
    g_canvas.setTextSize(4);
    g_canvas.printf("volume: %i\n", g_volume);
    g_canvas.drawFastHLine(0, SLOT_BOTTOM(2), DISPLAY_WIDTH, fg_color);

    paint_ble(fg_color);

    paint_bat(fg_color);

    M5.Display.startWrite();
    g_canvas.pushSprite(&M5.Display, 0, 0);
    M5.Display.endWrite();
}

void apply_values()
{
    g_midi.setInstrument(0, 0, g_tone);
    g_midi.setMasterVolume(g_volume);
}

bool handle_touch()
{
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

        if (IS_IN_SLOT(0, touch.y))
        {
            if (IS_IN_LEFT(touch.x))
                g_tone--;
            if (IS_IN_RIGHT(touch.x))
                g_tone++;
        }

        if (IS_IN_SLOT(1, touch.y))
        {
            if (IS_IN_LEFT(touch.x))
                g_transpose--;
            if (IS_IN_RIGHT(touch.x))
                g_transpose++;
        }

        if (IS_IN_SLOT(2, touch.y))
        {
            if (IS_IN_LEFT(touch.x))
                g_volume--;
            if (IS_IN_RIGHT(touch.x))
                g_volume++;
        }

        if (IS_IN_SLOT(3, touch.y) && g_ble_do_scan == false && g_ble_do_connect == false)
        {
            if (IS_IN_LEFT(touch.x))
                g_ble_do_scan = true;
            if (IS_IN_CENTER(touch.x))
                g_ble_do_connect = true;
            if (IS_IN_RIGHT(touch.x))
                g_ble_index++;
        }
    }

    if (192 <= g_tone && g_tone < 256)
        g_tone = 127;
    if (128 <= g_tone && g_tone < 192)
        g_tone = 0;

    if (g_transpose < -12)
        g_transpose = -12;
    if (12 < g_transpose)
        g_transpose = 12;

    if (192 <= g_volume && g_volume < 256)
        g_volume = 0;
    if (128 <= g_volume && g_volume < 192)
        g_volume = 127;

    if (g_ble_devices <= g_ble_index)
        g_ble_index = 0;

    return true;
}

bool handle_save()
{
    if (!M5.BtnPWR.wasClicked())
        return false;

    g_preferences.putUChar("g_tone", g_tone);
    g_preferences.putChar("g_transpose", g_transpose);
    g_preferences.putUChar("g_volume", g_volume);
    g_preferences.putString("g_ble_last_dev", g_ble_last_device);

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

    for (int i = 0; i < g_ble_devices; i++)
    {
        auto mac = BLEMidiClient.deviceMacAddress(i).c_str();

        if (strcmp(g_ble_last_device, mac) == 0)
        {
            g_ble_index = i;
            g_ble_conn_index = i;
            BLEMidiClient.connect(i);
            break;
        }
    }

    g_ble_do_scan = false;
}

void handle_ble_connect()
{
    if (g_ble_devices == 0 || g_ble_do_connect == false)
        return;

    BLEMidiClient.connect(g_ble_index);
    g_ble_conn_index = g_ble_index;
    strcpy(g_ble_last_device, BLEMidiClient.deviceMacAddress(g_ble_index).c_str());
    g_ble_do_connect = false;
}

void loop()
{
    M5.update();

    if (handle_touch())
        apply_values();

    if (handle_save())
        g_active_ticks = ACTIVE_TICKS;

    paint();

    if (0 < g_active_ticks)
        g_active_ticks--;

    delay(LOOP_DELAY_MS);
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
    g_transpose = g_preferences.getChar("g_transpose", 0);
    g_volume = g_preferences.getUChar("g_volume", 63);
    g_preferences.getString("g_ble_last_dev", g_ble_last_device, 18);

    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Power.begin();
    Serial.begin(115200);

    g_canvas.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    g_midi.begin(&Serial2, MIDI_BAUD, 16, 17);

    setup_ble_midi_callbacks();
    BLEMidiClient.begin("kp54");

    apply_values();
    paint();
    xTaskCreatePinnedToCore(task_ble, "task_ble", 2048, NULL, 1, NULL, 1);
}
