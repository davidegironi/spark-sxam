//default xively feed and api
#define XIVELY_FEEDID "YOURFEEDIDHERE"
#define XIVELY_APIKEY "YOURAPIKEYHERE"

//debug to serial port
#define UARTDEBUG 1

//application version
#define APPVERSION "0.1"
#define APPNAME "XivelyLib\r\nCopyright(c) Davide Gironi, 2014"

//xively status led port
#define XIVELY_STATUSLED_PORT D7

//reload default values to eeprom
#define CONFIG_LOADDEFAULT 1

//xively feed update time (seconds)
#define XIVELY_UPDATETIMERINTERVALSEC 60

//active connection test time (minutes)
#define SPARK_CONNECTIONINTERVALMIN 5

//sensor datastream channels
#define XIVELYDATAPOINT_TEMPERATURECHANNEL "1"
#define XIVELYDATAPOINT_HUMIDITYCHANNEL "2"
#define XIVELYDATAPOINT_BRIGHTNESSCHANNEL "3"
#define XIVELYDATAPOINT_NOISECHANNEL "4"

//timer to update the reading of temperature and humidity sensor
#define DHT22_UPDATETIMERINTERVALMS 10000 

//timer to update the reading of brightness sensor
#define BH1750_UPDATETIMERINTERVALMS 100 

//timer to update the reading of noise sensor
#define AUDIOGET_UPDATETIMERINTERVALMS 100 

//set the reference ADC voltage
#define ANALOGVOLTAGEREF 3.3

//humidity and temperature sensor port
#define DHTPIN D4
#define DHTTYPE DHT22 //DHT11 or DHT22

//set the audio input ADC channel
#define AUDIO_INPUTCHANNEL A0
//set the audio input board reference voltage
#define AUDIO_VOLTREF 0.0022
//set the audio input board reference dB value
#define AUDIO_DBREF 40

//set the led strip
#define PIXEL_PIN D2
#define PIXEL_COUNT 10
#define PIXEL_TYPE WS2812B

//set led strip limits
#define XIVELYDATAPOINT_PIXELMIN 0
#define XIVELYDATAPOINT_PIXELMAX 85
#define XIVELYDATAPOINT_TEMPERATUREPIXEL 1
#define XIVELYDATAPOINT_TEMPERATUREPIXELTHRESHOLD 3
#define XIVELYDATAPOINT_TEMPERATUREPIXELMAX 25 //optimal: 20<C<25
#define XIVELYDATAPOINT_TEMPERATUREPIXELMIN 20
#define XIVELYDATAPOINT_TEMPERATUREPIXELABSMAX 50
#define XIVELYDATAPOINT_TEMPERATUREPIXELABSMIN 20
#define XIVELYDATAPOINT_HUMIDITYPIXEL 2
#define XIVELYDATAPOINT_HUMIDITYPIXELTHRESHOLD  1.2
#define XIVELYDATAPOINT_HUMIDITYPIXELMAX 16.7 //optimal: 2.8<dewpoint<16.7
#define XIVELYDATAPOINT_HUMIDITYPIXELMIN 2.8
#define XIVELYDATAPOINT_HUMIDITYPIXELABSMAX 28
#define XIVELYDATAPOINT_HUMIDITYPIXELABSMIN 0.8
#define XIVELYDATAPOINT_BRIGHTNESSPIXEL 3
#define XIVELYDATAPOINT_BRIGHTNESSPIXELTHRESHOLD 50
#define XIVELYDATAPOINT_BRIGHTNESSPIXELMIN 400 //optimal: >400lux
#define XIVELYDATAPOINT_BRIGHTNESSPIXELABSMIN 20
#define XIVELYDATAPOINT_NOISEPIXEL 4
#define XIVELYDATAPOINT_NOISEPIXELTHRESHOLD 5
#define XIVELYDATAPOINT_NOISEPIXELMAX 50 //optimal: <50db
#define XIVELYDATAPOINT_NOISEPIXELABSMAX 90
