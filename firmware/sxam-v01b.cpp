//********************************************************************
// SXAM for Spark
// A Spark to Xively Ambient Monitor
//
// Copyright (c) Davide Gironi, 2014 
//
// Released under GPLv3.
// Please refer to LICENSE file for licensing information.
//********************************************************************

//include xively lib
#include "XivelyLib/XivelyLib.h"
//include dht22 lib
#include "Adafruit_DHT/Adafruit_DHT.h"
//include bh1750 lib
#include "BH1750Lib/BH1750Lib.h"
//include audioget lib
#include "AudioGetAverageLib/AudioGetAverageLib.h"
//include neopixel library
#include "neopixel/neopixel.h"

//include configuration
#include "config.h"

//application info
int ping = 1;
char appName[64] = APPNAME;
char appVersion[12] = APPVERSION;

//xively status led
int xively_statusLed = XIVELY_STATUSLED_PORT;

//xively datapoints
xivelyLib_datapoint xively_datapoints;

//datastream variables
long xivelydatapoint_temperaturetot = 0;
int xivelydatapoint_temperaturetotcount = 0;
bool xivelydatastream_enabled_temperature = false;
long xivelydatapoint_humiditytot = 0;
int xivelydatapoint_humiditytotcount = 0;
bool xivelydatastream_enabled_humidity = false;
long  xivelydatapoint_noisetot = 0;
int  xivelydatapoint_noisetotcount = 0;
bool xivelydatastream_enabled_noise = false;
long  xivelydatapoint_brightnesstot = 0;
int  xivelydatapoint_brightnesstotcount = 0;
bool xivelydatastream_enabled_brightness = false;

//input sensor DHT22
DHT dht(DHTPIN, DHTTYPE);

//input sensor BH1750
BH1750Lib bh1750(BH1750LIB_MODE_CONTINUOUSHIGHRES);

//initialize audioGet library
AudioGetAverageLib audioGet(
    AUDIO_INPUTCHANNEL,
    AUDIOGETAVERAGE_DEFAULTDYNAMICBIAS,
    AUDIOGETAVERAGE_DEFAULTBIASZERORAW,
    AUDIOGETAVERAGE_DEFAULTSMOOTHFILTERVAL,
    AUDIOGETAVERAGE_DEFAULTSAMPLES,
    AUDIOGETAVERAGE_DEFAULTSAMPLESINTERVALMICROSEC,
    8);

//initialize the led strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

//initialize the xively library
XivelyLib xively(XIVELY_FEEDID, XIVELY_APIKEY);

//storage id
#define CONFIG_VERSION "st1"
//storage start address
#define CONFIG_START 0
//storage data
struct ConfigStruct {
    char version[4];
    char xively_feedid[XIVELYLIB_FEEDID_SIZE]; //set the xively feedid
    char xively_apikey[XIVELYLIB_APIKEY_SIZE]; //set the xively apikey
} config = {
    CONFIG_VERSION,
    XIVELY_FEEDID,
    XIVELY_APIKEY
};


// Initialize
void setup()
{
    //register spark functions and variables
    Spark.variable("ping", &ping, INT);
    Spark.variable("appName", &appName, STRING);
    Spark.variable("appVersion", &appVersion, STRING);
    Spark.function("feedId", call_setFeedId);
    Spark.function("apiKey", call_setApiKey);
    Spark.variable("feedId", &config.xively_feedid, STRING);
    Spark.variable("apiKey", &config.xively_apikey, STRING);
 
    //start serial port if enabled
    #if UARTDEBUG == 1
    Serial.begin(9600);
    Serial.print("Starting...");
    #endif
 
    //boot led strip
    strip.begin();
    strip.show();
    colorRainbow(20);
    colorZero();
    
    //initialize dht
    dht.begin();
    
    //setup xively status led
    pinMode(xively_statusLed, OUTPUT);
    digitalWrite(xively_statusLed, LOW);
    
    //reload default config values
    #if CONFIG_LOADDEFAULT == 1
    strncpy(config.xively_feedid, XIVELY_FEEDID, strlen(XIVELY_FEEDID));
    strncpy(config.xively_apikey, XIVELY_APIKEY, strlen(XIVELY_APIKEY));
    saveConfig();
    #endif
        
    //load config
    loadConfig();
}


