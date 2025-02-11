#include "LilyGo_TWR.h"
#include "Constants.h"
#include <AceButton.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "hamming.h"

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

// ######################## Constants and Globals  ########################
#define COUNT(array) (sizeof(array) / sizeof(array[0]))

int ADC1_CHANNEL = 1; // Analog pin for reading the audio signal
int RCV_CHANNEL = 2; // Analog pin for reading if we recieve a signal
int zeros = 0; // counts the number of zeros for the demodulation
int running_avg = 700; // for the modulation, is always updated according to fomula from Prof. Tschudin 
bool above_avg = true; // is the read value above the running average
bool old_above_avg = true; // is the previous value above the running average
unsigned long start_time = 0; // used to measure the time for the demodulation
bool started_time = false; // used to check if we started the time for the demodulation
int rvc_msg_size = 2200; // how big is our array to recieve messages
bool * received_msg; // Array to store the received message
int current_received = 0; // Index of the current received bit
fecmagic::HammingCode c; // for the hamming code


bool timer_for_decode_started = false; // Timer for the decoding of the message
unsigned long start_decode_time = 0; // Start time for the decoding of the message

// not sure if this is needed but we'll keep it for now (has to be tested)
const int buffer_size = 10;
int signal_buffer[buffer_size];
int buffer_index = 0;
AceButton button(ENCODER_OK_PIN);  // Not sure if all the button stuff is still necessary (also above the globals)
bool sending = false; // Sending state flag (don't think this is needed anymore)


// ######################## Setup method  ########################
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

    //* Enable power off function
    twr.enablePowerOff(true);

    //* setup the OLED and print the function of the device
    uint8_t addr = twr.getOLEDAddress();
    while (addr == 0xFF) {
        Serial.println("OLED is not detected, please confirm whether OLED is installed normally.");
        delay(1000);
    }
    u8g2.setI2CAddress(addr << 1);
    u8g2.begin();
    u8g2.setFontMode(0);               
    u8g2.setFont(u8g2_font_cu12_hr);
    u8g2.setCursor(0,20);
    u8g2.print("Sender & Receiver");
    u8g2.sendBuffer();

    //* initialize the radio so that it is always the same
    radio.setRxFreq(446200000);
    radio.setTxFreq(446200000);
    radio.setRxCXCSS(0);
    radio.setTxCXCSS(0);

    //* Create an array to store the received message
    received_msg = new bool[rvc_msg_size];
}


// ######################## Sending methods  ##########################

