// Blynk Smart Remote AC
#define BLYNK_TEMPLATE_ID "TMPL6yHgcnZnh"
#define BLYNK_TEMPLATE_NAME "SMART REMOTE AC"
#define BLYNK_AUTH_TOKEN "kUsRi2-6Y1ODEbigLatWsGjjKNYTbtqe"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHTesp.h>
#include <IRremoteESP8266.h>
#include <ir_Gree.h>
#include <IRsend.h>
#include <ir_panasonic.h>

char ssid[] = "POCO M4 Pro";
char password[] = "123456790";

// IR setup
const uint16_t kIrLed = 14;  // Pin IR LED
IRGreeAC greeAC(kIrLed);    
IRPanasonicAc panasonicAC(kIrLed);     
#define DHTPIN 4
DHTesp dht;

// Variabel untuk memilih merek AC
enum ACBrand { AC_GREE, AC_PANASONIC, AC_UNKNOWN };
ACBrand currentAC = AC_GREE;

// Variabel global untuk menyimpan nilai sebelumnya
float lastTemperature = 0.0;
float lastHumidity = 0.0;
const float threshold = 0.5; // Batas perubahan nilai suhu/kelembapan
const unsigned long sendInterval = 30000; // Interval pengiriman data ke Blynk (30 detik)

// Fungsi untuk mengirim konfigurasi IR
void sendState() {
  Serial.println("Sending AC State ...");
  if (currentAC == AC_GREE) {
    Serial.println("Sending Gree AC signal...");
    greeAC.send();  // Kirim kode IR sesuai state
    Serial.println(greeAC.toString().c_str());
  } else if (currentAC == AC_PANASONIC) {
    Serial.println("Sending Panasonic AC signal...");
    panasonicAC.send();  // Kirim kode IR sesuai state
    Serial.println(panasonicAC.toString().c_str());
  } else {
    Serial.println("Unknown AC Brand!");
  }
}

// Fungsi untuk mengirim data suhu ke Blynk
void sendTemperatureData() {
  static unsigned long lastSendTime = 0;
  unsigned long currentMillis = millis();

  // Kirim data hanya jika interval waktu terpenuhi
  if (currentMillis - lastSendTime >= sendInterval) {
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  // Print hasil di Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");

 // Cek perubahan signifikan
    if (abs(temperature - lastTemperature) >= threshold || abs(humidity - lastHumidity) >= threshold) {
      // Kirim ke Blynk hanya jika ada perubahan signifikan
      Serial.print("Sending to Blynk - Temperature: ");
      Serial.print(temperature);
      Serial.print(" *C, Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

  // Kirim ke Blynk
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  // Simpan nilai terbaru
      lastTemperature = temperature;
      lastHumidity = humidity;
      lastSendTime = currentMillis;
  } else {
    Serial.println("No significant change in temperature/humidity");
  }
}
}

// BLYNK VIRTUAL PIN CONTROL
BLYNK_WRITE(V2) {  // Power ON/OFF
  int powerState = param.asInt();
  if (currentAC == AC_GREE) {
    if (powerState) greeAC.on();
    else greeAC.off();
  } else if (currentAC == AC_PANASONIC) {
    if (powerState) panasonicAC.on();
    else panasonicAC.off();
  }
  sendState();
}

BLYNK_WRITE(V3) {  // Suhu (16-30°C)
  int temperature = param.asInt();
  if (currentAC == AC_GREE) greeAC.setTemp(temperature);
  else if (currentAC == AC_PANASONIC) panasonicAC.setTemp(temperature);
  sendState();
}

BLYNK_WRITE(V4) {  // Kecepatan kipas (0-3)
  int fanSpeed = param.asInt();
  if (currentAC == AC_GREE) greeAC.setFan(fanSpeed);
  else if (currentAC == AC_PANASONIC) panasonicAC.setFan(fanSpeed);
  sendState();
}

BLYNK_WRITE(V5) {  // Mode (Cool, Dry, Fan)
  int mode = param.asInt();
  if (currentAC == AC_GREE) {
    switch (mode) {
      case 1: greeAC.setMode(kGreeCool); break;
      case 2: greeAC.setMode(kGreeDry); break;
      case 3: greeAC.setMode(kGreeFan); break;
    }
  } else if (currentAC == AC_PANASONIC) {
    switch (mode) {
      case 1: panasonicAC.setMode(kPanasonicAcCool); break;
      case 2: panasonicAC.setMode(kPanasonicAcDry); break;
      case 3: panasonicAC.setMode(kPanasonicAcFan); break;
    }
  }
  sendState();
}

BLYNK_WRITE(V6) {  // Virtual Pin V6 untuk memilih merek AC
  int selectedAC = param.asInt();  // Baca nilai dari tombol/switch di Blynk

  switch (selectedAC) {
    case 0:
      currentAC = AC_GREE;
      Serial.println("Gree AC Selected!");
      break;
    case 1:
      currentAC = AC_PANASONIC;
      Serial.println("Panasonic AC Selected!");
      break;
    default:
      currentAC = AC_UNKNOWN;
      Serial.println("Unknown AC Selected!");
      break;
  }
}

void setup() {
  // Inisialisasi Serial
  Serial.begin(115200);

  // Koneksi ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Inisialisasi Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

  // Inisialisasi DHT
  dht.setup(DHTPIN, DHTesp::DHT22);

  greeAC.begin();
  greeAC.off();  // Set default state ke ON
  greeAC.setMode(kGreeCool);
  Serial.println("IR Gree ready.");
  Serial.println("AC Remote Ready!");

  panasonicAC.begin();
  panasonicAC.off();  // Set default state ke ON
  panasonicAC.setMode(kPanasonicAcCool);
  Serial.println("IR Panasonic ready.");
  Serial.println("AC Remote Ready!");
}

void loop() {
  Blynk.run();
  static unsigned long lastDHTReadTime = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastDHTReadTime >= 1500) {
    lastDHTReadTime = currentMillis;
    sendTemperatureData();
  }
}