// Main loop
void loop()
{
    static unsigned long xivelyUpdateTimer = millis(); //xively update timer
    static unsigned long sparkConnectionTimer = millis(); //spark connection timer

    //do sensor readings
    sensorUpdate();
    
    //get xively response
    xively.responseListener();
    
    //send data to xively
    if (millis()-xivelyUpdateTimer > 1000*XIVELY_UPDATETIMERINTERVALSEC) {
        //debug xively status
        #if UARTDEBUG == 1
        Serial.print("Xively feed id: ");
        Serial.println(xively.getFeedId());
        Serial.print("Xively api key: ");
        Serial.println(xively.getApiKey());
        Serial.print("Xively update successful: ");
        Serial.println(xively.isUpdateSuccessful());
        #endif
        
        //update values to xively
        xivelyUpdate();
        
        //reset update timer
        xivelyUpdateTimer = millis();
    }
    
    //check if the spark connection is active
    if (millis()-sparkConnectionTimer > 1000*60*SPARK_CONNECTIONINTERVALMIN) {
        if(!Spark.connected()) //reconnect spark
            Spark.connect();
        sparkConnectionTimer = millis();
    }
}

// Load configuration
void loadConfig() {
    if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] && EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] && EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2]) {
        for (unsigned int t=0; t<sizeof(config); t++) {
            *((char*)&config + t) = EEPROM.read(CONFIG_START + t);
        }
    }
}

// Save configuration
void saveConfig() {
  for (unsigned int t=0; t<sizeof(config); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&config + t));
}

