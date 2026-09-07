#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // Ancho de la pantalla en pixeles
#define SCREEN_HEIGHT 64 // Alto de la pantalla en pixeles
#define OLED_RESET -1    // Pin de Reset (usar -1 si se comparte con el microcontrolador)
#define SCREEN_ADDRESS 0x3C // Dirección I2C (0x3C para 128x64, a veces es 0x3D)

#define FRAME_WIDTH (32)
#define FRAME_HEIGHT (32)
#define FRAME_COUNT 4

// Control de selección (cambio cada 2 segundos)
unsigned long lastSelectionMilli = 0;
bool selectTemp = true; // true = TEMP (To), false = SPEED (Vo)

// Estado del botón de Temperatura
bool btnTempLastState = HIGH;
unsigned long btnTempPressTime = 0;

// Estado del botón de Velocidad
bool btnSpeedLastState = HIGH;
unsigned long btnSpeedPressTime = 0;

const unsigned long LONG_PRESS_TIME = 500; // ms para considerar pulsación larga

unsigned long lastUpdate = 0;
const unsigned long UpdateTimeout = 1000;
int frame = 0;
const byte PROGMEM frames_speed[][128] = {
  {0,0,0,0,0,2,0,0,0,31,0,0,0,17,7,0,0,17,141,128,0,16,216,192,4,16,112,128,15,32,0,128,24,192,1,128,16,0,1,128,8,0,0,128,4,3,192,252,6,4,96,4,2,8,16,4,6,24,8,6,12,16,8,28,56,16,8,48,96,24,24,96,32,8,16,64,32,6,32,64,63,3,192,48,1,0,0,24,0,128,0,8,1,128,3,24,1,0,4,240,1,14,8,32,3,27,8,0,1,177,136,0,0,96,136,0,0,0,248,0,0,0,64,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,24,28,0,0,108,54,0,0,68,34,0,0,67,194,0,0,64,2,0,0,64,2,0,0,192,3,0,31,128,1,248,48,0,0,12,32,3,192,4,48,4,32,12,24,8,16,24,4,16,24,32,4,16,8,32,4,16,8,32,4,24,8,32,24,8,16,24,48,4,32,12,32,3,192,4,48,0,0,12,31,128,1,248,0,64,3,0,0,64,2,0,0,64,2,0,0,67,226,0,0,68,34,0,0,108,54,0,0,56,24,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,96,0,0,0,248,0,0,224,136,0,1,177,136,0,3,27,8,0,1,14,8,32,1,0,4,240,0,128,3,24,1,128,0,8,1,0,0,24,63,3,192,48,32,4,48,96,32,8,16,64,96,24,24,96,56,16,8,48,12,16,8,28,6,24,24,6,2,8,16,4,6,6,48,4,12,3,192,252,24,0,0,128,16,0,1,0,24,192,1,128,15,32,0,128,4,16,48,128,0,16,200,192,0,17,133,128,0,17,3,0,0,31,0,0,0,2,0,0,0,0,0,0},
  {0,0,0,0,0,3,192,0,0,2,64,0,0,6,96,0,1,4,32,128,3,132,33,192,4,124,62,32,12,32,12,48,4,0,0,32,2,0,0,64,3,0,0,192,3,3,192,192,2,12,96,64,30,8,16,124,96,24,8,14,64,16,8,6,64,16,8,6,112,16,24,14,30,8,16,120,2,6,48,64,3,3,192,192,3,0,0,192,2,0,0,64,4,0,0,32,12,48,4,48,4,124,62,32,3,132,33,192,1,4,32,128,0,6,96,0,0,2,64,0,0,3,192,0,0,0,0,0},  
};

bool displayInitialized = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initDisplay() {
    if (displayInitialized) {
        return;
    }

    Wire.begin(PIN_SDA, PIN_SCL);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        displayInitialized = false;
        while (true);
    }
    displayInitialized = true;
    display.clearDisplay();
    display.display();
    
}

String toHHMMSS(unsigned long seconds) {
  int hours = seconds / 3600;
  int min = (seconds % 3600) / 60;
  int sec = seconds % 60;
  
  String result = "";
  
  // Agregar horas con formato
  result += String(hours) + "h";
  result += String(min) + "m";
  result += String(sec) + "s";
  
  return result;
}

