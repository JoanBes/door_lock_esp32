# ESP32 door lock with RFID and MQTT

Firmware for an ESP32 Dev Module with an MFRC522 reader, a servo motor, and an
LED. An MQTT server authorizes opening; scanning the same RFID card again
closes the door.

## Operation

The lock has three logical states:

1. **Closed, no request:** waiting for an RFID card.
2. **Waiting for authorization:** the UID has been published and the lock is
   waiting for the server's calculated code.
3. **Open:** the servo stays open and the LED stays on until the same UID is
   scanned again.

The ESP32 connects to Wi-Fi and the MQTT broker, reads and publishes a card's
hexadecimal UID, and waits for the server to publish the access code. If the
code is correct, the servo opens and the LED turns on. The door remains open
without a timer. Scanning the same card closes the door, turns off the LED,
and publishes the release code. Other cards and repeated MQTT commands are
ignored while the door is open.

## Access code

Instead of generic commands such as `OK` or `UNLOCK`, the firmware computes an
FNV-1a hash of the normalized UID and represents it as eight uppercase
hexadecimal characters. The server must use the same algorithm as
`include/door_lock/AccessPolicy.h`. The release code is derived from the UID
that opened the door.

## Project structure

```text
src/main.cpp                         Wi-Fi, MQTT, pins, and main loop
include/door_lock/AccessPolicy.h     UID normalization and FNV-1a access code
include/door_lock/AccessControllerLogic.h  Authorization and door state
include/door_lock/Ports.h            Hardware-independent interfaces
include/door_lock/RfidReader.h       MFRC522 SPI adapter
include/door_lock/MqttService.h      MQTT connection and messaging
include/door_lock/DoorActuator.h     Servo and LED control
test/test_access_policy/test_main.cpp  Native logic tests
platformio.ini                       Board, dependencies, and build environments
```

## Configuration

Copy `include/door_lock/config.example.h` to `include/door_lock/config.h`,
then set `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_HOST`, `MQTT_PORT`, `MQTT_USER`,
and `MQTT_PASSWORD` for your network and broker. The local configuration is
ignored by Git; do not commit credentials.

The remaining configuration is in `src/main.cpp`: change `DEVICE_ID` and the
topic identifier for multiple locks, and adjust pins if the wiring changes.
The default topics are:

| Direction | Topic | Payload |
| --- | --- | --- |
| ESP32 to server | `doorlockJBL/01/rfid` | Hexadecimal UID |
| Server to ESP32 | `doorlockJBL/01/command` | Eight-character derived code |
| ESP32 to server | `doorlockJBL/01/released` | Code of the card that closed the door |
| ESP32 to broker | `doorlockJBL/01/status` | `online` or `offline` |

The broker must support TLS on the configured port. The current firmware uses
`setInsecure()` and does not verify the broker's certificate.

## Wiring

| Component | ESP32 |
| --- | --- |
| MFRC522 SDA/SS | GPIO 5 |
| MFRC522 RST | GPIO 4 |
| MFRC522 SCK | GPIO 18 |
| MFRC522 MOSI | GPIO 23 |
| MFRC522 MISO | GPIO 19 |
| MFRC522 VCC | 3.3 V |
| MFRC522 GND | GND |
| Servo signal | GPIO 16 |
| LED with resistor | GPIO 2 |

Connect the RC522 IRQ pin to nothing, and power the reader only from 3.3 V.
The servo requires a suitable supply, normally an external 5 V supply; connect
its ground to the ESP32 ground. Do not power a servo from a GPIO pin or the
ESP32's 3.3 V supply. Connect GPIO 2 to the LED anode through a 220–330 ohm
resistor, and connect the LED cathode to ground.

The signal connections are grouped on the right side of the board: GPIO 4, 5,
16, 18, 19, and 23. See [the wiring diagram](cables_diagram.png).