// Send the given message and add synchronization bits at the beginning
void playMessage(uint8_t pin, uint8_t channel, bool* message, int size) {
    ledcAttachPin(pin, channel);

    // Send two zeros to ensure the receiver is ready
    ledcWriteTone(channel, 600);
    delay(500);
    // Send a synchronization sequence (10101010)
    int pattern[7] = {1,1,1,-1,-1,1,-1}; 
    for (int i = 0; i < 7; i++) {
        if (pattern[i] == 1) {
            ledcWriteTone(channel, 1200); // '1'
        } else {
            ledcWriteTone(channel, 600);  // '0'
        }
        delay(250); // Bit duration
    }

    // Send the actual message
    for (int i = 0; i < size; i++) {
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


//Convert a string to a boolean array that represents the string in binary with the first 8 bits representing the length of the string
bool* convertStringToBool(String userInput) {
    userInput.trim();

    int stringLength = userInput.length();
    int totalBits = (stringLength * 8) + 8; 
    bool* boolArray = new bool[totalBits];

    // Store the length in the first 8 bits
    for (int bit = 7; bit >= 0; bit--) {
        boolArray[7 - bit] = (stringLength & (1 << bit)) != 0;
    }

    for (int i = 0; i < stringLength; i++) {
        char c = userInput[i];

        // Convert the character to 8 bits and store them after the length bits
        for (int bit = 7; bit >= 0; bit--) {
            boolArray[8 + i * 8 + (7 - bit)] = (c & (1 << bit)) != 0;
        }
    }

    return boolArray;
}

// ######################## Receiving methods  ########################

// check if the sync pattern is detected in the received message starting from the given index
bool syncPatternDetected(int start_value) {
    int pattern[8] = {1, 0, 1, 0, 1, 0, 1, 0}; // Sync pattern: 10101010
    for (int i = start_value; i < 8 + start_value; i++) {
        if (received_msg[i] != pattern[i - start_value]) {
            return false;
        }
    }
    return true;
}

bool syncPatternBarkerCodeDetected(int start_value) {
    int pattern[7] = {1,1,1,-1,-1,1,-1}; // Barker code
    //compute autocerrlation
    int sum = 0; 
    for (int i = start_value; i < 7 + start_value; i++) {
        if (received_msg[i] == 0) {
            sum -= pattern[i - start_value]; 
        } else {
            sum += pattern[i - start_value]; 
        }
    }
    if (sum >= 5) {
        return true; 
    } else {
        return false; 
    }
}

// find the sync pattern in the received message and decode the message
void processReceivedMessage() {
    // find the sync pattern
    int offset = 0;
    bool pattern_detected = false;
    for (int i = 0; i < current_received; i++){
        if (syncPatternBarkerCodeDetected(i)){
            offset = i;
            Serial.print("Pattern detected at: ");
            Serial.println(offset);
            pattern_detected = true;
            break;
        }
    }

    // Check if the sync pattern was detected
    if (!pattern_detected) {
        Serial.println("Sync pattern not detected.");
        return;
    // Decode the message
    } else {
        // TODO do the error correction here (get message from the bits with correction codes)
        bool* error_corrected_msg = fecDecodeMessage(&received_msg[7 + offset]); // Skip the sync pattern
        String decoded_msg = decodeMessage(error_corrected_msg); 
        Serial.print("Decoded Message: ");
        Serial.println(decoded_msg);
    }
}

// Decode a message from a boolean array where the first 8 bits represent the length of the message
String decodeMessage(const bool* boolArray) {
    // Decode the length from the first 8 bits
    int length = 0;
    for (int i = 0; i < 8; i++) {
        if (boolArray[i]) {
            length |= (1 << (7 - i));
        }
    }
    String decodedMessage = "";

    // Decode each character from the remaining bits
    for (int i = 0; i < length; i++) {
        char c = 0;
        for (int bit = 0; bit < 8; bit++) {
            if (boolArray[8 + i * 8 + bit]) {
                c |= (1 << (7 - bit));
            }
        }
        decodedMessage += c; 
    }

    return decodedMessage;
}

// ######################## FEC methods  ########################

bool* fecEncodeMessage (bool* message, int messageLength) {
    bool* encodedMessage = new bool[messageLength*16];
    //fecmagic::HammingCode c;
    // encode the message
    for (int i = 0; i < messageLength*2; i++){
        uint8_t word = 0;
        //encode 4 bits at a time as 8 bits in a unit8_t as least significant bits
        for (int j = 0; j < 4; j++) {
            word |= (message[i*4 + j] << j);
        }
        // use hamming code function to encode the word
        uint8_t coded = c.encode(word);
        // store the encoded word in the encoded message as a true false array
        for (int j = 0; j < 8; j++){
            encodedMessage[i*8+j] = coded & 1;
            coded = coded >> 1;
        }
    }
    if (message != nullptr) {
        delete[] message;
        message = nullptr;
    }
    return encodedMessage;
}

bool* fecDecodeMessage (bool* message) {
    Serial.println("Decoding packet info");
    bool* msg_length = fecDecodeMessage(message, 1); // get packet info with length of message
    // convert bit to int length
     Serial.println("packet info decoded");
    int length = 0;
    for (int i = 0; i < 8; i++) {
        if (msg_length[i]) {
            length |= (1 << (7 - i));
        }
    }
    Serial.print("Decoded length: ");
    Serial.println(length);
    if (msg_length != nullptr) {
        delete[] msg_length;
        msg_length = nullptr;
    }
    // decode actual message
    bool* decodedMessage = fecDecodeMessage(message, length+1);
    Serial.println("Decoded message");
    if (message != nullptr) {
        delete[] message;
        message = nullptr;
    }
    return decodedMessage;
}

bool* fecDecodeMessage (bool* encodedMessage, int messageLength) {
    bool* decodedMessage = new bool[messageLength*8];
    Serial.println("array or decodede message created");
    for (int i = 0; i < messageLength*2; i++){
        uint8_t word = 0;
        //decode 8 bits at a time in a unit8_t as least significant bits
        for (int j = 0; j < 8; j++) {
            word |= (encodedMessage[i*8 + j] << j);
        }
        Serial.println("word created");
        Serial.println(word);
        // use hamming code function to decode the word
        bool decodeSuccess;
        uint8_t decoded = c.decode(word, decodeSuccess);
        if (!decodeSuccess){
            Serial.println("Error decoding did not work");
        }
        Serial.println("decode success!");
        // cout << "decoded: " << decoded << endl;
        // store the decoded word in the decoded message as a true false array
        for (int j = 0; j < 4; j++){
            decodedMessage[i*4+j] = decoded & 1;
            decoded = decoded >> 1;
        }
    }
    return decodedMessage;
}

// ######################## Main Loop ########################
void loop() {

    // ----- SENDING ------
    if (Serial.available()) {
        String userInput = Serial.readStringUntil('\n');
        int msg_length = userInput.length()*8+8;
        // TODO Add error handling for too long messages since we are restriceted to 256 characters by the 8 bit integer at the beginning of the message
        //convert String to byte 
        bool * result = convertStringToBool(userInput); 
        Serial.print("Sending: ");
        Serial.println(userInput);
        // TODO add error correction here (add correction codes to the message)
        result = fecEncodeMessage(result, msg_length);
        // Send the binary message
        radio.transmit();
        playMessage(ESP2SA868_MIC, 0, result, msg_length);
        // Safly delete the result array
        if (result != nullptr) {
            delete[] result;
            result = nullptr;
        }
        radio.receive();
    }

    // ----- DEMODULATION ------
    // check if we receive a signal
    int RCV_In = analogRead(RCV_CHANNEL);
    if (RCV_In < 1000){
        timer_for_decode_started = false;
        start_decode_time = 0;
        // check if we started the time for the demodulation and if not start it
        if (!started_time) {
            started_time = true; 
            start_time = millis(); 
        }
        // read the audio signal, update the running average and check if it is above the running average
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
        // check it the signal is now above or below the running average and if it was not before, then update the number of zeros
        if (old_above_avg && !above_avg){
            zeros++;
        }
        if (above_avg && !old_above_avg){
            zeros++;
        }
        // after 250ms we have recieved a bit and check it the amount of zeros is below or above 450 which is the threshold for a 1 or 0
        if (250 <= millis() - start_time){
            if (zeros < 450) {
                // print some debug information and store the received bit
                Serial.print("Number of zeros: ");
                Serial.println(zeros);
                Serial.println("This is a 0");
                received_msg[current_received] = 0;
                current_received++;
            } else {
                // print some debug information and store the received bit
                Serial.print("Number of zeros: ");
                Serial.println(zeros);
                Serial.println("This is a 1");
                received_msg[current_received] = 1;
                current_received++;
            }
            zeros = 0;
            started_time = false; 
        }
    
    // ----- DECODING ------
    } else if(!timer_for_decode_started) {
        timer_for_decode_started = true;
        start_decode_time = millis();
    } else if (current_received >= 16 && 250 <= millis() - start_decode_time) {
        timer_for_decode_started = false;
        start_decode_time = 0;
        processReceivedMessage(); 
        Serial.print("Current received: ");
        Serial.println(current_received);
        // Reset so we can receive a new message
        current_received = 0;
        if (received_msg != nullptr){
            delete[] received_msg;
            received_msg = new bool[rvc_msg_size];
        }
    }
}
