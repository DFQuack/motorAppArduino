// ======= Sensor ultrasonido + motor DC con L293D =======
#include <NewPing.h>
#include <UnoR4WiFi_WebServer.h>
#include "page.h"

// ---------- Pines ----------
int PIN_TRIG = 10, PIN_ECHO = 9, L293_IN1 = 5, L293_IN2 = 3;

// ---------- Parámetros del sensor ----------
int distMin = 10, distDerecha = 20, distIzquierda = 30;
NewPing sonar(PIN_TRIG, PIN_ECHO, 30);

// ---------- Parámetros del motor ----------
bool modoManual = true;
String giro;

// ---------- Datos de red ----------
char ssid[] = "aaaa";
char pass[] = "holahola";
int keyIndex = 0;
UnoR4WiFi_WebServer server;

// ---------- Control del motor ----------
void girarDerecha() {
  digitalWrite(L293_IN1, HIGH);
  digitalWrite(L293_IN2, LOW);
  giro = "derecha";
}

void girarIzquierda() {
  digitalWrite(L293_IN1, LOW);
  digitalWrite(L293_IN2, HIGH);
  giro = "izquierda";
}

void detenerMotor() {
  digitalWrite(L293_IN1, LOW);
  digitalWrite(L293_IN2, LOW);
  giro = "detenido";
}

unsigned int getDistancia() {
  unsigned int tiempo = sonar.ping();
  unsigned int distancia = tiempo / US_ROUNDTRIP_CM;

  return distancia;
}

void motorSensor() {
  unsigned int distancia = getDistancia();
  Serial.println(distancia + " cm");
  if (distancia == 0 && distancia < distMin) {
    detenerMotor();
  } else if (distancia >= distMin && distancia < distDerecha) {
    girarDerecha();
  } else if (distancia >= distDerecha && distancia <= distIzquierda) {
    girarIzquierda();
  } else {
    detenerMotor();
  }

  delay(100);
}

// ---------- Handlers de la página ----------

String getResponse() {
  String response = HTML_CONTENT;

  // Dirección de giro
  String estadoGiro;
  if (giro == "derecha") {
    estadoGiro = "Motor girando a la derecha";
  } else if (giro == "izquierda") {
    estadoGiro = "Motor girando a la izquierda";
  } else {
    estadoGiro = "Motor detenido";
  }
  Serial.println(estadoGiro);
  response.replace("GIRO", estadoGiro);

  // Estado de modo
  String estadoModo, estadoDist;
  if (modoManual) {
    estadoModo = "Modo sensor desactivado";
    estadoDist = "No disponible";
  } else {
    estadoModo = "Modo sensor activado";
    estadoDist = getDistancia() + " cm";
  }
  Serial.println(estadoModo);
  Serial.print("Distancia medida: ");
  Serial.println(estadoDist);

  response.replace("MODO", estadoModo);
  response.replace("DISTANCIA", estadoDist);

  return response;
}

void handleIndex(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData) {
  String response = getResponse();
  server.sendResponse(client, response.c_str());
}

void handleMotorIzquierda(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData) {
  if (modoManual) {
    girarIzquierda();
  } else {
    Serial.println("Para controlar la dirección de giro active el modo manual.");
  }
  String response = getResponse();
  server.sendResponse(client, response.c_str());
}

void handleMotorDerecha(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData) {
  if (modoManual) {
    girarDerecha();
  } else {
    Serial.println("Para controlar la dirección de giro active el modo manual.");
  }
  String response = getResponse();
  server.sendResponse(client, response.c_str());
}

void handleMotorParar(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData) {
  detenerMotor();
  String response = getResponse();
  server.sendResponse(client, response.c_str());
}

void handleModo(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData) {
  if (modoManual) {
    modoManual = false;
    detenerMotor();
    while (modoManual) {
      motorSensor();
      String response = getResponse();
      server.sendResponse(client, response.c_str());
    }
  } else {
    modoManual = true;
  }
  String response = getResponse();
  server.sendResponse(client, response.c_str());
}

// ============================================================================

void setup() {
  pinMode(L293_IN1, OUTPUT);
  pinMode(L293_IN2, OUTPUT);
  Serial.begin(9600);
  delay(1000);
  while (!Serial) {
    ;  // Espera a que el puerto serial conecte.
  }

  Serial.println("/--- Arduino UNO R4 WiFi Web Server + Sensor Ultrasónico ---/");

  // Revisa el módulo de WiFi
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("La communicación con el módulo WiFi falló.");
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Por favor actualice el firmware.");
  }

  // Intenta conectar a la red WiFi
  Serial.print("Intentando conectar a la red ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Conexión exitosa
  Serial.println(" Conectado :D");

  // Endpoints
  server.addRoute("/", handleIndex);
  server.addRoute("/motor/izq", handleMotorIzquierda);
  server.addRoute("/motor/der", handleMotorDerecha);
  server.addRoute("/motor/stop", handleMotorParar);
  server.addRoute("/modo", handleModo);
  server.begin();

  Serial.println("<=== Servidor web listo ===>");
  Serial.print("Visita: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
}