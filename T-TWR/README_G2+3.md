# Custom Packet Format and (De)Modulation auf den LylyGo T-TWR Funkgeräten
This repository contains the implementation for the project of group 2 and 3 for the seminar _Radio Packet Networks_ in HS24 at the University of Basel.

# Project Description
Both groups work together to implement the basic functionality needed to send and receive information via the radios. Then work is divided into two groups.

### Group 2 - Packet Format
Group 2 focuses on creating a packet format that uses synchronisation to determine the beginning of the message and uses forward error correction. The packet is composed of two blocks, one block of fixed length which holds the length of the second block. The second block contains the actual message. The fixed size block is defined to have a length of 8 bits, limiting the maximum message length to 2^8-1 characters.
For the forward error correction, every 4 bits are encoded using Hamming codes. 

### Group 3 - Modulation
Group 3 implements modulation at the sender side and demodulation at the receiver side. The modulation is implemented using 2-FSK, where "1" is represented by the frequency 1200 Hz and "0" by the frequency 600 Hz. The sender plays each frequency for a set duration of 63 milliseconds.

The demodulation works with a running average calculated using the formula: 0.8 * running_avg + 0.2 * AN_In1. The frequency is then determined by counting how many times the amplitude crosses the running average. A count of more than 113 zero-crossings indicates a "1", corresponding to the higher frequency. Conversely, fewer zero-crossings indicate a "0", corresponding to the lower frequency.
For the demodulation to work correctly, the window size is also set to 63 milliseconds.

# User Guide
This requires VSCode with the PlatformIO extenstion, at least two Lyligo T-TWR radios and one computer per radio.

- In the file ``platformio.ini`` uncomment the import of the programm you want to use and comment the one you don't want. Currently our addition is uncommented.
- To build the project, press the tick button at the bottom of the VSCode GUI
- Connect each radio to a computer via the USB-C port and click the right arrow next to the tick to upload the project to your device.
- Once the upload is done, click the plug symbol at the bottom of the GUI to open the serial monitor. 
- Sender: The sender can type a message into the serial monitor. By hitting enter, the message is sent.
- Receiver: The receiver will see the message on his serial monitor after it is decoded.