// Update sensor values
void sensorUpdate() { 
    static unsigned long dht22_updateTimer = millis(); //dht22 update timer
    static unsigned long bh1750_updateTimer = millis(); //bh1750 update timer
    static unsigned long audioget_updateTimer = millis(); //audioget update timer
    
    //check when to update the dht22 readings
    if (millis()-dht22_updateTimer > DHT22_UPDATETIMERINTERVALMS) {
        dht22_updateTimer = millis();
        
        #if UARTDEBUG == 1
        Serial.print("Aquire DHT22 data... ");
        #endif
        
        //get values
        int temperature = (int)dht.getTempCelcius();
        int humidity = (int)dht.getHumidity();
        
        if (!isnan(humidity) && !isnan(temperature)) {
            #if UARTDEBUG == 1
            Serial.println("successful.");
            #endif
            
            double dewpoint = dht.getDewPoint();
            
            #if UARTDEBUG == 1
            Serial.print("  Temperature: ");
            Serial.println(temperature);
            Serial.print("  Humidity: ");
            Serial.println(humidity);
            Serial.print("  DewPoint: ");
            Serial.println(dewpoint);
            #endif
            
            //set pixel color
            if(temperature >= XIVELYDATAPOINT_TEMPERATUREPIXELMIN-XIVELYDATAPOINT_TEMPERATUREPIXELTHRESHOLD && temperature <= XIVELYDATAPOINT_TEMPERATUREPIXELMAX+XIVELYDATAPOINT_TEMPERATUREPIXELTHRESHOLD) {
                strip.setPixelColor(XIVELYDATAPOINT_TEMPERATUREPIXEL-1, colorWheel(0));
                strip.show();
            } else {
                uint16_t pos = 0;
                if(temperature > XIVELYDATAPOINT_TEMPERATUREPIXELABSMAX)
                    temperature = XIVELYDATAPOINT_TEMPERATUREPIXELABSMAX;
                else if(temperature < XIVELYDATAPOINT_TEMPERATUREPIXELABSMIN)
                    temperature = XIVELYDATAPOINT_TEMPERATUREPIXELABSMIN;
                if(temperature < XIVELYDATAPOINT_TEMPERATUREPIXELMIN-XIVELYDATAPOINT_TEMPERATUREPIXELTHRESHOLD) {
                    pos = map(temperature, XIVELYDATAPOINT_TEMPERATUREPIXELMIN-XIVELYDATAPOINT_TEMPERATUREPIXELTHRESHOLD, XIVELYDATAPOINT_TEMPERATUREPIXELABSMIN, XIVELYDATAPOINT_PIXELMIN, XIVELYDATAPOINT_PIXELMAX);
                    strip.setPixelColor(XIVELYDATAPOINT_TEMPERATUREPIXEL-1, colorWheel(pos));
                    strip.show();
                } else {
                    pos = map(temperature, XIVELYDATAPOINT_TEMPERATUREPIXELMAX+XIVELYDATAPOINT_TEMPERATUREPIXELTHRESHOLD, XIVELYDATAPOINT_TEMPERATUREPIXELABSMAX, XIVELYDATAPOINT_PIXELMIN, XIVELYDATAPOINT_PIXELMAX);
                    strip.setPixelColor(XIVELYDATAPOINT_TEMPERATUREPIXEL-1, colorWheel(pos));
                    strip.show();
                }
            }
            if(dewpoint >= XIVELYDATAPOINT_HUMIDITYPIXELMIN-XIVELYDATAPOINT_HUMIDITYPIXELTHRESHOLD && dewpoint <= XIVELYDATAPOINT_HUMIDITYPIXELMAX+XIVELYDATAPOINT_HUMIDITYPIXELTHRESHOLD) {
                strip.setPixelColor(XIVELYDATAPOINT_HUMIDITYPIXEL-1, colorWheel(0));
                strip.show();
            } else {
                uint16_t pos = 0;
                if(dewpoint > XIVELYDATAPOINT_HUMIDITYPIXELABSMAX)
                    dewpoint = XIVELYDATAPOINT_HUMIDITYPIXELABSMAX;
                else if(dewpoint < XIVELYDATAPOINT_HUMIDITYPIXELABSMIN)
                    dewpoint = XIVELYDATAPOINT_HUMIDITYPIXELABSMIN;
                if(dewpoint < XIVELYDATAPOINT_HUMIDITYPIXELMIN-XIVELYDATAPOINT_HUMIDITYPIXELTHRESHOLD) {
                    pos = map(dewpoint, XIVELYDATAPOINT_HUMIDITYPIXELMIN-XIVELYDATAPOINT_HUMIDITYPIXELTHRESHOLD, XIVELYDATAPOINT_HUMIDITYPIXELABSMIN, XIVELYDATAPOINT_PIXELMIN, XIVELYDATAPOINT_PIXELMAX);
                    strip.setPixelColor(XIVELYDATAPOINT_HUMIDITYPIXEL-1, colorWheel(pos));
                    strip.show();
                } else {
                    pos = map(dewpoint, XIVELYDATAPOINT_HUMIDITYPIXELMAX+XIVELYDATAPOINT_HUMIDITYPIXELTHRESHOLD, XIVELYDATAPOINT_HUMIDITYPIXELABSMAX, XIVELYDATAPOINT_PIXELMIN, XIVELYDATAPOINT_PIXELMAX);
                    strip.setPixelColor(XIVELYDATAPOINT_HUMIDITYPIXEL-1, colorWheel(pos));
                    strip.show();
                }
            }
            
            //update xively values
    	    xivelydatastream_enabled_temperature = true;
    	    xivelydatastream_enabled_humidity = true;
			xivelydatapoint_temperaturetot += temperature;
            xivelydatapoint_temperaturetotcount++;
            xivelydatapoint_humiditytot += humidity;
            xivelydatapoint_humiditytotcount++;
        } else {
    	    #if UARTDEBUG == 1
            Serial.println("fail.");
            #endif
    	}
    }
    
    //check when to update the bh1750 readings
    if (millis()-bh1750_updateTimer > BH1750_UPDATETIMERINTERVALMS) {
        bh1750_updateTimer = millis();
        
        #if UARTDEBUG == 1
        Serial.print("Aquire BH1750 data... ");
        #endif
        
        //get values
        int brightness = (int)bh1750.lightLevel();
        
        #if UARTDEBUG == 1
        Serial.print("  Brightness: ");
        Serial.println(brightness);
        #endif
            
        //set pixel color
        if(brightness >= XIVELYDATAPOINT_BRIGHTNESSPIXELMIN-XIVELYDATAPOINT_BRIGHTNESSPIXELTHRESHOLD) {
            strip.setPixelColor(XIVELYDATAPOINT_BRIGHTNESSPIXEL-1, colorWheel(0));
            strip.show();
        } else {
            uint16_t pos = 0;
            if(brightness < XIVELYDATAPOINT_BRIGHTNESSPIXELABSMIN)
                brightness = XIVELYDATAPOINT_BRIGHTNESSPIXELABSMIN;
            pos = map(brightness, XIVELYDATAPOINT_BRIGHTNESSPIXELMIN-XIVELYDATAPOINT_BRIGHTNESSPIXELTHRESHOLD, XIVELYDATAPOINT_BRIGHTNESSPIXELABSMIN, XIVELYDATAPOINT_PIXELMIN, XIVELYDATAPOINT_PIXELMAX);
            strip.setPixelColor(XIVELYDATAPOINT_BRIGHTNESSPIXEL-1, colorWheel(pos));
            strip.show();
        }
                
        //update xively values
        xivelydatapoint_brightnesstot += brightness;
        xivelydatapoint_brightnesstotcount++;
                
	    #if UARTDEBUG == 1
        Serial.println("successful.");
        #endif
    }
    
    //check when to update the audio readings
    if (millis()-audioget_updateTimer > AUDIOGET_UPDATETIMERINTERVALMS) {
        audioget_updateTimer = millis();
        
        #if UARTDEBUG == 1
        Serial.print("Aquire Noise data... ");
        #endif
        
        //get values
        int audioRms = 0; //audio RMS value
        int noise = 0; //audio Spl value
        //get RMS value
    	audioRms = audioGet.getRms();
    	double voltageRms = AUDIO_VOLTREF;
    	if(audioRms > 0) {
    	    voltageRms = audioRms*ANALOGVOLTAGEREF/4096;
    	    if(voltageRms < AUDIO_VOLTREF) { //prevent values less than AUDIO_DBREF
    	        voltageRms = AUDIO_VOLTREF;
    	    }
    	}
        //get Spl value
    	noise = audioGet.getSpl(voltageRms, AUDIO_VOLTREF, AUDIO_DBREF);
	
	    #if UARTDEBUG == 1
	    Serial.print("  Noise RMS: ");
        Serial.println(audioRms);
        Serial.print("  Noise Spl: ");
        Serial.println(noise);
        #endif
        
	    //set pixel color
        if(noise <= XIVELYDATAPOINT_NOISEPIXELMAX+XIVELYDATAPOINT_NOISEPIXELTHRESHOLD) {
            strip.setPixelColor(XIVELYDATAPOINT_NOISEPIXEL-1, colorWheel(0));
            strip.show();
        } else {
            uint16_t pos = 0;
            if(noise > XIVELYDATAPOINT_NOISEPIXELABSMAX)
                noise = XIVELYDATAPOINT_NOISEPIXELABSMAX;
            pos = map(noise, XIVELYDATAPOINT_NOISEPIXELMAX+XIVELYDATAPOINT_NOISEPIXELTHRESHOLD, XIVELYDATAPOINT_NOISEPIXELABSMAX, XIVELYDATAPOINT_PIXELMIN, XIVELYDATAPOINT_PIXELMAX);
            strip.setPixelColor(XIVELYDATAPOINT_NOISEPIXEL-1, colorWheel(pos));
            strip.show();
        }
                
        //update xively values
        xivelydatapoint_noisetot += noise;
        xivelydatapoint_noisetotcount++;
                
	    #if UARTDEBUG == 1
        Serial.println("successful.");
        #endif
    }
}

