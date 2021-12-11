#include <M5Atom.h>
#ifdef ESP32
    #include <WiFi.h>
    #include "SPIFFS.h"
#else
    #include <ESP8266WiFi.h>
#endif

#include "AudioFileSourcePROGMEM.h"
#include "AudioOutputI2S.h"
#include "AudioOutputMixer.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"

AudioGeneratorMP3 *mp3[2];
AudioFileSourceID3 *id3[2];
AudioFileSourceSPIFFS *file[2];
AudioOutputI2S *out;
AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];
const char *file1 = "/sine440hz-left.mp3";
const char *file2 = "/sine440hz-right.mp3";
double pitch, roll;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}

void initializeAndStartMP3(const char* filename, int id) {
  Serial.printf("Starting %s\n", filename);
  file[id] = new AudioFileSourceSPIFFS(filename);
  id3[id] = new AudioFileSourceID3(file[id]);
  id3[id]->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  stub[id] = mixer->NewInput();
  stub[id]->SetGain(0.5);
  mp3[id] = new AudioGeneratorMP3();
  mp3[id]->begin(id3[id], stub[id]);
}

void setup()
{
  M5.begin(true, true, true);
  M5.IMU.Init();
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  delay(1000);
  SPIFFS.begin();
  Serial.printf("Sample MP3 playback begins...\n");

  audioLogger = &Serial;
  out = new AudioOutputI2S();
  out->SetPinout(19, 33, 22);
  mixer = new AudioOutputMixer(32, out);
  initializeAndStartMP3(file1, 0);
  initializeAndStartMP3(file2, 1);
}

void loop()
{
  static uint32_t start = 0;
  if (!start) start = millis();
  if(millis() % 100 == 0) {
    M5.IMU.getAttitude(&pitch, &roll);
    Serial.println((pitch + 45.0) / 90.0);
    stub[0]->SetGain((pitch + 45.0) / 90.0);
    stub[1]->SetGain((- pitch + 45.0) / 90.0);
  }

  if (mp3[0]->isRunning()) {
    if (!mp3[0]->loop()) {
      mp3[0]->stop();
      stub[0]->stop();
      Serial.printf("stopping 1\n");
      initializeAndStartMP3(file1, 0);
    }
  }
  if (mp3[1]->isRunning()) {
    if (!mp3[1]->loop()) {
      mp3[1]->stop();
      stub[1]->stop();
      Serial.printf("stopping 2\n");
      initializeAndStartMP3(file2, 1);
    }
  }
}