void drawUI() {
    if (OTA_update) return; 

    static bool lastState = HIGH;
    unsigned long currentMillis = millis();

    // ==========================================
    // 1. LÓGICA DE SELECCIÓN AUTOMÁTICA
    // ==========================================
    /*if (currentMillis - lastSelectionMilli >= 1000) {
        selectTemp = !selectTemp; // Alterna entre TEMP y SPEED
        lastSelectionMilli = currentMillis; // Reinicia el temporizador
    }*/

    // ==========================================
    // 3. RENDERIZADO DE LA PANTALLA OLED/LCD
    // ==========================================
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // --- SECCIÓN TEMPERATURA ---
    display.setTextSize(1);
    display.setCursor(0, 0); 
    display.print("TEMP");
    display.print(" (");
    display.print(To, 0);
    display.print(")");

    display.setTextSize(2);
    display.setCursor(0, 13);
    display.print(T, 0);
    display.setTextSize(1);
    display.print("C");

    // --- SECCIÓN VELOCIDAD ---
    display.setCursor(0, 36);
    display.print("SPEED");

    display.setTextSize(2);
    display.setCursor(0, 50);
    display.print(Vo);
    
    // Posición de la unidad "cm/min" dinámica según los dígitos de Vo
    int xWidth = String((int)Vo).length() * 12; 
    display.setCursor(xWidth, 56);
    display.setTextSize(1);
    display.print("cm/min");

    // --- INDICADOR VISUAL DE SELECCIÓN ---
    /*if (selectTemp) {
        // Flecha apuntando a la sección de Temperatura
        display.fillTriangle(0, 0, 0, 6, 6, 3, SSD1306_WHITE);
    } else {
        // Flecha apuntando a la sección de Velocidad
        display.fillTriangle(0, 36, 0, 42, 6, 39, SSD1306_WHITE);
    }*/

    if (status == "working") {
        display.drawBitmap(94, 34, frames_speed[frame], FRAME_WIDTH, FRAME_HEIGHT, 1);
        frame = (frame + 1) % FRAME_COUNT;
        if (F || !Fenable) {
            int xWidth = toHHMMSS(Ts).length() * 6; 
            display.setCursor(128-xWidth, 0);
            display.print(toHHMMSS(Ts));
        }
    }

    lastState = !lastState;
    display.display();
}

#if VERSION > 1501
void checkButtons() {
    unsigned long currentMillis = millis();

    // ==========================================
    // 1. CONTROL DEL BOTÓN DE TEMPERATURA
    // ==========================================
    bool btnTempState = digitalRead(PIN_BTN1);

    // Flanco de bajada: cuando presionas el botón de temperatura
    if (btnTempState == LOW && btnTempLastState == HIGH) {
        btnTempPressTime = currentMillis;
    }
    // Flanco de subida: cuando sueltas el botón
    else if (btnTempState == HIGH && btnTempLastState == LOW) {
        unsigned long pressDuration = currentMillis - btnTempPressTime;

        if (pressDuration > 50) { // Debounce de 50ms
            if (pressDuration < LONG_PRESS_TIME) {
                // --- PULSACIÓN CORTA: SUBIR TEMPERATURA (+5) ---
                To += 5;
                if (To > maxT) To = maxT; // Límite máximo Temp
            } else {
                // --- PULSACIÓN LARGA: BAJAR TEMPERATURA (-5) ---
                To -= 5;
                if (To < minT) To = minT; // Límite mínimo Temp
            }
        }
    }
    btnTempLastState = btnTempState;

    // ==========================================
    // 2. CONTROL DEL BOTÓN DE VELOCIDAD
    // ==========================================
    bool btnSpeedState = digitalRead(PIN_BTN2);

    // Flanco de bajada: cuando presionas el botón de velocidad
    if (btnSpeedState == LOW && btnSpeedLastState == HIGH) {
        btnSpeedPressTime = currentMillis;
    }
    // Flanco de subida: cuando sueltas el botón
    else if (btnSpeedState == HIGH && btnSpeedLastState == LOW) {
        unsigned long pressDuration = currentMillis - btnSpeedPressTime;

        if (pressDuration > 50) { // Debounce de 50ms
            if (pressDuration < LONG_PRESS_TIME) {
                // --- PULSACIÓN CORTA: SUBIR VELOCIDAD (+5) ---
                Vo += 5;
                if (Vo > maxV) Vo = maxV;   // Límite máximo Speed
            } else {
                // --- PULSACIÓN LARGA: BAJAR VELOCIDAD (-5) ---
                Vo -= 5;
                if (Vo < minV) Vo = minV;     // Límite mínimo Speed
            }
        }
        drawUI();
    }
    btnSpeedLastState = btnSpeedState;
}
#endif


void displayTask() {
    #if VERSION > 1501
        checkButtons();
    #endif
    if (millis() - lastUpdate >= UpdateTimeout) {
        lastUpdate = millis();
        initDisplay();
        drawUI();
    }
}