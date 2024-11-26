#include "LilyGo_TWR.h"
int ADC1_CHANNEL = 1;
int RCV_CHANNEL = 2;
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
    radio.setRxFreq(446200000);
    analogSetPinAttenuation(ADC1_CHANNEL, ADC_11db);
    twr.enablePowerOff(true);
}

void loop(){
    int RCV_In = analogRead(RCV_CHANNEL);
    if (RCV_In < 1000){
        Serial.println("Radio is receiving:");
    }
    int AN_In1 = analogRead(ADC1_CHANNEL);
    Serial.println(AN_In1);
    delay(250);
}