@ECHO OFF
ECHO programming motion1 object
pio run --target upload -e default --upload-port 10.0.1.63
ECHO programming motion2 object
pio run --target upload -e default --upload-port 10.0.1.170
ECHO programming motion3 object
pio run --target upload -e default --upload-port 10.0.1.73