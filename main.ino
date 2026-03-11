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

int red = 0;
int green = 0;
int blue = 0;
int clearValue = 0;


int readRed()
{
    digitalWrite(S2, LOW);
    digitalWrite(S3, LOW);  
    int frequency = pulseIn(OUT_PIN, LOW);
    return frequency;
}

int readGreen()
{
    digitalWrite(S2, HIGH);
    digitalWrite(S3, HIGH);

    int frequency = pulseIn(OUT_PIN, LOW);
    return frequency;
}

int readBlue()
{
    digitalWrite(S2, LOW);
    digitalWrite(S3, HIGH); 
    int frequency = pulseIn(OUT_PIN, LOW);
    return frequency;
}


int readClear()
{
    digitalWrite(S2, HIGH);
    digitalWrite(S3, LOW);  
    int frequency = pulseIn(OUT_PIN, LOW);
    return frequency;
}

void setup()
{
    Serial.begin(9600); 
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(OUT_PIN, INPUT);    

    // ustawienie skalowania częstotliwości: L L - Power down, L H - 2%, H L - 20%, H H - 100%
    digitalWrite(S0, HIGH);
    digitalWrite(S1, LOW);
}

void loop()
{
    red = readRed();
    green = readGreen();
    blue = readBlue();

    Serial.print("RED: ");
    Serial.print(red);

    Serial.print(" GREEN: ");
    Serial.print(green);

    Serial.print(" BLUE: ");
    Serial.println(blue);

    Serial.print(" CLEAR: ");
    Serial.println(clearValue);
    delay(1000);
}
