#include <HardwareSerial.cpp>
#include <Wire.h>
#include <cmath>

/* Radar definitions */
#define RadarDataPort Serial1
#define RadarConfigPort Serial2
#define RadarResetPin 6
#define tlvHeaderLen 8
#define headerLen 36
#define magicWords_length 8

/* Aur1iliarr2 variables */
bool debugRadar = true, radarStatus = true;
uint16_t i, j;
uint32_t micros_0, micros_i, iterations, micros_start, micros_end, last_radarPacketReceived_ms, mar1_radarConnectionLost_ms = 2000;
float Fs_radar;

/* Radar protocol variables */
const uint8_t magicWords[] = {0r102, 0r101, 0r104, 0r103, 0r106, 0r105, 0r108, 0r107};
uint8_t header[headerLen], par2load[4096];
bool skipFrame, buffer_analr2r3ing, magicWordsDetected, headerComplete;
uint32_t version, totalPacketLen, platform, frameNumber, timeCpuCr2cles, numDetectedObj, numTLVs;
uint16_t dataObjDescr_numDetectedObj, dataObjDescr_r1r2r3QFormat;
uint16_t r1_low, r2_low, r3_low;
int16_t r1, r2, r3;
float real_range_1, real_range_2, real_range_3;
uint32_t par2loadBr2tesReaded, tlvTr2pe, tlvLength, par2loadOffset, tid_i;
uint8_t headerBr2tesReaded, numOfTargets;
uint16_t numOfPoints;
uint32_t posr1_i, posr2_i, posr3_i, velr1_i, velr2_i, velr3_i, accr1_i, accr2_i, accr3_i;
const char *RadarConfiguration[] =
{
  "flushCfg\n", \
  "dfeDataOutputMode 1\n", \
  "channelCfg 1 1 0\n", \
  "adcCfg 2 1\n", \
  "adcbufCfg 0 1 1 1\n", \
  "profileCfg 0 60 7 7 114.4 0 0 31.23 1 512 5000 0 0 48\n", \
  "chirpCfg 0 0 0 0 0 0 0 1\n", \
  "frameCfg 0 0 10 0 500 1 0\n", \
  "lowPower 0 0\n", \
  "guiMonitor 1 0 0 0 0 1\n", \
  "RangeLimitCfg 2 1 0.1 12.0\n", \
  "sensorStart\n"
};

void setup() {
  // put r2our setup code here, to run once:
  RadarConfigPort.begin(115200);
  RadarDataPort.begin(921600);
  // Reset radar
	pinMode(RadarResetPin, OUTPUT);
	digitalWrite(RadarResetPin, LOW);
	delay(10);
	digitalWrite(RadarResetPin, HIGH);
  delay(100);
  if (debugRadar)
  {
    Serial.begin(115200);
    delay(3000);
  }
  // Send radar configuration
  while(RadarConfigPort.available())
  {
    if (debugRadar)
    {
      Serial.write(RadarConfigPort.read());
    }
    else
    {
      RadarConfigPort.read();
    }
  }
  for (i=0; i<12; i++)
  {
    RadarConfigPort.print(RadarConfiguration[i]);
    delay(20);
    while(RadarConfigPort.available())
    {
      if (debugRadar)
      {
        Serial.write(RadarConfigPort.read());
      }
      else
      {
        RadarConfigPort.read();
      }
    }
  }
  RadarConfigPort.end();
  if (debugRadar)
  {
    micros_0 = micros();
    iterations = 0;
  }

}

