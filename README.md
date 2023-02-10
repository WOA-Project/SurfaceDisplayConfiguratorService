# Surface Display Configurator Service

This repository holds the code responsible for configuring and setting up the display topology layouts on Surface Duo devices.

Additionally, this service performs the following:

- Sets up initial display configuration in 2D space
- Sets up both displays to be in extended mode in regards to each other
- Enables rotation of both displays at the same time
- Manages auto rotation lock settings on behalf of the user for both displays
- Handles cutting off listening to sensors when the device goes to sleep
- Notifies the Windows Auto rotation LPC port to trigger the rotation animation if needed
- Listens to Posture sensor and Flip sensor to enable basic device gesture functionality for foldable dual screen devices
- Enables Tablet Taskbar experience in Windows 11 version 21H2 and higher for both displays

## Acknowledgements

- ADeltaX for his help on reversing internal sensor posture and sensor flip APIs.
- Imbushuo for reversing the Windows Auto Rotation Alpc Port API.
- Rafael Rivera for contributing to better error handling and a quick review in a pinch
- Windows 95 for providing up to date documentation on the still unchanged Windows Display APIs (More modern APIs would be very welcome!)

## License

The code in this repository is licensed under the MIT license unless otherwise specified