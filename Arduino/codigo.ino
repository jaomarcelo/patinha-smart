int nivelEscolhido = 85; 

void setup() {
  Serial.begin(9600);
}

void loop() {

  Serial.println(nivelEscolhido);

  delay(5000);
}