#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
int ldrValue;
String pesan="";

const int interval = 10000;
unsigned long previousMillis = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define pinLED 5
#define pinLDR 36

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class RecieveCallbacks: public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string rxValue = pCharacteristic->getValue();

    Serial.println("masuk pesan");
    
    if(rxValue.length()>0){
      for (int i = 0; i < rxValue.length(); i++){
        char pesanMasuk = rxValue[i];
        pesan += String(pesanMasuk);
      }   
    }
    Serial.println(pesan);
    if(pesan == "nyala"){
      digitalWrite(pinLED,HIGH);
    }
    else if(pesan == "mati"){
      digitalWrite(pinLED,LOW);
    }
    pesan = "";
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(pinLED,OUTPUT);
  pinMode(pinLDR,INPUT);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // 1. Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 2. Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 3. Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                    );
                    
  // 4. Create Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new RecieveCallbacks());
  
  // 5. Start the service
  pService->start();

  // 6. Start advertising
  pServer->getAdvertising()->start();

  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    unsigned long currentMillis = millis();
    
    // notify changed value
    if (deviceConnected) {
        if (currentMillis - previousMillis >= interval){
          previousMillis = currentMillis;
          ldrValue = analogRead(pinLDR);

          //konversi ldrValue to string
          char ldrString[8];
          dtostrf(ldrValue,1,2,ldrString);
          
          pCharacteristic->setValue(ldrString);
          pCharacteristic->notify();
          delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
