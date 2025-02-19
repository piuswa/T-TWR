#include "LilyGo_TWR.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>
#include "fix_fft.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int ADC1_CHANNEL = 1;
int RCV_CHANNEL = 2;
int array_size = 2000;
int i = 0;
int* recieved;
int8_t* recieved_scaled;
int8_t* bit;
bool recieving = false;
int minValue = 5000;
int maxValue = 0;


void scaleTo8Bit(const int* inputArray, int8_t* outputArray, int size, int minValue, int maxValue) {
    // Scale each value from the input array
    for (int i = 0; i < size; ++i) {
        // Normalize the value in the range [0, 1]
        double normalized = static_cast<double>(inputArray[i] - minValue) / (maxValue - minValue);

        // Scale the value to the range [-128, 127]
        double scaled = normalized * 255.0 - 128.0;

        // Round and clamp the value to fit within int8_t range [-128, 127]
        if (scaled > 127.0) scaled = 127.0;
        if (scaled < -128.0) scaled = -128.0;

        // Store the result as int8_t
        outputArray[i] = static_cast<int8_t>(std::round(scaled));
    }
}

void setup(){
    bool rlst = false;

    Serial.begin(115200);

    twr.begin();
    if (twr.getVersion() == TWRClass::TWR_REV2V1) {
        Serial.println("Detection using TWR Rev2.1");

        // Initialize SA868
        radio.setPins(SA868_PTT_PIN, SA868_PD_PIN);
        rlst = radio.begin(RadioSerial, twr.getBandDefinition());

    } else {

        Serial.println("Detection using TWR Rev2.0");

        //* Rev2.0 cannot determine whether it is UHF or VHF, and can only specify the initialization BAND.

        //* Designated as UHF
        //  rlst = radio.begin(RadioSerial, SA8X8_UHF);

        //* Designated as VHF
        radio.setPins(SA868_PTT_PIN, SA868_PD_PIN, SA868_RF_PIN);
        rlst = radio.begin(RadioSerial, SA8X8_VHF);
    }

    Serial.println(rlst ? "Radio initialization succeeded" : "Radio initialization failed");
    analogSetPinAttenuation(ADC1_CHANNEL, ADC_11db);
    twr.enablePowerOff(true);
    // Setup for OLED
    uint8_t addr = twr.getOLEDAddress();
    while (addr == 0xFF) {
        Serial.println("OLED is not detected, please confirm whether OLED is installed normally.");
        delay(1000);
    }
    u8g2.setI2CAddress(addr << 1);
    u8g2.begin();
    u8g2.setFontMode(0);               // write solid glyphs
    u8g2.setFont(u8g2_font_cu12_hr);   // choose a suitable h font
    u8g2.setCursor(0,20);              // set write position
    u8g2.print("Reciever");            // use extra spaces here
    u8g2.sendBuffer();                 // transfer internal memory to the display
    recieved = new int[array_size];
    recieved_scaled = new int8_t[array_size];
    bit = new int8_t[array_size/8];
    radio.setRxFreq(446200000);
    radio.setTxFreq(446200000);
    radio.setRxCXCSS(0);
    radio.setTxCXCSS(0);
}

void loop(){
    int RCV_In = analogRead(RCV_CHANNEL);
    // check if we are recieving and have not filled the array
    if (RCV_In < 1000 && i < array_size){
        int AN_In1 = analogRead(ADC1_CHANNEL);
        recieved[i] = AN_In1;
        i++;
        recieving = true;
        if (AN_In1 < minValue){
            minValue = AN_In1;
        }
        if (AN_In1 > maxValue){
            maxValue = AN_In1;
        }
    }
    // check if we have recieved and are no longer recieving
    if (recieving && RCV_In > 1000){
        recieving = false;
        int max_i = i - 1;
        // scaling
        scaleTo8Bit(recieved, recieved_scaled, max_i, minValue, maxValue);
        while(i < array_size){
            recieved_scaled[i] = 0;
            i++;
        }
        // print the scaled array
        // for (int j = 0; j < (max_i + 1); j++){
        //     Serial.println(recieved_scaled[j]);
        // }
        delete[] recieved;
        // What goes here now?
        for (int j = 0; j < 8; j++){
            for (int k = 0; k < 250; k++){
                bit[k] = recieved_scaled[j * 250 + k];
            }
            int16_t fft_result = fix_fftr(bit, 8, 0);
            int max_index = 0;
            int max_magnitude = 0;
            for (int l = 0; l < 250; l++) {
                int real = bit[l];
                if (real > max_magnitude) {
                    max_magnitude = real;
                    max_index = l;
                }
            }
            Serial.println("Max Magnitude:");
            Serial.println(max_magnitude);
            // Serial.println("Max Index:");
            // Serial.println(max_index);
            // Serial.println("FTT Result:");
            // Serial.println(fft_result);
        }
        // clean up and print, get ready to recive again
        delete[] recieved_scaled;
        recieved_scaled = new int8_t[array_size];
        recieved = new int[array_size];
        i = 0;
        minValue = 5000;
        maxValue = 0;
    }
    // so that we have a smapling rate of 1000 Hz
    delay(1);
}