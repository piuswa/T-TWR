#include "LilyGo_TWR.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

int ADC1_CHANNEL = 1;
int RCV_CHANNEL = 2;
int array_size = 16384;
int i = 0;
int* recieved;
bool recieving = false;

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
    u8g2.print("Reciever");              // use extra spaces here
    u8g2.sendBuffer();                 // transfer internal memory to the display
    recieved = new int[array_size];
    radio.setRxFreq(446200000);
    radio.setTxFreq(446200000);
    radio.setRxCXCSS(0);
    radio.setTxCXCSS(0);
}

void loop(){
    int RCV_In = analogRead(RCV_CHANNEL);
    if (RCV_In < 1000 && i < array_size){
        int AN_In1 = analogRead(ADC1_CHANNEL);
        //Serial.println(i);
        //Serial.println(AN_In1);
        recieved[i] = AN_In1;
        //Serial.println(*(recieved[i]));
        i++;
        recieving = true;
    }
    if (recieving && RCV_In > 1000){
        int max_i = i;
        while(i < array_size){
            recieved[i] = 0;
            i++;
        }
        for (int j = 0; j < max_i; j++){
            Serial.println(recieved[j]);
        }
        delete[] recieved;
        recieved = new int[array_size];
        i = 0;
        recieving = false;
        //Serial.println("End of transmission, Max i:");
        //Serial.println(max_i);
    }
    delay(1);
}