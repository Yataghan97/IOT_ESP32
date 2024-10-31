#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Inicializa o BMP280
Adafruit_BMP280 bmp; // I2C

// Configurações do DHT11
#define DHTPIN 26
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configurações do LDR
#define LDRPIN 34

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
    } else {
        Serial.println("Falha ao conectar ao WiFi.");
    }
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
}
