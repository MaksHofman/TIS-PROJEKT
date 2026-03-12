/*
  Projekt TIS E1
  Czujnik koloru TCS3200
  Platforma: Arduino MKR WAN 1310
*/

#define S0 2
#define S1 3
#define S2 4
#define S3 5
#define OUT_PIN 6

float red = 0;
float green = 0;
float blue = 0;
float clear = 0;
float max1;

float readRed()
{
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);  
    int frequency = pulseIn(OUT_PIN, LOW);
    return 1.0f/frequency;
}

float readGreen()
{
    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);

    int frequency = pulseIn(OUT_PIN, LOW);
    return 1.0f/frequency;
}

float readBlue()
{
    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH); 
    int frequency = pulseIn(OUT_PIN, LOW);
    return 1.0f/frequency;
}

float readClear()
{
    digitalWrite(S2, HIGH);
    digitalWrite(S3, LOW);  
    int frequency = pulseIn(OUT_PIN, LOW);
    return 1.0f/frequency;
}

// calibration values
float maxR;
float maxG;
float maxB;

float readCalRed() {
    return readRed() / maxR;
}

float readCalGreen() {
    return readGreen() / maxG;
}

float readCalBlue() {
    return readBlue() / maxB;
}

void setup()
{
    Serial.begin(9600); delay(2000);
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(OUT_PIN, INPUT);    

    // ustawienie skalowania częstotliwości: L L - Power down, L H - 2%, H L - 20%, H H - 100%
    digitalWrite(S0, HIGH);
    digitalWrite(S1, LOW);

    calibrate();
}

void loop() {
    red = readCalRed();
    green = readCalGreen();
    blue = readCalBlue();
    clear = readClear();
    max1 = max(max(max(red, green), blue), clear);

    Serial.print("DBG: (");
    // Serial.print("RED: ");
    Serial.print(red/max1);

    Serial.print(", ");
    Serial.print(green/max1);

    Serial.print(", ");
    Serial.print(blue/max1);

    Serial.print(", ");
    Serial.print(clear/max1);
    Serial.println(")");

    getColor();
    delay(1000);
}

void calibrate() {
    int del = 5000;
    Serial.println("cal red: ");
    delay(del);
    maxR = readRed();

    Serial.println("cal green: ");
    delay(del);
    maxG = readGreen();

    Serial.println("cal blue: ");
    delay(del);
    maxB = readBlue();
}

void getColor() {
    if (red == max1)
        Serial.println("RED");
    else if (green == max1)
        Serial.println("GREEN");
    else if (blue == max1)
        Serial.println("BLUE");
    else
        Serial.println("WHITE");
}
