/*
  PROYECTO TUXTTER - NODO TRANSMISOR (DIFFUSION)
  Envía ráfagas de datos de forma abierta sin requerir direccionamiento MAC.
  Diseñado para pruebas de rendimiento de antenas en campo.
*/
#include <WiFi.h>
#include <esp_now.h>

// Estructura de transmisión de datos protegida
typedef struct __attribute__((packed)) {
  uint32_t ID_secuencia;
} BloqueDatos;

const uint8_t DIRECCION_DIFUSION[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint32_t contador_envios = 0;

void setup() {
  Serial.begin(115200);
  
  // Inicialización del entorno de radiofrecuencia
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("[PROYECTO TUXTTER] Error crítico: No se pudo iniciar el protocolo inalámbrico.");
    while (true) delay(1000);
  }
  
  // Registro y configuración del enlace abierto (Peer)
  esp_now_peer_info_t configuracion_enlace = {};
  memcpy(configuracion_enlace.peer_addr, DIRECCION_DIFUSION, 6);
  configuracion_enlace.channel = 1; // Canal fijo de transmisión
  configuracion_enlace.ifidx = WIFI_IF_STA;
  configuracion_enlace.encrypt = false;
  
  esp_now_add_peer(&configuracion_enlace);
  Serial.println("=== PROYECTO TUXTTER: Transmisor Inicializado Core OK ===");
}

void loop() {
  BloqueDatos paquete_actual = { contador_envios++ };
  
  // Despacho del paquete de datos al espacio de radio
  esp_err_t estado_transmision = esp_now_send(DIRECCION_DIFUSION, (uint8_t *)&paquete_actual, sizeof(paquete_actual));
  
  // Reporte periódico en consola cada 25 envíos para no saturar el buffer
  if (contador_envios % 25 == 0) {
    Serial.print("[PROYECTO TUXTTER] Muestras enviadas al aire: ");
    Serial.print(contador_envios);
    Serial.print(" | Estado del bus: ");
    Serial.println(estado_transmision == ESP_OK ? "ACTIVO" : "FALLO");
  }
  
  delay(200); // Mantiene la tasa fija de 5 paquetes por segundo
}
