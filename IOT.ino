#include <WiFi.h>
#include <HTTPClient.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>  // Biblioteca para BMP280

// Definindo as credenciais do WiFi
#define WIFI_SSID "S23FE"
#define WIFI_PASSWORD "142536yata"

// Firebase configuração
#define FIREBASE_HOST "esp32---iot-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "FQaDDfSKibS2tczPyoWIUZUQjUPoQ4tFEQihNnV5"

// OpenWeatherMap API Key e cidade
#define API_KEY "db75e961cb494cd9bb330d9965781719"
#define CITY "Curitiba"
#define WEATHER_URL "http://api.openweathermap.org/data/2.5/weather?q=" CITY "&appid=" API_KEY "&units=metric"

// Definindo pinos dos sensores
#define DHTPIN 26
#define DHTTYPE DHT11
#define LDRPIN 34   // Exemplo de pino para o LDR

// Instâncias dos sensores
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp; // Instanciando sensor de pressão BMP280

// Variáveis para armazenar dados dos sensores e do OpenWeatherMap
float localTemperature, localHumidity, pressure, ldrValue;
float apiTemperature, apiPressure, rainChance;

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

void setup() {
  Serial.begin(115200);
  
  // Inicializando o sensor DHT11 e o sensor de pressão
  dht.begin();
  if (!bmp.begin(0x76)) {  // Endereço padrão I2C do BMP280
    Serial.println("Falha ao inicializar o sensor de pressão BMP280!");
    while (1);
  }

  // Conectando ao Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando-se ao Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Conectado ao Wi-Fi!");

  // Configuração do Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  // Leitura dos dados do DHT11 e do LDR
  localTemperature = dht.readTemperature();
  localHumidity = dht.readHumidity();
  ldrValue = analogRead(LDRPIN);
  pressure = bmp.readPressure() / 100.0F; // Converte para hPa

  // Verificação das leituras do DHT11 e BMP280
  if (isnan(localTemperature) || isnan(localHumidity)) {
    Serial.println("Falha ao ler do sensor DHT11!");
    return;
  }
  if (isnan(pressure)) {
    Serial.println("Falha ao ler do sensor de pressão!");
    return;
  }

  // Exibindo os dados dos sensores locais
  Serial.print("Temperatura local: ");
  Serial.print(localTemperature);
  Serial.print("°C | Umidade local: ");
  Serial.print(localHumidity);
  Serial.print("% | Pressão local: ");
  Serial.print(pressure);
  Serial.print(" hPa | Luminosidade: ");
  Serial.println(ldrValue);

  // Enviando dados do OpenWeatherMap
  if (WiFi.status() == WL_CONNECTED) {
    if (fetchWeatherData()) {
      sendDataToFirebase(localTemperature, localHumidity, ldrValue, pressure, apiTemperature, apiPressure, rainChance);
      checkRainCondition();
    }
  }

  // Aguardar antes da próxima leitura
  delay(60000); // 1 minuto entre atualizações
}

bool fetchWeatherData() {
  HTTPClient http;
  http.begin(WEATHER_URL);  // URL da API
  int httpCode = http.GET(); // Fazendo a requisição GET

  // Se o código de resposta for positivo (dados recebidos corretamente)
  if (httpCode > 0) {
    String payload = http.getString();  // Obter o corpo da resposta em JSON
    Serial.println(payload);            // Exibir a resposta no serial

    // Parseando os dados JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Extrair dados relevantes do clima
      apiTemperature = doc["main"]["temp"]; // Temperatura (em °C)
      apiPressure = doc["main"]["pressure"]; // Pressão atmosférica (em hPa)
      rainChance = doc.containsKey("rain") ? doc["rain"]["1h"] : 0; // Chuva nas últimas 1 hora (mm)

      Serial.print("Temperatura do OpenWeather: ");
      Serial.print(apiTemperature);
      Serial.print("°C | Pressão do OpenWeather: ");
      Serial.print(apiPressure);
      Serial.print(" hPa | Chuva prevista (última 1h): ");
      Serial.println(rainChance);

      http.end(); // Finalizando a conexão
      return true; // Sucesso ao obter dados do clima
    } else {
      Serial.println("Falha ao analisar os dados JSON");
    }
  } else {
    Serial.print("Erro ao se conectar: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();  // Finalizando a conexão
  return false; // Falha ao obter dados do clima
}

void sendDataToFirebase(float localTemperature, float localHumidity, float ldrValue, float pressure, float apiTemperature, float apiPressure, float rainChance) {
  // Criando estrutura JSON para enviar
  FirebaseJson json;
  json.set("localTemperature", localTemperature);
  json.set("localHumidity", localHumidity);
  json.set("ldrValue", ldrValue);
  json.set("pressure", pressure);
  json.set("apiTemperature", apiTemperature);
  json.set("apiPressure", apiPressure);
  json.set("rainChance", rainChance);
  
  // Enviando dados para o Firebase
  if (Firebase.setJSON(firebaseData, "/sensores", json)) {
    Serial.println("Dados enviados ao Firebase com sucesso!");
  } else {
    Serial.print("Erro ao enviar dados para o Firebase: ");
    Serial.println(firebaseData.errorReason());
  }
}

void checkRainCondition() {
  // Condição para chuva com base em pressão
  if (pressure < 1005 && rainChance > 0) {
    Serial.println("Alerta: Condição de chuva detectada!");
    // Ação adicional pode ser tomada, como enviar uma notificação
  } else {
    Serial.println("Sem condições de chuva.");
  }
}
