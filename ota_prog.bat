@ECHO OFF
ECHO programming motion1 object
pio run --target upload -e default --upload-port motion1.local
ECHO programming motion2 object
pio run --target upload -e default --upload-port motion2.local
ECHO programming motion3 object
pio run --target upload -e default --upload-port motion3.local