// Update data to xively
void xivelyUpdate() {
    char numtemp[10];
	double d = 0;
	int datapointsIndex = 0;
	
	//set the datapoints array
	xivelyLib_datapoint datapoints[4];
	for(int i=0; i<sizeof(datapoints)/sizeof(datapoints[0]); i++) {
	    datapoints[i].enabled = false;
	    memset(datapoints[i].id, 0, sizeof(datapoints[i].id));
	    memset(datapoints[i].value, 0, sizeof(datapoints[i].value));
	}
	
    //set temperature datapoint
    if(xivelydatapoint_temperaturetotcount != 0) {
        d = (double)xivelydatapoint_temperaturetot/(double)xivelydatapoint_temperaturetotcount;
        memset(numtemp, 0, sizeof(numtemp));
    	sprintf(numtemp, "%3.1f", d);
    	datapoints[datapointsIndex].enabled = true;
        strncpy(datapoints[datapointsIndex].id, XIVELYDATAPOINT_TEMPERATURECHANNEL, strlen(XIVELYDATAPOINT_TEMPERATURECHANNEL));
        strncpy(datapoints[datapointsIndex].value, numtemp, strlen(numtemp));
        //reset sensor readings
        xivelydatapoint_temperaturetot = 0;
        xivelydatapoint_temperaturetotcount = 0;
    }
    //skip to next datapoints index
    datapointsIndex++;
    
    //set humidity datapoint
    if(xivelydatapoint_humiditytotcount != 0) {
        d = (double)xivelydatapoint_humiditytot/(double)xivelydatapoint_humiditytotcount;
        memset(numtemp, 0, sizeof(numtemp));
    	sprintf(numtemp, "%3.1f", d);
    	datapoints[datapointsIndex].enabled = true;
        strncpy(datapoints[datapointsIndex].id, XIVELYDATAPOINT_HUMIDITYCHANNEL, sizeof(XIVELYDATAPOINT_HUMIDITYCHANNEL));
        strncpy(datapoints[datapointsIndex].value, numtemp, sizeof(numtemp));
        //reset sensor readings
        xivelydatapoint_humiditytot = 0;
        xivelydatapoint_humiditytotcount = 0;
    }
    //skip to next datapoints index
    datapointsIndex++;
    
    //set brightness datapoint
    if(xivelydatapoint_brightnesstotcount != 0) {
        d = (double)xivelydatapoint_brightnesstot/(double)xivelydatapoint_brightnesstotcount;
        memset(numtemp, 0, sizeof(numtemp));
    	sprintf(numtemp, "%d", (int)d);
    	datapoints[datapointsIndex].enabled = true;
        strncpy(datapoints[datapointsIndex].id, XIVELYDATAPOINT_BRIGHTNESSCHANNEL, sizeof(XIVELYDATAPOINT_BRIGHTNESSCHANNEL));
        strncpy(datapoints[datapointsIndex].value, numtemp, sizeof(numtemp));
        //reset sensor readings
        xivelydatapoint_brightnesstot = 0;
        xivelydatapoint_brightnesstotcount = 0;
    }
    //skip to next datapoints index
    datapointsIndex++;
    
    //set noise datapoint
    if(xivelydatapoint_noisetotcount != 0) {
        d = (double)xivelydatapoint_noisetot/(double)xivelydatapoint_noisetotcount;
        memset(numtemp, 0, sizeof(numtemp));
    	sprintf(numtemp, "%d", (int)d);
    	datapoints[datapointsIndex].enabled = true;
        strncpy(datapoints[datapointsIndex].id, XIVELYDATAPOINT_NOISECHANNEL, sizeof(XIVELYDATAPOINT_NOISECHANNEL));
        strncpy(datapoints[datapointsIndex].value, numtemp, sizeof(numtemp));
        //reset sensor readings
        xivelydatapoint_noisetot = 0;
        xivelydatapoint_noisetotcount = 0;
    }
    //skip to next datapoints index
    datapointsIndex++;
    
    //update to xively
    xively.updateDatapoints(datapoints, sizeof(datapoints)/sizeof(datapoints[0]));
    
    //update xively status led
    if(xively.isUpdateSuccessful()) {
        digitalWrite(xively_statusLed, HIGH);
    } else {
        digitalWrite(xively_statusLed, LOW);
    }
}

