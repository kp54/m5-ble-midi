#include <M5_SAM2695.h>
#include <BLEMidi.h>

extern int g_ble_index;
extern M5_SAM2695 g_midi;
extern signed char g_transpose;

void onConnect()
{
    Serial.printf("Connected name=%s mac=%s\r\n", BLEMidiClient.deviceName(g_ble_index), BLEMidiClient.deviceMacAddress(g_ble_index).c_str());
}

void onDisconnect()
{
    Serial.printf("Disconnected\r\n");
}

void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
    uint8_t transposed = note + g_transpose;
    if (0 < transposed)
        g_midi.setNoteOn(channel, transposed, velocity);

    Serial.printf("NoteOn channel=%d note=%d velocity=%d timestamp=%d\r\n", channel, note, velocity, timestamp);
}

void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
{
    uint8_t transposed = note + g_transpose;
    if (0 < transposed)
        g_midi.setNoteOff(channel, transposed, velocity);

    Serial.printf("NoteOff channel=%d note=%d velocity=%d timestamp=%d\r\n", channel, note, velocity, timestamp);
}

void onAfterTouch(uint8_t channel, uint8_t pressure, uint16_t timestamp)
{
    Serial.printf("AfterTouch channel=%d pressure=%d timestamp=%d\r\n", channel, pressure, timestamp);
}

void onAfterTouchPoly(uint8_t channel, uint8_t note, uint8_t pressure, uint16_t timestamp)
{
    Serial.printf("AfterTouchPoly channel=%d note=%d pressure=%d timestamp=%d\r\n", channel, note, pressure, timestamp);
}

void onPitchBend(uint8_t channel, uint16_t value, uint16_t timestamp)
{
    g_midi.setPitchBend(channel, value);
    Serial.printf("PitchBend channel=%d value=%d timestamp=%d\r\n", channel, value, timestamp);
}

void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp)
{
    switch (controller)
    {
        case 11:
            g_midi.setExpression(channel, value);
            break;
    }

    Serial.printf("ControlChange channel=%d controller=%d value=%d timestamp=%d\r\n", channel, controller, value, timestamp);
}

void onProgramChange(uint8_t channel, uint8_t program, uint16_t timestamp)
{
    Serial.printf("ProgramChange channel=%d program=%d timestamp=%d\r\n", channel, program, timestamp);
}

void setup_ble_midi_callbacks()
{
    BLEMidiClient.setOnConnectCallback(onConnect);
    BLEMidiClient.setOnDisconnectCallback(onDisconnect);

    BLEMidiClient.setNoteOnCallback(onNoteOn);
    BLEMidiClient.setNoteOffCallback(onNoteOff);

    BLEMidiClient.setAfterTouchCallback(onAfterTouch);
    BLEMidiClient.setAfterTouchPolyCallback(onAfterTouchPoly);

    BLEMidiClient.setPitchBendCallback(onPitchBend);

    BLEMidiClient.setControlChangeCallback(onControlChange);
    BLEMidiClient.setProgramChangeCallback(onProgramChange);
}
