#include "LilyGo_TWR.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <math.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int ADC1_CHANNEL = 1;
int RCV_CHANNEL = 2;
// int i = 0;
int zeros = 0;
// int window_size = 2000;
int running_avg = 700;
bool above_avg = true;
bool old_above_avg = true;
int count_for_avg = 0;
unsigned long start_time; 
bool started_time = false; 

const int buffer_size = 10;
int signal_buffer[buffer_size];
int buffer_index = 0;

void updateRunningAverage(int new_value) {
    signal_buffer[buffer_index] = new_value;
    buffer_index = (buffer_index + 1) % buffer_size;

    running_avg = 0;
    for (int i = 0; i < buffer_size; i++) {
        running_avg += signal_buffer[i];
    }
    running_avg /= buffer_size;
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
    radio.setRxFreq(446200000);
    radio.setTxFreq(446200000);
    radio.setRxCXCSS(0);
    radio.setTxCXCSS(0);
}

void loop(){
    int RCV_In = analogRead(RCV_CHANNEL);
    // check if we are recieving
    if (RCV_In < 1000){
        if (!started_time) {
            started_time = true; 
            start_time = millis(); 
        }
        int AN_In1 = analogRead(ADC1_CHANNEL);
        running_avg = (int) (0.8*(double)running_avg + 0.2*(double)AN_In1);
        //updateRunningAverage(AN_In1); 
        if (AN_In1 > running_avg){
            old_above_avg = above_avg;
            above_avg = true;
        }
        else{
            old_above_avg = above_avg;
            above_avg = false;
        }
        if (old_above_avg && !above_avg){
            zeros++;
        }
        if (above_avg && !old_above_avg){
            zeros++;
        }
        if (250 <= millis() - start_time){
            if (zeros < 450) {
                Serial.print("Number of zeros: ");
                Serial.println(zeros);
                Serial.println("This is a 0");
            }
            else{
                Serial.print("Number of zeros: ");
                Serial.println(zeros);
                Serial.println("This is a 1");
            }
            zeros = 0;
            started_time = false; 
            // i = 0;
            // count_for_avg++;
        }
        // i++;
        // if (count_for_avg == 30){
        //     Serial.print("Running average: ");
        //     Serial.println(running_avg);
        //     count_for_avg = 0;
        // }
    }
    // so that we have a smapling rate of 1000 Hz
    //delay(1);
}