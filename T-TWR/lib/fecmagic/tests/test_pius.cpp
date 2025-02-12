#include <iostream>
#include <cstdint>
#include "../src/hamming.h"

using namespace std;
using namespace fecmagic;

int main() {
    bool message[] = {true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false};
    int messageLength = 2;
    bool encodedMessage[messageLength*16];
    bool decodedMessage[messageLength*8];
    HammingCode c;
    // encode the message
    for (int i = 0; i < messageLength*2; i++){
        uint8_t word = 0;
        //encode 4 bits at a time as 8 bits in a unit8_t as least significant bits
        for (int j = 0; j < 4; j++) {
            word |= (message[i*4 + j] << j);
        }
        cout << "word: " << word << endl;
        // use hamming code function to encode the word
        uint8_t coded = c.encode(word);
        cout << "coded: " << coded << endl;
        // store the encoded word in the encoded message as a true false array
        for (int j = 0; j < 8; j++){
            encodedMessage[i*8+j] = coded & 1;
            coded = coded >> 1;
        }
    }
    cout << "Encoded message: ";
    for(int i = 0; i < messageLength*16; i++){
        cout << encodedMessage[i];
    }
    cout << endl;
    // filp a bit in the encoded message
    int flipBit = 2;
    encodedMessage[flipBit] = !encodedMessage[flipBit];
    // decode the message
    for (int i = 0; i < messageLength*2; i++){
        uint8_t word = 0;
        //decode 8 bits at a time in a unit8_t as least significant bits
        for (int j = 0; j < 8; j++) {
            word |= (encodedMessage[i*8 + j] << j);
        }
        // use hamming code function to decode the word
        bool decodeSuccess;
        uint8_t decoded = c.decode(word, decodeSuccess);
        cout << "decoded: " << decoded << endl;
        // store the decoded word in the decoded message as a true false array
        for (int j = 0; j < 4; j++){
            decodedMessage[i*4+j] = decoded & 1;
            decoded = decoded >> 1;
        }
    }
    cout << "Decoded message: ";
    for(int i = 0; i < messageLength*8; i++){
        cout << decodedMessage[i];
    }
    cout << endl;
}