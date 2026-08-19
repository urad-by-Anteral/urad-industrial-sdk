/* Radar definitions */
#define RadarResetPin 6
/* Data UART baud rate of the flashed firmware variant: 9600 for
   uRAD_LevelSensing_*_9600_br.bin (recommended on Arduino), 115200 for
   *_115200_br.bin, 921600 for the standard *_921600_br.bin (needs a
   board whose UART supports it reliably). */
#define RadarDataBaudRate 9600
#define tlvHeaderLen 8
#define headerLen 36
#define magicWords_length 8

/* Auxiliary variables */
bool debugRadar = false, radarStatus = true;
uint16_t i, j;
uint32_t micros_0, micros_i, iterations, micros_start, micros_end, last_radarPacketReceived_ms, max_radarConnectionLost_ms = 2000;
float Fs_radar;

/* Radar protocol variables */
const uint8_t magicWords[] = {0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07};
uint8_t header[headerLen], payload[4096];
bool skipFrame, buffer_analyzing, magicWordsDetected, headerComplete;
uint32_t version, totalPacketLen, platform, frameNumber, timeCpuCycles, numDetectedObj, numTLVs;
uint16_t dataObjDescr_numDetectedObj, dataObjDescr_xyzQFormat;
/* Low halves of the three fixed-point ranges (always unsigned) and their
   high halves. The legacy sketch reused the out-of-box point struct names
   (rangeIdx/dopplerIdx/peakVal) and decoded r3_low as signed, reading
   range 3 62.5 mm short whenever its low word was >= 0x8000. */
