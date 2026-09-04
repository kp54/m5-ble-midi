#include <M5Unified.h>
#include <M5_SAM2695.h>
#include <BLEMidi.h>

extern const char *INSTRUMENT_NAMES[];
const char TEXT_FACTOR = 8;
const char LOOP_DELAY = 1;

unsigned char g_tone = 0;
unsigned char g_volume = 63;
bool g_is_touched = false;
bool g_do_ble_scan = false;
int g_ble_index = 0;
int g_ble_devices = 0;
M5Canvas g_canvas;
M5_SAM2695 g_midi;

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

    g_canvas.setCursor(0, TEXT_FACTOR * 16);
    g_canvas.setTextSize(4);
    if (g_ble_devices == 0)
    {
        g_canvas.printf("device: n/a\n");
    }
    else
    {
        g_canvas.printf("device: %i/%i\n", g_ble_index + 1, g_ble_devices);
        g_canvas.setCursor(0, TEXT_FACTOR * 21);
        g_canvas.setTextSize(2);
        g_canvas.println(BLEMidiClient.deviceName(g_ble_index));
    }
    g_canvas.drawFastHLine(0, TEXT_FACTOR * 23, width, TFT_WHITE);

    M5.Display.startWrite();
    g_canvas.pushSprite(&M5.Display, 0, 0);
    M5.Display.endWrite();
}

void apply_values()
{
    g_midi.setInstrument(0, 0, g_tone);
    g_midi.setMasterVolume(g_volume);
    if (0 < g_ble_devices)
    {
        BLEMidiClient.connect(g_ble_index);
    }
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

        if (TEXT_FACTOR * 16 <= touch.y && touch.y < TEXT_FACTOR * 24)
        {
            if (0 <= touch.x && touch.x < 64)
                g_do_ble_scan = true;
            if (display_width - 64 <= touch.x && touch.x < display_width)
                g_ble_index++;
            if (g_ble_devices <= g_ble_index)
                g_ble_index = 0;
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

    return true;
}

void handle_ble()
{
    if (!g_do_ble_scan)
        return;
    g_do_ble_scan = false;

    g_ble_index = 0;
    g_ble_devices = BLEMidiClient.scan();
    if (0 < g_ble_devices)
        BLEMidiClient.connect(0);
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    g_canvas.createSprite(M5.Display.width(), M5.Display.height());
    g_midi.begin(&Serial2, MIDI_BAUD, 16, 17);

    BLEMidiClient.setNoteOnCallback([](u8_t channel, u8_t note, u8_t velocity, u16_t timestamp)
                                    { Serial.printf("NoteOn: channel=%i note=%i velocity=%i timestamp=%i\r\n", channel, note, velocity, timestamp); });
    BLEMidiClient.setNoteOffCallback([](u8_t channel, u8_t note, u8_t velocity, u16_t timestamp)
                                     { Serial.printf("NoteOff: channel=%i note=%i velocity=%i timestamp=%i\r\n", channel, note, velocity, timestamp); });

    BLEMidiClient.setOnConnectCallback([]()
                                       { Serial.printf("connected\r\n"); });
    BLEMidiClient.setOnDisconnectCallback([]()
                                          { Serial.printf("disconnected\r\n"); });

    BLEMidiClient.begin("kp54");

    apply_values();
    paint();
}

void loop()
{
    M5.update();

    if (handle_touch())
    {
        apply_values();
        handle_ble();
        paint();
    }

    delay(LOOP_DELAY);
}
