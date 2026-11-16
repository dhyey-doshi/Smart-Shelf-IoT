# Smart Shelf System (IoT)

An ESP8266-based smart shelf prototype that detects whether an item is present at the front of a shelf, runs a motor to move the next item forward when the front position is empty, displays the operating state on a 16×2 I2C LCD, and logs state changes to Google Sheets over Wi-Fi.

## Features

- IR-based object presence/absence detection.
- Automatic DC motor control through an L298N H-bridge motor driver.
- 16×2 I2C LCD status display.
- ESP8266 Wi-Fi connectivity.
- Google Sheets logging through a Google Apps Script web app.
- State-change-based logging rather than sending a request on every loop iteration.

## Operating logic

1. The ESP8266 reads the IR sensor's digital output.
2. In the tested project configuration, **HIGH = object absent** and **LOW = object present**.
3. If the object is absent, the ESP8266 commands the L298N to run the motor.
4. When an object is detected, the motor is stopped.
5. The LCD displays the IR and motor states.
6. When the state changes, the ESP8266 sends `ir`, `motor`, and `weight` parameters to Google Apps Script. `weight` is currently sent as `0`; HX711/load-cell readings are not implemented in this final sketch.

## Hardware

| Component | Purpose |
|---|---|
| ESP8266 NodeMCU | Main controller and Wi-Fi interface |
| IR sensor | Detects object presence/absence |
| L298N motor driver | Drives the DC motor |
| DC motor | Moves the shelf mechanism |
| 16×2 I2C LCD | Displays operating status |
| LM2596 buck converter | Regulates the ESP8266 supply |
| Battery | Main power source |
| Capacitors/resistors | Power filtering and signal conditioning as used in the prototype |
| HX711 and load cell | Used/considered during project development; not read by this sketch |

## Pin mapping

| ESP8266 pin | Connection |
|---|---|
| D5 | IR sensor digital output |
| D8 | L298N IN1 |
| D0 | L298N IN2 |
| D2 | LCD SDA |
| D1 | LCD SCL |

**Power and safety:** The project wiring used a regulated 3.3 V supply for the ESP8266 and a 5 V output from the motor-driver module for the LCD/sensor. Verify the actual L298N board's regulator/jumper configuration and 5 V output capability before relying on it. All grounds need a common reference. Never connect 5 V to an ESP8266 GPIO or its 3V3 pin. Do not power a motor directly from an ESP8266 GPIO.

## Software requirements

- Arduino IDE
- ESP8266 board support package
- `LiquidCrystal_PCF8574` library
- ESP8266 core libraries: `ESP8266WiFi`, `ESP8266HTTPClient`, and BearSSL (included with the ESP8266 core)

## Configuration

Open `Smart_Shelf.ino` and replace the placeholders with your own values:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec";
```

Use the `/exec` URL of your deployed Google Apps Script web app. Do not commit real Wi-Fi passwords, private deployment URLs, API keys, or other secrets to a public repository.

The Apps Script `doGet(e)` handler should read `e.parameter.ir`, `e.parameter.motor`, and `e.parameter.weight`, then append the record to the spreadsheet.

## Upload and test

1. Inspect the wiring with motor power disconnected.
2. Verify the ESP8266 has a regulated 3.3 V supply and all grounds share a common reference.
3. Enter your Wi-Fi credentials and Apps Script URL.
4. Select the correct ESP8266 board and port in Arduino IDE, then upload.
5. Open Serial Monitor at **9600 baud**.
6. Verify the IR polarity: absent should be `ABSENT`; present should be `PRESENT`.
7. Check the LCD state and test motor direction with the mechanism unloaded.
8. Verify that state changes are recorded in Google Sheets.

If your IR module has the opposite output polarity, change `OBJECT_ABSENT_LEVEL` from `HIGH` to `LOW` after testing the sensor output.

## Response-time and networking note

The motor and LCD are updated when a changed sensor state is read in the main loop. Cloud logging is only attempted on a state change, reducing unnecessary network traffic. However, HTTPS requests are synchronous in this sketch and can temporarily block the loop while the request completes or times out. This is not fully asynchronous networking. For safety-critical or very fast actuation, use a separate controller for motor control or implement a genuinely asynchronous networking architecture.

## Security note

`client.setInsecure()` simplifies HTTPS certificate handling for this prototype, but it does not verify the server certificate. Use certificate validation for a production deployment.

## Limitations of this firmware

- Uses one IR sensor and binary object detection; it does not identify product types.
- Sends `weight=0`; HX711/load-cell measurement is not integrated in this sketch.
- HTTPS logging is synchronous and can briefly delay the main loop.
- Motor run/stop behavior depends on the mechanical design positioning the next item in front of the sensor.

## Future enhancements

- Integrate calibrated HX711/load-cell measurements.
- Add shelf and product identifiers for multi-shelf deployment.
- Analyze historical depletion data to estimate demand and recommend replenishment.
- Add owner notifications and a centralized inventory dashboard.
- Improve network reliability with buffering and a non-blocking communication architecture.

## License

See `LICENSE`. Confirm that all project contributors agree with the chosen license before publishing.
