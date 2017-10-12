# Rain-check
This IoT project aims to provide an automated way of answering the question *"Do i Need to bring an umbrella to work today?"*.

No more need to check the weather forecast on your computer or phone!

# Platform
NodeMCU

# Setting up the project
1. Download the code
2. Modify `settings.h` (pretty self explanatory - see instructions below)
3. Open `raincheck.ino` in the Arduino IDE and hit `Upload`
4. The unit will now run the program directly upon boot every time it is powered on

# Settings
The NodeMCU will connect to your wifi using the SSID and password specified in `settings.h`.

Settings in this file are also used to specify the coordinate to lookup weather for, as well as how many hours of the forecast will be considered.

# Privacy note
In order to avoid accidentally commiting your wifi credentials and location after updating `settings.h`, you can run the following command:

`git update-index --assume-unchanged settings.h`
