# Custom Packet Format and (De)Modulation auf den LylyGo T-TWR Funkgeräten
This repository contains the implementation for the project of group 2 and 3 for the semniar _Radio Packet Networks_ in HS24 at the University of Basel.

# Project Description
Both groups work together to implement the basic functionality needed to send and receive information via the radios. Then work was divided into two groups.

### Group 2 - Packet Format
Group 2 focuses on creating a packet format that uses synchronisation to determine the beginning of the message and uses forward error correction. The packet contains one block of fixed length, the packet information. This block holds the length of the message, meaning the length of the second block.

### Group 3 - Modulation
Group 3 implemented modulation on the sender side and demodulation on the receiver side. The modulation was implemented using 2-FSK, where "1" is represented by the frequency 1200 Hz and "0" by the frequency 600 Hz. The sender plays each frequency for a set duration of 250 milliseconds.
The demodulation works with a running average calculated using the formula: 0.8 * running_avg + 0.2 * AN_In1. The frequency is then determined by counting how many times the amplitude crosses the running average. A count of more than 450 zero-crossings indicates a "1," corresponding to the higher frequency. Conversely, fewer zero-crossings indicate a "0" and the lower frequency.
For the demodulation to work correctly, the window size is also set to 250 milliseconds.

# User Guide
This requires VSCode and the PlatformIO extenstion and at least 2 Lyligo T-TWR radios.

- In the file ``platformio.ini`` uncomment the import of the programm you want to use. 
- On the bottom of the VSCode GUI click the tick to build the project and the rightarrow to uppload it to your devices.
