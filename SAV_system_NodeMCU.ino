#define BLYNK_TEMPLATE_ID "TMPL4my4AC07g"  // Унікальний ідентифікатор шаблону Blynk (зв'язує пристрій з конкретним шаблоном Blynk Cloud)
#define BLYNK_TEMPLATE_NAME "project1"     // Ім'я шаблону для проекту Blynk (допомагає ідентифікувати проект Blynk)

#include <ESP8266WiFi.h>         // Бібліотека для підключення ESP8266 до Wi-Fi
#include <BlynkSimpleEsp8266.h>  // Бібліотека Blynk для ESP8266

char auth[] = "Ваш токен аутентифікації";  // Токен аутентифікації (можна отримати у додатку Blynk)
char ssid[] = "Ваш SSID";                  // Назва Wi-Fi мережі, до якої підключаємо прилад
char pass[] = "Ваш пароль від Wi-Fi";      // Пароль для Wi-Fi мережі, до якої підключено прилад

#define LM35PIN A0  // Визначаємо аналоговий пін, до якого підключено LM35

#define TRIGGER_PIN1 15  // GPIO15 (D8) - передавач (за потоком)
#define ECHO_PIN1 5      // GPIO5 (D1) - приймач (за потоком)
#define TRIGGER_PIN2 12  // GPIO12 (D6) - передавач (проти потоку)
#define ECHO_PIN2 4      // GPIO4 (D2) - приймач (проти потоку)
#define DIMMER_PIN1 14   // GPIO14 (D5) - пін m1 димера
#define DIMMER_PIN2 13   // GPIO13 (D7) - пін m2 димера
#define LED_RED 0        // GPIO0 (D3) - червоний світлодіод, Q < required_performance
#define LED_GREEN 2      // GPIO2 (D4) - зелений світлодіод, Q >= required_performance

#define SAMPLES 3  // Кількість семплів для фільтра ковзного (рухомого) середнього

float impulseTime1 = 0;  // Час проходження сигналу першої пари датчиків (за потоком)
float impulseTime2 = 0;  // Час проходження сигналу для другої пари датчиків (проти потоку)
int N = 25;              // Кількість вимірів

const float S = 8281e-6;                     // Площа повітроводу в м²
const float D = 91e-3;                       // Діаметр повітроводу в м
const float DEGREES = 30.00;                 // Кут між напрямком руху ультразвуку та повітряним потоком
const float RADIANS = DEGREES * PI / 180;    // Кут у радіанах
const float SIN_2_ANGLE = sin(2 * RADIANS);  // Синус подвійного кута

float vSamples[SAMPLES];  // Масив для зберігання семплів швидкості
int vIndex = 0;           // Індекс поточного семпла швидкості

int V = 60;       // Початкове значення V об'єм приміщення в м³
int K = 5;        // Початкове значення K кількість людей у приміщенні
int AXR = 8;      // Air exchange rate; кратність повітрообміну (8 - ср. значення для офісу)
int Q_norm = 40;  // Норма витрати повітря на одну особу (для офісу 40 м³/год)

// Функція для розрахунку повітрообміну за кратністю
float calculateExchangeByMultiplicity() {
  return AXR * V;
}

// Функція для розрахунку повітрообміну за кількістю людей
float calculateExchangeByPeople() {
  return K * Q_norm;
}

WidgetTerminal terminal(V7);  // Ініціалізація віджету Terminal на віртуальному піні V7

// Обробник для віджету Terminal на V7
BLYNK_WRITE(V7) {
  String command = param.asStr();

  if (command.startsWith("V ")) {
    V = command.substring(2).toInt();
    terminal.print("Об'єм приміщення (V) змінено на: ");
    terminal.println(V);
  } else if (command.startsWith("K ")) {
    K = command.substring(2).toInt();
    terminal.print("Кількость людей (K) змінено на: ");
    terminal.println(K);
  } else if (command.startsWith("AXR ")) {
    AXR = command.substring(4).toInt();
    terminal.print("Кратність повітрообміну (AXR) змінено на: ");
    terminal.println(AXR);
  } else if (command.startsWith("Q_norm ")) {
    Q_norm = command.substring(7).toInt();
    terminal.print("Норму витрати пов. на ос. (Q_norm) змінено на: ");
    terminal.println(Q_norm);
  } else if (command.equals("info")) {
    terminal.println("Поточний об'єм приміщення (V): " + String(V) + " м³");
    terminal.println("Поточна кількість людей (K): " + String(K));
    terminal.println("Поточна кратність повітрообміну (AXR): " + String(AXR));
    terminal.println("Поточна норма витрати пов. на ос. (Q_norm): " + String(Q_norm) + " м³/ч");
  } else if (command.equals("clear")) {
    terminal.clear();  // Очищення терміналу
  }

  terminal.flush();  // Оновлення терміналу
}

