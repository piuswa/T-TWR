#include "LilyGo_TWR.h"
#include "Constants.h"
#include <AceButton.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
using namespace ace_button;

AceButton                           buttons[3];


const uint8_t                       buttonPins [] = {
    ENCODER_OK_PIN,
    BUTTON_PTT_PIN,
    BUTTON_DOWN_PIN
};

enum Button {
    ShortPress,
    LongPress,
    Unknown
};

// Constants and Globals
#define COUNT(array) (sizeof(array) / sizeof(array[0]))

int ADC1_CHANNEL = 1;
int RCV_CHANNEL = 2;
int zeros = 0;
int running_avg = 700;
bool above_avg = true;
bool old_above_avg = true;
unsigned long start_time = 0;
bool started_time = false;

bool * received_msg = new bool[2056]; 
int current_received = 0; 

const int buffer_size = 10;
int signal_buffer[buffer_size];
int buffer_index = 0;

AceButton button(ENCODER_OK_PIN);
// String fixedMessage = "00110101";
bool sending = false; // Sending state flag


// Function to send a fixed message
void playMessage(uint8_t pin, uint8_t channel, bool* message, int size) {
    ledcAttachPin(pin, channel);
    // ledcWriteTone(channel, 1700); // Optional sync signal
    // delay(1000);

    for (uint8_t i = 0; i < size; i++) {
        if (message[i] == 0) {
            ledcWriteTone(channel, 600);
        } else {
            ledcWriteTone(channel, 1200);
        }
        delay(250);
    }
    ledcWriteTone(channel, 0);
    ledcDetachPin(pin);
    Serial.println("Message sent.");
}

void setup()
{
    bool rlst = false;

    Serial.begin(115200);

    //* Initializing PMU is related to other peripherals
    //* Automatically detect the revision version through GPIO detection.
    //* If GPIO2 has been externally connected to other devices, the automatic detection may not work properly.
    //* Please initialize with the version specified below.
    //* Rev2.1 is not affected by GPIO2 because GPIO2 is not exposed in Rev2.1
    twr.begin();

    //* If GPIO2 is already connected to other devices, please initialize it with the specified version.
    // twr.begin(LILYGO_TWR_REV2_0);

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

    if (!rlst) {
        while (1) {
            Serial.println("SA868 is not online !");
            delay(1000);
        }
    }

    //* Microphone will be routed to ESP ADC IO15 and the SA868 audio input will be routed to ESP IO18
    twr.routingMicrophoneChannel(TWRClass::TWR_MIC_TO_ESP);

    // Start transfer
    // radio.transmit();
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
    u8g2.print("Sender & Receiver");              // use extra spaces here
    u8g2.sendBuffer();                 // transfer internal memory to the display
    radio.setRxFreq(446200000);
    radio.setTxFreq(446200000);
    radio.setRxCXCSS(0);
    radio.setTxCXCSS(0);
}


bool* convertStringToBool(String userInput) {
    userInput.trim(); // Remove leading/trailing whitespace

    int stringLength = userInput.length();
    int totalBits = (stringLength * 8) + 8; // Include 8 bits for the length

    // Dynamically allocate the boolean array
    bool* boolArray = new bool[totalBits];

    // Store the length in the first 8 bits
    for (int bit = 7; bit >= 0; bit--) {
        boolArray[7 - bit] = (stringLength & (1 << bit)) != 0;
    }

    // Iterate over each character in the string
    for (int i = 0; i < stringLength; i++) {
        char c = userInput[i]; // Get the character

        // Convert the character to 8 bits and store them after the length bits
        for (int bit = 7; bit >= 0; bit--) {
            boolArray[8 + i * 8 + (7 - bit)] = (c & (1 << bit)) != 0;
        }
    }

    return boolArray;
}

String decodeMessage(const bool* boolArray) {
    // Decode the length from the first 8 bits
    int length = 0;
    for (int i = 0; i < 8; i++) {
        if (boolArray[i]) {
            length |= (1 << (7 - i));
        }
    }

    // Create a string to store the decoded message
    String decodedMessage = "";

    // Decode each character from the remaining bits
    for (int i = 0; i < length; i++) {
        char c = 0;
        for (int bit = 0; bit < 8; bit++) {
            if (boolArray[8 + i * 8 + bit]) {
                c |= (1 << (7 - bit));
            }
        }
        decodedMessage += c; // Append the character to the string
    }

    return decodedMessage;
}


// Main loop
void loop() {

    if (Serial.available()) {
        String userInput = Serial.readStringUntil('\n');
        //convert String to byte 
        bool * result = convertStringToBool(userInput); 
        Serial.print("Sending: ");
        Serial.println(userInput);

        radio.transmit();
        playMessage(ESP2SA868_MIC, 0, result, userInput.length()*8+8);
        radio.receive();
    }

    int RCV_In = analogRead(RCV_CHANNEL);
    if (RCV_In < 1000){
        if (!started_time) {
            started_time = true; 
            start_time = millis(); 
        }
        int AN_In1 = analogRead(ADC1_CHANNEL);
        running_avg = (int) (0.8*(double)running_avg + 0.2*(double)AN_In1);
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
                received_msg[current_received] = 0;
                current_received++;
            }
            else{
                Serial.print("Number of zeros: ");
                Serial.println(zeros);
                Serial.println("This is a 1");
                received_msg[current_received] = 1;
                current_received++;
            }
            zeros = 0;
            started_time = false; 
        }
    } else if (current_received != 0) {
        String decoded_msg = decodeMessage(received_msg);
        Serial.print("Decoded Message: ");
        Serial.println(decoded_msg); 
        current_received = 0;
    }
}