uint16_t r1_low, r2_low, r3_low;
int16_t r1, r2, r3;
float range_meters_1, range_meters_2, range_meters_3, range_inches_1, range_inches_2, range_inches_3, factor_meterToInches = 0.0254;
uint32_t payloadBytesReaded, tlvType, tlvLength, payloadOffset, tid_i;
uint8_t headerBytesReaded, numOfTargets;
uint16_t numOfPoints;
uint32_t posX_i, posY_i, posZ_i, velX_i, velY_i, velZ_i, accX_i, accY_i, accZ_i;
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
  // put your setup code here, to run once:
  Serial1.begin(115200);
  Serial.begin(115200);
  // Reset radar
  pinMode(RadarResetPin, OUTPUT);
  digitalWrite(RadarResetPin, LOW);
  delay(10);
  digitalWrite(RadarResetPin, HIGH);
  delay(100);
  // Send radar configuration
  for (i=0; i<12; i++)
  {
    Serial1.print(RadarConfiguration[i]);
    delay(20);
  }
  Serial1.end();
  if (debugRadar)
  {
    micros_0 = micros();
    iterations = 0;
  }
  Serial1.begin(RadarDataBaudRate);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Radar frames
  if (debugRadar)
  {
    micros_start = micros();
  }
  skipFrame = false;
  buffer_analyzing = false;
  magicWordsDetected = false;
  headerComplete = false;
  headerBytesReaded = 0;

  while (true)
  {
    if (Serial1.available())
    {
      buffer_analyzing = true;
      if (!magicWordsDetected)
      {
        header[headerBytesReaded] = Serial1.read();
        //delayMicroseconds(625);
        if (header[headerBytesReaded] == magicWords[headerBytesReaded]) {
          headerBytesReaded++;
          if (headerBytesReaded == magicWords_length) {
            magicWordsDetected = true;
          }
        } else {
          skipFrame = true;
        }
      }
      else if (!headerComplete)
      {
        header[headerBytesReaded] = Serial1.read();
        headerBytesReaded++;
        if (headerBytesReaded == headerLen)
        {
          // parse header
          version = (header[11] << 24) | (header[10] << 16) | (header[9] << 8) | header[8];
          totalPacketLen = (header[15] << 24) | (header[14] << 16) | (header[13] << 8) | header[12];
          platform = (header[19] << 24) | (header[18] << 16) | (header[17] << 8) | header[16];
          frameNumber = (header[23] << 24)  | (header[22] << 16) | (header[21] << 8) | header[20];
          timeCpuCycles = (header[27] << 24)  | (header[26] << 16) | (header[25] << 8) | header[24];
          numDetectedObj = (header[31] << 24)  | (header[30] << 16) | (header[29] << 8) | header[28];
          numTLVs = (header[35] << 24)  | (header[34] << 16) | (header[33] << 8) | header[32];

          headerComplete = true;
          payloadBytesReaded = 0;
          
          if (debugRadar)
          {
            Serial.print("Packet size: ");
            Serial.print(totalPacketLen);
            Serial.print(" bytes, numTLVs: ");
            Serial.println(numTLVs);
          }
        }
      }
      else
      {
        payload[payloadBytesReaded] = Serial1.read();
        payloadBytesReaded++;
        if (payloadBytesReaded == totalPacketLen-headerLen)
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

          // parse packet payload
          payloadOffset = 0;
          for (j=0; j<numTLVs; j++)
          {
            tlvType = (payload[payloadOffset+3] << 24) | (payload[payloadOffset+2] << 16) | (payload[payloadOffset+1] << 8) | payload[payloadOffset+0];
            tlvLength = (payload[payloadOffset+7] << 24) | (payload[payloadOffset+6] << 16) | (payload[payloadOffset+5] << 8) | payload[payloadOffset+4];

            if (debugRadar)
            {
              Serial.print("TLV type: ");
              Serial.print(tlvType);
              Serial.print(" , TLV length: ");
              Serial.println(tlvLength);
            }
            if (tlvType > 20 || tlvLength > 10000)
            {
              break;
            }

            if (tlvType == 1)
            {
              dataObjDescr_numDetectedObj = (payload[payloadOffset+tlvHeaderLen+1] << 8) | payload[payloadOffset+tlvHeaderLen+0];
              dataObjDescr_xyzQFormat = (payload[payloadOffset+tlvHeaderLen+3] << 8) | payload[payloadOffset+tlvHeaderLen+2];

              r1_low = (payload[payloadOffset+tlvHeaderLen+5] << 8) | payload[payloadOffset+tlvHeaderLen+4];
              r3_low = (payload[payloadOffset+tlvHeaderLen+7] << 8) | payload[payloadOffset+tlvHeaderLen+6];
              r2_low = (payload[payloadOffset+tlvHeaderLen+9] << 8) | payload[payloadOffset+tlvHeaderLen+8];
              r1 = (payload[payloadOffset+tlvHeaderLen+11] << 8) | payload[payloadOffset+tlvHeaderLen+10];
              r2 = (payload[payloadOffset+tlvHeaderLen+13] << 8) | payload[payloadOffset+tlvHeaderLen+12];
              r3 = (payload[payloadOffset+tlvHeaderLen+15] << 8) | payload[payloadOffset+tlvHeaderLen+14];

              range_meters_1 = ((float)r1 * 65536.0 + (float)r1_low)/1048576.0;
              range_meters_2 = ((float)r2 * 65536.0 + (float)r2_low)/1048576.0;
              range_meters_3 = ((float)r3 * 65536.0 + (float)r3_low)/1048576.0;

              range_inches_1 = range_meters_1/factor_meterToInches;
              range_inches_2 = range_meters_2/factor_meterToInches;
              range_inches_3 = range_meters_3/factor_meterToInches;
              
              Serial.print("range_1: ");
              Serial.print(range_inches_1, 3);
              Serial.println(" inches");
              Serial.print("range_2: ");
              Serial.print(range_inches_2, 3);
              Serial.println(" inches");
              Serial.print("range_3: ");
              Serial.print(range_inches_3, 3);
              Serial.println(" inches");
              Serial.println(" ");
            }
            payloadOffset += (tlvLength + tlvHeaderLen);
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
    if (skipFrame || !buffer_analyzing)
    {
      break;
    }
  }
  if (millis()-last_radarPacketReceived_ms > max_radarConnectionLost_ms)
  {
    radarStatus = false;
  }

}
