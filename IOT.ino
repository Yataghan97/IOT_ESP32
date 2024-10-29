#include <WiFi.h>
#include <HTTPClient.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

#define WIFI_SSID "S23FE"
#define WIFI_PASSWORD "142536yata"

#define FIREBASE_HOST "esp32---iot-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "FQaDDfSKibS2tczPyoWIUZUQjUPoQ4tFEQihNnV5"

#define API_KEY "db75e961cb494cd9bb330d9965781719"
#define CITY "Curitiba"
#define WEATHER_URL "http://api.openweathermap.org/data/2.5/weather?q=" CITY "&appid=" API_KEY "&units=metric"

#define DHTPIN 26
#define DHTTYPE DHT11
#define LDRPIN 34

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

float localTemperature, localHumidity, pressure, ldrValue;
float apiTemperature, apiPressure, rainChance;

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

void setup() {
  Serial.begin(115200);
  
  dht.begin();
  if (!bmp.begin(0x76)) {
    Serial.println("Falha ao inicializar o sensor de pressão BMP280!");
    while (1);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando-se ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado ao Wi-Fi!");

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  localTemperature = dht.readTemperature();
  localHumidity = dht.readHumidity();
  ldrValue = analogRead(LDRPIN);
  pressure = bmp.readPressure() / 100.0F;

  if (isnan(localTemperature) || isnan(localHumidity)) {
    Serial.println("Falha ao ler do sensor DHT11!");
    return;
  }
  if (isnan(pressure)) {
    Serial.println("Falha ao ler do sensor de pressão!");
    return;
  }

  Serial.print("Temperatura local: ");
  Serial.print(localTemperature);
  Serial.print("°C | Umidade local: ");
  Serial.print(localHumidity);
  Serial.print("% | Pressão local: ");
  Serial.print(pressure);
  Serial.print(" hPa | Luminosidade: ");
  Serial.println(ldrValue);

  if (WiFi.status() == WL_CONNECTED) {
    if (fetchWeatherData()) {
      sendDataToFirebase(localTemperature, localHumidity, ldrValue, pressure, apiTemperature, apiPressure, rainChance);
      checkRainCondition();
    }
  }

  delay(60000);
}

bool fetchWeatherData() {
  HTTPClient http;
  http.begin(WEATHER_URL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      apiTemperature = doc["main"]["temp"];
      apiPressure = doc["main"]["pressure"];
      rainChance = doc.containsKey("rain") ? doc["rain"]["1h"] : 0;

      Serial.print("Temperatura do OpenWeather: ");
      Serial.print(apiTemperature);
      Serial.print("°C | Pressão do OpenWeather: ");
      Serial.print(apiPressure);
      Serial.print(" hPa | Chuva prevista (última 1h): ");
      Serial.println(rainChance);

      http.end();
      return true;
    } else {
      Serial.println("Falha ao analisar os dados JSON");
    }
  } else {
    Serial.print("Erro ao se conectar: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
  return false;
}

void sendDataToFirebase(float localTemperature, float localHumidity, float ldrValue, float pressure, float apiTemperature, float apiPressure, float rainChance) {
  FirebaseJson json;
  json.set("localTemperature", localTemperature);
  json.set("localHumidity", localHumidity);
  json.set("ldrValue", ldrValue);
  json.set("pressure", pressure);
  json.set("apiTemperature", apiTemperature);
  json.set("apiPressure", apiPressure);
  json.set("rainChance", rainChance);
  
  if (Firebase.setJSON(firebaseData, "/sensores", json)) {
    Serial.println("Dados enviados ao Firebase com sucesso!");
  } else {
    Serial.print("Erro ao enviar dados para o Firebase: ");
    Serial.println(firebaseData.errorReason());
  }
}

void checkRainCondition() {
  if (pressure < 1005 && rainChance > 0) {
    Serial.println("Alerta: Condição de chuva detectada!");
  } else {
    Serial.println("Sem condições de chuva.");
  }
}
