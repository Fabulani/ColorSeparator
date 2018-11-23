/* TESTAR SENSOR DE PRESENÇA IR */

/* Conecte o OUT do sensor IR ao pino pin_IR (MEGA).
 * Execute o código e abra o monitor serial.
 * Caso o sensor envie LOW ao detectar algo, coloque
 * um '!' em frente ao "digitalRead(pin_IR)" no void loop.
 */

const byte pin_IR = 19;
bool detected;

void setup(){
    pinMode(pin_IR, INPUT);
    Serial.begin(9600);
}

void loop(){
    detected = !digitalRead(pin_IR);
    if (detected){
        Serial.println("Detectou algo.");
    }
    else{
        Serial.println("(...)");
    }
    delay(500);
}
