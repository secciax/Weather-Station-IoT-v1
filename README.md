# 🌤️ IoT Weather Station (BLE & MQTT) — Version 1.0

Prototipo di **Stazione Meteo IoT** a due nodi basato su microcontrollori Arduino, architettura di comunicazione a due livelli (**BLE** per la local area network sensoristica ed **MQTT** per l'offloading cloud) e sistema di alimentazione solare auto-sostenibile per il nodo esterno.

Questo repository contiene la **Fase 1** di un progetto di tesi in Ingegneria Informatica incentrato sul monitoraggio microclimatico remoto, sull'ottimizzazione dei consumi energetici e sulla futura integrazione di Edge AI e comunicazione a lungo raggio (LoRaWAN).

## 📸 Galleria del Progetto

| Hardware & Moduli | Case Esterno Solare (Vista 1) | Case Esterno Solare (Vista 2) | 
| ----- | ----- | ----- | 
|  |  |  | 
| *Arduino Nano 33 BLE Sense Rev2 & Nano RP2040 Connect* | *Custodia provvisoria per esterni con pannello solare 5V integrated* | *Dettaglio del passaggio cavi e alloggiamento sensori* | 

| Dashboard Grafici ThingSpeak | Monitoraggio LCD locale | 
| ----- | ----- | 
|  |  | 
| *Serie temporali (Temp, Umidità, Pressione) su ThingSpeak* | *Display LCD 16x2 I2C con aggiornamento live* | 

## 🏗️ Architettura di Sistema

Il sistema si suddivide in due entità hardware complementari che separano la fase di **sensing a basso consumo** dalla fase di **processing e connettività di rete (Gateway)**.

```
flowchart TD
    subgraph OUTDOOR[" 🏕️ Modulo Esterno (Sensor Node / BLE Peripheral) "]
        SOLAR["☀️ Pannello Solare 5V"] -->|Ricarica| CN["🔋 Caricabatterie CN3065"]
        BAT["⚡ Batt. Li-Po 3.7V 1S"] <--> CN
        CN -->|VIN / GND| BLE_SENSE["📟 Arduino Nano 33 BLE Sense Rev2"]
        SENS1["🌡️ HS300x (Temp & Umid)"] -->|I2C On-board| BLE_SENSE
        SENS2["⏲️ LPS22HB (Pressione)"] -->|I2C On-board| BLE_SENSE
    end

    subgraph PAN[" 📡 PAN (Bluetooth Low Energy GATT) "]
        BLE_SENSE -.->|"Advertising & NOTIFY<br/>(10-15 min interval)"| RP2040
    end

    subgraph INDOOR[" 🏠 Modulo Interno (Gateway / BLE Central) "]
        RP2040["📟 Arduino Nano RP2040 Connect"]
        LCD["📺 Display LCD 16x2 (I2C 0x27)"] <-->|SDA (A4) / SCL (A5)| RP2040
    end

    subgraph CLOUD[" ☁️ Cloud & Analytics (ThingSpeak) "]
        RP2040 ==>|"Wi-Fi (WPA2) + MQTT (Port 1883)<br/>Payload: field1, field2, field3"| TS["📊 Broker MQTT ThingSpeak"]
    end


```

### 1. Modulo Esterno (BLE Peripheral / Sensor Node)

* **Hardware**: Arduino Nano 33 BLE Sense Rev2 (nRF52840 @ 64 MHz, ARM Cortex-M4F).

* **Sensori attivi**: Sensore di Temperatura e Umidità integrato `HS300x`, Barometro integrato `LPS22HB`.

* **Alimentazione**: Cella solare monocristallina 5V + Gestore di ricarica Li-Po `CN3065` + Batteria Li-Po 1S 3.7V.

* **Comportamento**:

  1. Esegue il boot e la configurazione dei servizi BLE.

  2. Acquisisce la tripletta di dati fisici dai sensori I2C di bordo.

  3. Aggiorna i valori delle tre caratteristiche GATT e invia una notifica `NOTIFY` ai client iscritti.

  4. Entra in stato di risparmio energetico in attesa dell'intervallo successivo. (Nota: Il modulo BLE rimane però attivo, così come la connessione, consumando energia)

### 2. Modulo Interno (BLE Central / Gateway)

* **Hardware**: Arduino Nano RP2040 Connect (Raspberry Pi RP2040 dual-core @ 133 MHz, modulo Wi-Fi/BT u-blox NINA-W102).

* **Display**: Schermo LCD 16x2 con interfaccia I2C (chip PCF8574, indirizzo `0x27`).

* **Comportamento**:

  1. Inizializza il Wi-Fi locale e si connette al broker MQTT di ThingSpeak.

  2. Esegue uno scan BLE alla ricerca della periferca identificata come `"StazioneMeteo"`.

  3. Si sottoscrive alle notifiche delle caratteristiche di temperatura, umidità e pressione.

  4. Alla ricezione dei pacchetti, aggiorna il display LCD locale e genera la stringa di pubblicazione MQTT (`field1=temp&field2=umid&field3=pres`).

## ⚡ Schemi Elettrici e Cablaggi

### 1. Sotto-sistema di Alimentazione Solare (Modulo Esterno)

Il modulo esterno è reso autonomo tramite un circuito di energy harvesting basato sul controller **CN3065**.

```
[Pannello Solare 5V] ──(+)──> [CN3065 SOLAR+]
                     ──(-)──> [CN3065 SOLAR-]

[Batteria Li-Po 3.7V] ──(+)──> [CN3065 BAT+]
                      ──(-)──> [CN3065 BAT-]

[CN3065 SYS+] ──[ Switch ON/OFF ]──> [Arduino Nano 33 BLE - VIN]
[CN3065 SYS-] ────────────────────> [Arduino Nano 33 BLE - GND]


```

> **Nota di Sicurezza Elettrica:** La batteria Li-Po deve essere provvista di scheda di protezione integrata (BMS PCM) per evitare sovra-scarica sotto i 3.0V. Il pin `VIN` dell'Arduino accetta tensioni comprese tra 4.5V e 21V per regolazione interna; l'alimentazione diretta a 3.7V-4.2V sfrutta la soglia di dropout permissiva del regolatore onboard.

### 2. Collegamento Display LCD I2C (Modulo Interno)

| Pin Modulo I2C (LCD) | Pin Arduino Nano RP2040 Connect | Tensione / Segnale | 
| ----- | ----- | ----- | 
| **GND / VSS** | **GND** | Massa comune | 
| **VCC / VDD** | **3V3** *(oppure VUSB 5V previo ponticello)* | Alimentazione 3.3V | 
| **SDA** | **A4** | Linea Dati I2C | 
| **SCL** | **A5** | Linea Clock I2C | 

## 💻 Modello Dati e Comunicazione

### BLE GATT Service & Characteristics UUID

* **Service UUID**: `19B10000-E8F2-537E-4F6C-D104768A1214`

  * **Temperature Characteristic**: `19B10001-E8F2-537E-4F6C-D104768A1214` (BLERead | BLENotify)

  * **Humidity Characteristic**: `19B10002-E8F2-537E-4F6C-D104768A1214` (BLERead | BLENotify)

  * **Pressure Characteristic**: `19B10003-E8F2-537E-4F6C-D104768A1214` (BLERead | BLENotify)

### MQTT Topic & Payload (ThingSpeak)

* **Topic**: `channels/<SECRET_ID_CHANNEL>/publish`

* **Payload Format**: `field1=<TEMP>&field2=<HUMID>&field3=<PRESS>`

* **QoS Level**: `0` (At most once)

## 🛠️ Guida all'Installazione e Deploy

### 1. Clonazione del Repository

```
git clone https://github.com/secciax/Weather-Station-IoT-v1.git
cd Weather-Station-IoT-v1


```

### 2. Configurazione Credenziali di Rete

Per mantenere sicure le proprie chiavi privando il codice di valori hardcoded:

1. Naviga nella cartella `Modulo_Interno/`.

2. Troverai il file template `secrets_template.h`.

3. Rinominalo in `secrets.h`.

4. Compila le costanti con i tuoi parametri reali:

```
#define SECRET_SSID "NOME_DELLA_TUA_RETE_WIFI"
#define SECRET_PASS "PASSWORD_WIFI"

#define SECRET_MQTT_CLIENT_ID "CLIENT_ID_THINGSPEAK"
#define SECRET_MQTT_USER      "USERNAME_MQTT_THINGSPEAK"
#define SECRET_PASS_MQTT      "PASSWORD_MQTT_THINGSPEAK"

#define SECRET_ID_CHANNEL     3410719UL  // ID Canale ThingSpeak (Suffisso UL)


```

### 3. Dipendenze Librerie Arduino IDE

Assicurati di installare tramite l'Arduino Library Manager:

* `ArduinoBLE`

* `WiFiNINA` / `WiFi` (per RP2040 Connect)

* `PubSubClient` (di Nick O'Leary)

* `LiquidCrystal_I2C`

* `Arduino_HS300x` e `Arduino_LPS22HB` (per Nano 33 BLE Sense Rev2)

## 🔍 Problemi Risolti e Lesson Learned

Durante la progettazione e l'integrazione del prototipo v1 sono state affrontate e risolte diverse criticità hardware e software:

### 1. Contrasto e Alimentazione Display LCD a 3.3V

* **Problema**: Alimentando lo schermo LCD a 3.3V dal pin `3V3` dell'RP2040, il controller rispondeva all'indirizzo I2C `0x27`, ma il testo risultava del tutto sbiadito o visibile solo con angoli di visione estremi.

* **Causa**: I cristalli liquidi dei classici moduli 16x2 richiedono una tensione VDD vicina ai 5V per garantire il contrasto ottimale. A 3.3V la caduta di tensione sulla breadboard e sui jumper attenuava il segnale visibile.

* **Soluzione**: Pulizia dei contatti, riduzione della lunghezza dei cavi I2C e stabilizzazione del bus tramite inserimento di ritardi di avvio (`delay(250)`) prima della `lcd.init()` per permettere la carica dei condensatori del driver I2C.

### 2. Duplicazione dei Pacchetti MQTT su Notifica BLE

* **Problema**: Nel dataset esportato da ThingSpeak figuravano coppie di invii MQTT distanti pochissimi secondi l'uno dall'altro invece dell'intervallo fisso di 10 minuti.

* **Causa**: La funzione `gestisciStazioneEsterna()` valutava la condizione in OR logico (`caratTemp.valueUpdated() || caratUmid.valueUpdated() || ...`). Poiché le notifiche BLE delle 3 caratteristiche venivano ricevute in due cicli separati di `loop()`, la variabile `nuoviDatiPronti` veniva attivata due volte consecutive.

* **Soluzione**: Implementazione di un flag di sincronizzazione che attende l'aggiornamento di tutte e tre le caratteristiche prima di sollevare l'evento di pubblicazione MQTT.

### 3. Corretta Definizioni di Tipi Numetici (`UL`)

* **Problema**: L'ID del canale ThingSpeak superava il limite massimo rappresentabile da un intero a 16 bit (`int` di default in C++ su alcune architetture).

* **Soluzione**: Adozione del suffisso `UL` (`#define SECRET_ID_CHANNEL 3410719UL`) nel file `secrets.h` per forzare l'assegnazione esplicita del tipo `unsigned long`.

## 🚀 Limiti della Versione 1.0 e Roadmap Futura (Perché LoRaWAN?)

Sebbene la combinazione BLE + Wi-Fi/MQTT abbia dimostrato un'eccellente affidabilità in ambiente domestico, il sistema presenta dei limiti strutturali in ottica di monitoraggio meteo professionale:

### Limiti Riscontrati (BLE & Wi-Fi)

1. **Portata del BLE (Range limitato)**: Il segnale BLE a 2.4 GHz soffre di una forte attenuazione in presenza di muri perimetrali in cemento armato o ostacoli naturali, limitando la distanza tra modulo esterno e modulo interno a 10-15 metri.

2. **Consumo energetico del Gateway Wi-Fi**: Il modulo interno necessita di alimentazione di rete continua a causa dell'assorbimento elevato dello stack Wi-Fi/MQTT durante la riconnessione.

3. **Dipendenza da Infrastruttura Wi-Fi**: In scenari di monitoraggio rurale o agricolo (es. vigneti, campi) la copertura Wi-Fi domestica è assente.

### 🔮 Roadmap Futura (Fase 2 - Tesi Specialistica / Evoluzione)

* \[ \] **Migrazione del Modulo Esterno a LoRa / LoRaWAN (Heltec V3 / ESP32 + SX1262)**: Sostituzione del protocollo BLE con **LoRaWAN** operante a 868 MHz per coprire distanze fino a 5-10 km in campo aperto con elevata penetrazione degli ostacoli e consumo energetico nell'ordine dei microampere in Deep Sleep.

* \[ \] **Infrastruttura Fog & Server Locale**: Realizzazione di un Gateway privato basato su **Raspberry Pi 4**, broker **Mosquitto MQTT**, database per serie temporali **InfluxDB** e dashboard **Grafana** locale.

* \[ \] **TinyML & Edge AI**: Implementazione di modelli di apprendimento automatico direttamente sul microcontrollore esterno (es. modelli *Time Series Forecasting* per la previsione microclimatica locale a corto raggio).

* \[ \] **Integrazione Pluviometro e Anemometro**: Aggiunta di sensori meccanici ad interrupt (interruzioni hardware su pin di wake-up) per la misura della piovosità e della velocità del vento.

## 📜 Licenza

Questo progetto è rilasciato sotto Licenza MIT. Consulta il file [LICENSE](LICENSE) per ulteriori dettagli.

**Autore:** Federico Secciani

**Contatto:** [GitHub Profile](https://github.com/secciax)