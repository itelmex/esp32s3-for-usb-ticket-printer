# ESP32S3 FOR USB PRINTER

## Esc Pos Usb printer with Esp32S3

The code that finally worked doesn't rely on a "library" in the traditional Arduino sense (like something you download from a Library Manager). 
Instead, it uses the Native ESP-IDF USB Host Stack.

## Why this approach worked?:

### Hardware Directness: 

Most Arduino USB libraries are written for external chips (like the MAX3421E). This code, at change,  talks directly to the internal USB engine of the ESP32-S3.

### Interface Claiming: 

The point is the use of "usb_host_interface_claim". USB devices are protective; they won't accept data until a Host explicitly "claims" the interface.

### Endpoint Discovery: 

By manually scanning the descriptors, we stopped "guessing" if the printer was on address 1 or 2 and let the esp32s3 find the correct path.

## Key Components of the code script:

-.usb_host_install Powers on the USB hardware inside the ESP32-S3.

-.usb_host_client_register Creates a "listener" for when you plug/unplug the cable.

-.usb_host_interface_claim Locks the printer so it only listens to your ESP32.

-.usb_host_transfer_submit Pushes the ESC/POS bytes through the wire to the thermal head.


## Videos:

https://www.youtube.com/shorts/tLLG3RAhL6I


![IMG-20260111-WA0007](https://github.com/user-attachments/assets/139dea82-fcfe-4b6b-8dba-15d83fa999a2)

I also tested the printing on an Epson TM-T20II USB printer. It printed very well as well.

![foto epson](https://github.com/user-attachments/assets/d318630d-b62d-4fca-9b93-4184b839b35b)


## Specific application:

For years I have developed printing systems over the Internet, but this always required the use of microcontrollers that printed over serial/RS232 ports. Now, I will be able to use USB printers, which are more modern and accessible.
