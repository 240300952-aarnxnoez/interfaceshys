/*
  PROYECTO TUXTTER - NODO RECEPTOR (ESCUCHA)
  Captura las ráfagas de datos en el aire y evalúa la potencia de enlace (RSSI).
*/
#include <WiFi.h>
#include <esp_now.h>

// La estructura de datos interna debe acoplarse con el emisor
typedef struct __attribute__((packed)) {
  uint32_t ID_secuencia;
} BloqueDatos;

#define PIN_TEST_LED 2 // Canal físico para control visual por LED

// Callback de hardware ejecutado al recibir tramas de radio válidas
void interceptarFlujoDatos(const esp_now_recv_info_t *metadatos_enlace, const uint8_t *datos_crudos, int dimension) {
  BloqueDatos datos_procesados;
  
  // Reconstrucción del paquete binario
  memcpy(&datos_procesados, datos_crudos, sizeof(datos_procesados));
  
  // Extracción del valor de decibelios recibidos (RSSI)
  int16_t ganancia_rssi = metadatos_enlace->rx_ctrl.rssi;

  // Renderizado de lecturas en el Monitor Serie
  Serial.printf("[TUXTTER - DATOS DETECTADOS] -> Registro: #%lu | Nivel de Red: %d dBm\n", (unsigned long)datos_procesados.ID_secuencia, ganancia_rssi);

  // Pulso inverso del indicador lumínico para registrar el evento de llegada
  digitalWrite(PIN_TEST_LED, LOW);
  delay(20);
  digitalWrite(PIN_TEST_LED, HIGH);
}

void setup() {
  Serial.begin(115200);
  
  // Seteo del LED indicador en modo escucha permanente
  pinMode(PIN_TEST_LED, OUTPUT);
  digitalWrite(PIN_TEST_LED, HIGH);
  
  // Activación del receptor Wi-Fi en modo limpio
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("[PROYECTO TUXTTER] Falla fatal: Capa de red inalámbrica deshabilitada.");
    while (true) {
      digitalWrite(PIN_TEST_LED, !digitalRead(PIN_TEST_LED));
      delay(150); // Alerta visual de error en hardware
    }
  }

  // Enlace de la función destructora de paquetes al bus de recepción
  esp_now_register_recv_cb(esp_now_recv_cb_t(interceptarFlujoDatos));
  
  Serial.println("=== PROYECTO TUXTTER: Nodo Receptor Escuchando Frecuencias Abiertas ===");
}

void loop() {
  // El bucle cíclico se mantiene en espera pasiva por interrupciones
}
