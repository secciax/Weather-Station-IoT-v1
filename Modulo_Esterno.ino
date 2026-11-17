/*
Progetto di IoT: Stazione Meteo
Modulo Esterno: Arduino Nano 33 BLE Sense Rev2
Ruolo BLE: peripheral

Il modulo esterno si occupa della cattura di temperatura, umidità e pressione grazie ai sensori integrati nel chip.
Con il BLE si offre il servizio "servizioMeteo" che comprende le rispettive 3 caratteristiche, tutte e tre con proprietà NOTIFY.
Una volta fatte le letture e inviati i dati con BLE notify il dispositivo entra in uno stato di sleep in cui la CPU si spegne,
consumando il minimo dell'energia, mentre il chip radio continua a funzionare mantenendo la connessione attiva.

Federico Secciani
*/

#include <Arduino_HS300x.h>
#include <Arduino_LPS22HB.h>
#include <ArduinoBLE.h>
#include <chrono>

float temperatura;
float umidita;
float pressione;

unsigned long inizioFinestra;
const unsigned long durataFinestra = 5000;
bool statoADV = true;

//Dichiarazione del servizio con le rispettive caratteristiche. Gli vengono assegnati UUID a 128-bit essendo un servizio Custom, e la modalità Notify
BLEService servizioMeteo("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEFloatCharacteristic caratTemperatura("19B10001-E8F2-537E-4F6C-D104768A1214", BLENotify);
BLEFloatCharacteristic caratUmidita("19B10002-E8F2-537E-4F6C-D104768A1214", BLENotify);
BLEFloatCharacteristic caratPressione("19B10003-E8F2-537E-4F6C-D104768A1214", BLENotify);

void setup() {

  /*Inizializzazione della seriale, del BLE e dei sensori:
  * HS300x (Temperatura e Umidità)
  * LPS22HB (Pressione)*/


  Serial.begin(9600);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1)
      ;
  }

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1)
      ;
  }

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  //Assegna nome e UUID per l'advertising del servizio
  BLE.setLocalName("StazioneMeteo");
  BLE.setAdvertisedService(servizioMeteo);

  //Aggiunta delle caratteristiche al servizio
  servizioMeteo.addCharacteristic(caratTemperatura);
  servizioMeteo.addCharacteristic(caratUmidita);
  servizioMeteo.addCharacteristic(caratPressione);
  //Aggiunta del servizio
  BLE.addService(servizioMeteo);

  //Inizia a inviare i pacchetti di advertising
  BLE.advertise();
  Serial.println("Bluetooth® device active, waiting for connections...");
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, LOW);
}

void loop() {

  /*Nel loop si attende la connessione, che viene effettuata dal central. 
  **Una volta connesso, tramite il while e millis() si crea una attesa attiva (5s) in cui si da il tempo al central di scoprire gli attributi e iscriversi al NOTIFY.
  **Nel mentre il peripheral si mostra disponibile alla comunicazione grazie a BLE.poll().
  **Spengo il led RGB per segnalare che è connesso con il peripheral.*/

  BLEDevice central = BLE.central();

  if (central && central.connected()) {
    statoADV = false;

    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, HIGH);

    Serial.print("Connesso all'Arduino Interno (Central): ");
    Serial.println(central.address());

    inizioFinestra = millis();

    while (millis() - inizioFinestra < durataFinestra) {

      BLE.poll();

      if (!central.connected()) break;
    }

    /*Finchè rimane connesso esegue solamente questo while:
    **1.Fase di lettura dei sensori
    **2.Fase di scrittura(aggiornamento) delle caratteristiche (automaticamente avviene il NOTIFY)
    **3.Fase di sleep, preceduta dallo svuotamento del buffer seriale*/

    while (central.connected()) {

      temperatura = HS300x.readTemperature();
      umidita = HS300x.readHumidity();
      pressione = BARO.readPressure() * 10;

      caratTemperatura.writeValue(temperatura);
      caratUmidita.writeValue(umidita);
      caratPressione.writeValue(pressione);

      // print each of the sensor values
      Serial.print("Temperatura = ");
      Serial.print(temperatura);
      Serial.println(" °C");

      Serial.print("Umidità    = ");
      Serial.print(umidita);
      Serial.println(" %");

      Serial.print("Pressione    = ");
      Serial.print(pressione);
      Serial.println(" hPa");

      Serial.println();

      Serial.println("Invio completato. La CPU entra in modalità risparmio energetico...");
      Serial.flush();  // Svuota il buffer seriale prima di addormentare la CPU
      rtos::ThisThread::sleep_for(std::chrono::minutes(10));

      Serial.println("...la CPU si è svegliata per la lettura.");
    }
  }
  /*Entra nell'if nel momento il cui il central non è connesso e l'advertise è spento (si spegne dopo ogni connessione)
  **Così viene riacceso e settata la variabile statoADV = true per non eseguire nuovamente l'advertise inutilmente.
  **Inoltre attivo il led RGB integrato di colore Blu per segnalare che il dispositivo è in advertising (non avendo una seriale per visualizzare i vari stati)*/
  if (!central.connected() && !statoADV) {
    delay(1000);
    Serial.println("Connessione interrotta! Ritorno in modalità Advertising...");
    BLE.advertise();
    statoADV = true;

    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDG, HIGH);
    digitalWrite(LEDB, LOW);
  }
}