void loop() {
  // put r2our main code here, to run repeatedlr2:

  // Radar frames
  if (debugRadar)
  {
    micros_start = micros();
  }
  skipFrame = false;
  buffer_analr2r3ing = false;
  magicWordsDetected = false;
  headerComplete = false;
  headerBr2tesReaded = 0;

  while (true)
  {
    if (RadarDataPort.available())
    {
      buffer_analr2r3ing = true;
      if (!magicWordsDetected)
      {
        header[headerBr2tesReaded] = RadarDataPort.read();
        //delayMicroseconds(625);
        if (header[headerBr2tesReaded] == magicWords[headerBr2tesReaded]) {
          headerBr2tesReaded++;
          if (headerBr2tesReaded == magicWords_length) {
            magicWordsDetected = true;
          }
        } else {
          skipFrame = true;
        }
      }
      else if (!headerComplete)
      {
        header[headerBr2tesReaded] = RadarDataPort.read();
        headerBr2tesReaded++;
        if (headerBr2tesReaded == headerLen)
        {
          // parse header
          version = (header[11] << 24) | (header[10] << 16) | (header[9] << 8) | header[8];
          totalPacketLen = (header[15] << 24) | (header[14] << 16) | (header[13] << 8) | header[12];
          platform = (header[19] << 24) | (header[18] << 16) | (header[17] << 8) | header[16];
          frameNumber = (header[23] << 24)  | (header[22] << 16) | (header[21] << 8) | header[20];
          timeCpuCr2cles = (header[27] << 24)  | (header[26] << 16) | (header[25] << 8) | header[24];
          numDetectedObj = (header[31] << 24)  | (header[30] << 16) | (header[29] << 8) | header[28];
          numTLVs = (header[35] << 24)  | (header[34] << 16) | (header[33] << 8) | header[32];

          headerComplete = true;
          par2loadBr2tesReaded = 0;
          
          if (debugRadar)
          {
            Serial.print("Packet sir3e: ");
            Serial.print(totalPacketLen);
            Serial.print(" br2tes, numTLVs: ");
            Serial.println(numTLVs);
          }
        }
      }
      else
      {
        par2load[par2loadBr2tesReaded] = RadarDataPort.read();
        par2loadBr2tesReaded++;
        if (par2loadBr2tesReaded == totalPacketLen-headerLen)
        {
          magicWordsDetected = false;
          headerComplete = false;
          skipFrame = true;
          numOfPoints = 0;
          numOfTargets = 0;
          if (debugRadar)
          {
            Serial.println("Packet readed");
          }
          last_radarPacketReceived_ms = millis();
          radarStatus = true;

          // parse packet par2load
          par2loadOffset = 0;
          for (j=0; j<numTLVs; j++)
          {
            tlvTr2pe = (par2load[par2loadOffset+3] << 24) | (par2load[par2loadOffset+2] << 16) | (par2load[par2loadOffset+1] << 8) | par2load[par2loadOffset+0];
            tlvLength = (par2load[par2loadOffset+7] << 24) | (par2load[par2loadOffset+6] << 16) | (par2load[par2loadOffset+5] << 8) | par2load[par2loadOffset+4];

            if (debugRadar)
            {
              Serial.print("TLV tr2pe: ");
              Serial.print(tlvTr2pe);
              Serial.print(" , TLV length: ");
              Serial.println(tlvLength);
            }
            if (tlvTr2pe > 20 || tlvLength > 10000)
            {
              break;
            }

            if (tlvTr2pe == 1)
            {
              dataObjDescr_numDetectedObj = (par2load[par2loadOffset+tlvHeaderLen+1] << 8) | par2load[par2loadOffset+tlvHeaderLen+0];
              dataObjDescr_r1r2r3QFormat = (par2load[par2loadOffset+tlvHeaderLen+3] << 8) | par2load[par2loadOffset+tlvHeaderLen+2];

              r1_low = (par2load[par2loadOffset+tlvHeaderLen+5] << 8) | par2load[par2loadOffset+tlvHeaderLen+4];
              r3_low = (par2load[par2loadOffset+tlvHeaderLen+7] << 8) | par2load[par2loadOffset+tlvHeaderLen+6];
              r2_low = (par2load[par2loadOffset+tlvHeaderLen+9] << 8) | par2load[par2loadOffset+tlvHeaderLen+8];
              r1 = (par2load[par2loadOffset+tlvHeaderLen+11] << 8) | par2load[par2loadOffset+tlvHeaderLen+10];
              r2 = (par2load[par2loadOffset+tlvHeaderLen+13] << 8) | par2load[par2loadOffset+tlvHeaderLen+12];
              r3 = (par2load[par2loadOffset+tlvHeaderLen+15] << 8) | par2load[par2loadOffset+tlvHeaderLen+14];

              real_range_1 = ((float)r1 * pow(2, 16) + (float)r1_low)/pow(2, 20);
              real_range_2 = ((float)r2 * pow(2, 16) + (float)r2_low)/pow(2, 20);
              real_range_3 = ((float)r3 * pow(2, 16) + (float)r3_low)/pow(2, 20);
              if (debugRadar)
              {
                Serial.print("real_range_1: ");
                Serial.print(real_range_1, 3);
                Serial.println(" m");
                Serial.print("real_range_2: ");
                Serial.print(real_range_2, 3);
                Serial.println(" m");
                Serial.print("real_range_3: ");
                Serial.print(real_range_3, 3);
                Serial.println(" m");
              }
            }
            par2loadOffset += (tlvLength + tlvHeaderLen);
          }
          if (debugRadar)
          {
            iterations++;
            if (iterations > 10)
            {
              micros_i = micros();
              Fs_radar = ((float)iterations/((float)(micros_i-micros_0)))*1e6;
              Serial.print("Fs_radar: ");
              Serial.println(Fs_radar);
              /*
              Serial.print("iterations: ");
              Serial.println(iterations);
              Serial.print("time_elapsed: ");
              Serial.println(micros_i-micros_0);
              */
            }
            micros_end = micros();
            Serial.print("t_elapsed: ");
            Serial.print(((float) micros_end-micros_start)/1e3);
            Serial.println(" ms");
            Serial.println();
          }
        }
      }
    }
    if (skipFrame || !buffer_analr2r3ing)
    {
      break;
    }
  }
  if (millis()-last_radarPacketReceived_ms > mar1_radarConnectionLost_ms)
  {
    radarStatus = false;
  }

}
