/* TESTAR SERVO MOTOR */

/* Este teste inclui uma interrupção ativada por botão no pino pin_B.
 * Ao pressionar o botão de interrupção, o teste sendo realizado será
 * alterado. Há dois testes:
 * 1) Girar servo de 0 a 180 e retornar, (in)(de)crementos de 10.
 * 2) Simular coleta e descarte de peça alternando giro entre duas posições pré-definidas: pegar_caneta e descarta_caneta.
 *
 * O OUT do servo deve ser ligado ao pino "pin_S" (MEGA), o qual pode ser alterado. O pino do botão de interrupção, "pin_B", deve ser um dos
 * seguintes (MEGA): 2, 3, 18, 19, 20, 21.
*/

#include <Servo.h>

Servo S;  // Criar objeto para controlar o Servo.
byte pin_S = 20;  // Pino ao qual o OUT do servo está conectado.
byte pin_B = 18;  // Pino do botão interrupção. Um dos seguintes: 2, 3, 18, 19, 20, 21.
int valor_inicial = 90;  // Valor inicial em graus do servo.
int pegar_caneta = 180;  // Graus de ajuste do servo para pegar caneta.
int descartar_caneta = 70;  // Graus de ajuste do servo para descartar caneta.
volatile bool teste_flag = false;  // Troca de teste.
volatile bool stop_test = false;  // Para o teste atual.

void setup() {
    S.attach(pin_S);
    S.write(valor_inicial);
    Serial.begin(9600);

    attachInterrupt(digitalPinToInterrupt(pin_B), ISR_trocarTeste, RISING);
}

void loop() {
    stop_test = false;
    if(teste_flag) {
        teste1();
    }
    else {
        teste2();
    }
    delay(500);

}

// Servo gira de 0 -> 180 -> 0.
void teste1() {
    S.write(0);
    delay(100);
    for(int j = 0; j<=180; j = j+10) {
        S.write(j);
        Serial.println(j);
        delay(400);
        if(stop_test) {
            break;
        }
    }
    for(int j = 180; j>=0; j = j-10) {
        S.write(j);
        Serial.println(j);
        delay(400);
        if(stop_test) {
            break;
        }
    }
}

// Servo alterna entre pegar_caneta e descartar_caneta.
void teste2() {
    while(stop_test != true) {
        S.write(pegar_caneta);
        delay(750);
        S.write(descartar_caneta);
        delay(750);
    }
}

// Interrupção para trocar de teste e parar o que estiver em andamento.
void ISR_trocarTeste() {
    stop_test = true;
    teste_flag = !teste_flag;
    for(int i = 0; i<1000; i++) {
    }  // Delay para tentar evitar debounce do botão.
}