// Turn off al led
void colorZero() {
    uint16_t i;
    for(i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
}

// Emit a rainbow to the led strip
void colorRainbow(uint8_t wait) {
    uint16_t i, j;
    
    for(j=0; j<256; j++) {
        for(i=0; i<strip.numPixels(); i++) {
            strip.setPixelColor(i, colorWheel((i+j) & 255));
        }
        strip.show();
        delay(wait);
    }
}

// Color selection, input a value 0 to 255 to get a color value.
uint32_t colorWheel(uint8_t wheelPos) {
    if(wheelPos < 85) {
        return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
    } else if(wheelPos < 170) {
        wheelPos -= 85;
        return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
    } else {
        wheelPos -= 170;
        return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
}

// Set config xively_feedid
int call_setFeedId(String feedId) {
    char feedIdc[XIVELYLIB_FEEDID_SIZE];
    feedId.toCharArray(feedIdc, sizeof(feedIdc));
    if(xively.validateFeedId(feedIdc)) {
        //update new value to eeprom
        feedId.toCharArray(config.xively_feedid, sizeof(config.xively_feedid));
        saveConfig();
        //update the value to xively
        xively.setFeedId(config.xively_feedid);
        return 1;
    } else {
        return 0;
    }
}

// Set config xively_apikey
int call_setApiKey(String apiKey) {
    char apiKeyc[XIVELYLIB_APIKEY_SIZE];
    apiKey.toCharArray(apiKeyc, sizeof(apiKeyc));
    if(xively.validateApiKey(apiKeyc)) {
        //update new value to eeprom
        apiKey.toCharArray(config.xively_apikey, sizeof(config.xively_apikey));
        saveConfig();
        //update the value to xively
        xively.setApiKey(config.xively_apikey);
        return 1;
    } else {
        return 0;
    }
}