void setup() {
  Serial.begin(9600);  // Ініціалізуємо Serial порт зі швидкістю 9600 бод

  pinMode(TRIGGER_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIGGER_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);
  pinMode(DIMMER_PIN1, OUTPUT);
  pinMode(DIMMER_PIN2, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  Blynk.begin(auth, ssid, pass);  // Ініціалізація Blynk

  // Виведення потокових значень V, K, AXR, Q_norm у термінал Blynk при вмиканні приладу
  terminal.print("Поточний об'єм приміщення (V): ");
  terminal.print(V);
  terminal.println(" м³");
  terminal.print("Поточна кількість людей (K): ");
  terminal.println(K);
  terminal.print("Поточна кратність повітрообміну (AXR): ");
  terminal.println(AXR);
  terminal.print("Поточна норма витрати пов. на ос. (Q_norm): ");
  terminal.print(Q_norm);
  terminal.println(" м³/ч");

  terminal.flush();  // Оновлення терміналу

  // Початкове налаштування для димера
  analogWrite(DIMMER_PIN1, 128);  // Встановлюємо середнє значення для m1
  analogWrite(DIMMER_PIN2, 128);  // Встановлюємо середнє значення для m2
}

// Керування димером за допомогою слайдера у додатку на телефоні
BLYNK_WRITE(V5) {
  int dimmerValue = param.asInt();
  // Слайдер ділиться на дві частини: ліву та праву
  if (dimmerValue <= 128) {
    // Ліва частина слайдера (зменшуємо m1, збільшуємо m2)
    analogWrite(DIMMER_PIN1, dimmerValue * 2);        // Максимальне значення при 0
    analogWrite(DIMMER_PIN2, 255 - dimmerValue * 2);  // Мінімальне значення при 0
  } else {
    // Права частина слайдера (збільшуємо m1, зменшуємо m2)
    analogWrite(DIMMER_PIN1, 255 - (dimmerValue - 128) * 2);  // Мінімальне значення при 255
    analogWrite(DIMMER_PIN2, (dimmerValue - 128) * 2);        // Максимальне значення при 255
  }
}

void loop() {
  int analogValue = analogRead(LM35PIN);       // Зчитуємо аналогове значення з LM35
  float voltage = analogValue * (5 / 1023.0);  // Перекладаємо отримане значення в напругу
  float t = voltage * 100.0;                   // Перекладаємо напругу в температуру (°C)

  // Вимірювання для першої пари датчиків (за потоком)
  for (int i = 0; i < N; i++) {
    digitalWrite(TRIGGER_PIN1, HIGH);          // Подаємо високий рівень на пін Trig
    delayMicroseconds(10);                     // Пауза 10 мкс
    digitalWrite(TRIGGER_PIN1, LOW);           // Подаємо низький рівень на пін Trig
    impulseTime1 += pulseIn(ECHO_PIN1, HIGH);  // Зчитуємо час проходу луна-сигналу і додаємо до загального часу
    delay(50);                                 // Пауза 50 мс
  }

  delay(500);  // Час перед наступним розрахунком

  // Вимірювання для другої пари датчиків (проти потоку)
  for (int i = 0; i < N; i++) {
    digitalWrite(TRIGGER_PIN2, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN2, LOW);
    impulseTime2 += pulseIn(ECHO_PIN2, HIGH);
    delay(50);
  }

  impulseTime1 /= N;  // Середній час для першої пари датчиків (за потоком)
  impulseTime2 /= N;  // Середній час для другої пари датчиків (проти потоку)

  float v = abs((D / SIN_2_ANGLE) * ((impulseTime2 - impulseTime1) / (impulseTime2 * impulseTime1) * 1e6));  // Обчислюємо швидкість повітряного потоку (мкс * 1е6 = с => м/с)

  // Фільтрування швидкості за допомогою ковзного (рухомого) середнього
  vSamples[vIndex] = v;                // Додаємо поточний семпл до масиву
  vIndex = (vIndex + 1) % SAMPLES;     // Збільшуємо індекс, обнулюємо, якщо досягли кінця масиву
  float vFiltered = 0;                 // Скидаємо значення фільтрованої швидкості
  for (int i = 0; i < SAMPLES; i++) {  // Перебираємо всі семпли
    vFiltered += vSamples[i];          // Підсумовуємо всі семпли
  }

  vFiltered /= SAMPLES;  // Ділимо на кількість семплів, щоб отримати середнє значення

  float Q = vFiltered * S * 3600 * 0.98;  // Фільтрована продуктивність (м³/год)

  // Розрахунок необхідної продуктивності
  float L_by_multiplicity = calculateExchangeByMultiplicity();
  float L_by_people = calculateExchangeByPeople();

  // Вибираємо більше значення необхідної продуктивності
  float required_performance = max(L_by_multiplicity, L_by_people);

  // Перевірка умов Q < required_performance та Q >= required_performance для керування світлодіодами
  if (Q < required_performance) {
    digitalWrite(LED_RED, HIGH);   // Запалюємо червоний світлодіод
    digitalWrite(LED_GREEN, LOW);  // Гасимо зелений світлодіод
  } else {
    digitalWrite(LED_RED, LOW);     // Гасимо червоний світлодіод
    digitalWrite(LED_GREEN, HIGH);  // Запалюємо зелений світлодіод
  }

  Blynk.virtualWrite(V4, t);                     // Надсилаємо дані температури на віртуальний пін V4
  Blynk.virtualWrite(V0, Q);                     // Надсилаємо дані фільтрованої продуктивності на віртуальний пін V0
  Blynk.virtualWrite(V6, required_performance);  // Надсилаємо дані необхідної продуктивності на віртуальний пін V6

  // Обнулення значень для наступного циклу
  impulseTime1 = 0;
  impulseTime2 = 0;

  Blynk.run();
  delay(1000);
}
