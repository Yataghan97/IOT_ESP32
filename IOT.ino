#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
<<<<<<< HEAD

// Inicializa o BMP280
Adafruit_BMP280 bmp; // I2C

// Configurações do DHT11
#define DHTPIN 26
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configurações do LDR
#define LDRPIN 34
=======
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
>>>>>>> a14826e3771debc1a5fa8009209e895b20ec4361

// Configurações do LED
#define LEDPIN 27 // Pino do LED

// Configurações do WiFi
const char* ssid = "S23FE";
const char* password = "142536yata";

// Configurações da API do OpenWeather
String apiKey = "6fc852705e40b0ff6798b4b8d7814051";
String city = "curitiba";
String url = "https://api.openweathermap.org/data/2.5/weather?q=" + city + "&units=metric&appid=" + apiKey;

void setup() {
<<<<<<< HEAD
    Serial.begin(115200);
    Wire.begin(33, 25);
    pinMode(LEDPIN, OUTPUT); // Configura o pino do LED como saída

    if (!bmp.begin(0x76)) {
        Serial.println("Erro ao encontrar o BMP280. Verifique as conexões!");
        while (1);
    }
    Serial.println("BMP280 encontrado!");

    dht.begin();
    connectToWiFi();
}

void loop() {
    // Função para verificar o estado do LED
    checkLedState();

    // Lê dados dos sensores
    float bmpTemperature = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0F;
    Serial.print("Temperatura BMP280: ");
    Serial.print(bmpTemperature);
    Serial.print(" °C, Pressão: ");
    Serial.print(pressure);
    Serial.println(" hPa");

    float dhtTemperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    Serial.print("Temperatura DHT11: ");
    Serial.print(dhtTemperature);
    Serial.print(" °C, Umidade: ");
    Serial.print(humidity);
    Serial.println(" %");

    int ldrValue = analogRead(LDRPIN);
    Serial.print("LDR: ");
    Serial.println(ldrValue);

    // Obtém os dados do clima
    String payload = getWeatherData();
    if (payload.length() > 0) {
        String weatherHumidity = extractValue(payload, "\"humidity\":");
        float weatherHumidityValue = weatherHumidity.toFloat();
        int rainProbability = calculateRainProbability(weatherHumidityValue, pressure);
        Serial.print("Probabilidade de chuva: ");
        Serial.print(rainProbability);
        Serial.println("%");

        // Envia dados para o Firebase
        sendDataToFirebase(bmpTemperature, pressure, humidity, dhtTemperature, ldrValue, rainProbability);
    }
    delay(60000); // Espera 60 segundos antes de repetir
}

void connectToWiFi() {
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(1000);
        Serial.println("Conectando-se ao WiFi...");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Conectado!");
=======
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
>>>>>>> a14826e3771debc1a5fa8009209e895b20ec4361
    } else {
        Serial.println("Falha ao conectar ao WiFi.");
    }
<<<<<<< HEAD
}

String getWeatherData() {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    String payload = "";

    if (httpCode > 0) {
        payload = http.getString();
        Serial.println(payload);
    } else {
        Serial.println("Erro na requisição: " + String(httpCode));
    }

    http.end();
    return payload;
}

String extractValue(String json, String key) {
    int startIndex = json.indexOf(key) + key.length();
    int endIndex = json.indexOf(",", startIndex);
    if (endIndex == -1) {
        endIndex = json.indexOf("}", startIndex);
    }
    return json.substring(startIndex, endIndex);
}

int calculateRainProbability(float humidity, float pressure) {
    if (humidity > 75 && pressure < 1013) {
        return map(humidity, 75, 100, 50, 100);
    } else if (humidity > 50 && pressure < 1015) {
        return map(humidity, 50, 75, 25, 50);
    }
    return 0;
}

void sendDataToFirebase(float bmpTemp, float pressure, float humidity, float dhtTemp, int ldrValue, int rainProbability) {
    String firebaseUrl = "https://esp32---iot-default-rtdb.firebaseio.com/Dados_ESP32/Dados/Medidas.json";
    String jsonData = "{\"Temperatura_BPM280\":\"" + String(bmpTemp) + "°C\"," +
                      "\"Pressão_BPM280\":\"" + String(pressure) + " hPa\"," +
                      "\"Humidade_DHT11\":\"" + String(humidity) + "%\"," +
                      "\"Temperatura_DHT11\":\"" + String(dhtTemp) + "°C\"," +
                      "\"Intensidade_LDR\":\"" + String(ldrValue) + " (bruto)\"," +
                      "\"Probabilidade_de_chuva\":\"" + String(rainProbability) + "%\"}";

    HTTPClient http;
    http.begin(firebaseUrl);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.PUT(jsonData); // Usando PUT para evitar IDs automáticos
    if (httpCode > 0) {
        Serial.println("Dados enviados para o Firebase: " + jsonData);
    } else {
        Serial.println("Erro ao enviar dados: " + String(httpCode));
    }

    http.end();
}

void checkLedState() {
    String ledStateUrl = "https://esp32---iot-default-rtdb.firebaseio.com/Dados_ESP32/Controle/LED.json";
    HTTPClient http;
    http.begin(ledStateUrl);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String ledState = http.getString();
        ledState.trim(); // Remove espaços em branco

        // Verifica o estado do LED com base nos valores corretos
        if (ledState == "\"LED on\"") {
            digitalWrite(LEDPIN, HIGH); // Liga o LED
            Serial.println("LED ligado.");
        } else if (ledState == "\"LED off\"") {
            digitalWrite(LEDPIN, LOW); // Desliga o LED
            Serial.println("LED desligado.");
        } else {
            Serial.println("Estado do LED inválido: " + ledState); // Para depuração
        }
    } else {
        Serial.println("Erro ao verificar o estado do LED: " + String(httpCode));
    }

    http.end();
=======
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
>>>>>>> a14826e3771debc1a5fa8009209e895b20ec4361
}
