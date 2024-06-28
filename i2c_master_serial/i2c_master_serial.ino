#include <Wire.h>
#include "ECG_GSR.h"

// For storing control messages from serial
String controlMessage;

// For temporarily storing temperature information retrieved from a slave board
float temperatures[4];

// For storing all connected boards
char boards[128] = {0};

// For storing intensityMultipliers
float temperatureMultiplier = 1;
float vibrationMultiplier = 1;

// Set to true, if GSR sensor is used
bool gsrEnabled = false;

int tempintensity = 200;

void setup() {
  pinMode(7, OUTPUT);
  digitalWrite(7, 1);
  // Initialize serial communication for receiving commands
  Serial.begin(9600);
  // Initialize I2C communication with slave boards
  Wire.begin();
  delay(550);
  // Scan for all devices
  getDevices();

  if (gsrEnabled) {
    initializeGSR();
  }

  //peltierSetIntensity(101, tempintensity, 0, tempintensity, 0, 15000);
  //peltierSetIntensity(104, tempintensity, 0, tempintensity, 0, 15000);
  //peltierSetIntensity(31, tempintensity, 0, tempintensity, 0, 15000);
  //peltierSetIntensity(33, tempintensity, 0, tempintensity, 0, 15000);
  //vibratorSetIntensity(53, 100, 100, 10000);
  peltierSetMinimumMaximum(104, 0, 045, 0, 045);
  //peltierSetIntensity(104, tempintensity, 0, tempintensity, 0, 5000);

}

void loop() {
  // Check for incoming control messages without interrupting control loop
  if (Serial.available()) {
    char inByte = Serial.read();
    // Read entire control message to be handled
    while (inByte != '\n') {
      if (Serial.available()) {
        controlMessage += inByte;
        inByte = Serial.read();
      }
    }

    // Handle control message
    handleMessage();
    controlMessage = "";
  }

  //controlMessage = "p1101";
  //handleMessage();
  //controlMessage = "p1104";
  //handleMessage();
  //delay(100);

  // Any required control functionality

  /*peltierReadSensors(104);
  Serial.print("S1 ");
  Serial.print(temperatures[0]);
  Serial.print(" ");
  Serial.print(temperatures[1]);
  Serial.print(" ");
  Serial.print(temperatures[2]);
  Serial.print(" ");
  Serial.print(temperatures[3]);
  Serial.println("");
  /*peltierReadSensors(33);
  Serial.print("S2 ");
  Serial.print(temperatures[0]);
  Serial.print(" ");
  Serial.print(temperatures[1]);
  Serial.print(" ");
  Serial.print(temperatures[2]);
  Serial.print(" ");
  Serial.print(temperatures[3]);
  Serial.println("");
  peltierReadSensors(104);
  Serial.print("S3 ");
  Serial.print(temperatures[0]);
  Serial.print(" ");
  Serial.print(temperatures[1]);
  Serial.print(" ");
  Serial.print(temperatures[2]);
  Serial.print(" ");
  Serial.println(temperatures[3]);
  Serial.print("");
  peltierReadSensors(31);
  Serial.print("S4 ");
  Serial.print(temperatures[0]);
  Serial.print(" ");
  Serial.print(temperatures[1]);
  Serial.print(" ");
  Serial.print(temperatures[2]);
  Serial.print(" ");
  Serial.print(temperatures[3]);
  Serial.println("");*/
  delay(100);

}

// ############
// #Serial API#
// ############

// Message format:
// First byte - Device type
// Second byte - Command type
// Third/nth byte - Parameters
void handleMessage() {
  // General API
  // Commands:
  // 0 - Get Devices
  // 1 - Soft Shutdown
  // 2 - Hard Shutdown
  // 3 - Set Temperature Multiplier | multiplier
  // 4 - Set Vibration Multiplier | multiplier
  if (controlMessage[0] == 'g') {
    if (controlMessage[1] == '0') {
      bool first = true;
      Serial.print("devices:[");
      for (int i = 1; i < 128; i++) {
        if (boards[i]) {
          if (!first) {
            Serial.print(',');
          }
          Serial.print('(');
          Serial.print(i);
          Serial.print(',');
          Serial.print(boards[i]);
          Serial.print(')');
          first = false;
        }
      }
      Serial.println(']');
    } else if (controlMessage[1] == '1') {
      softShutdown();
      Serial.println("code: 0");
    } else if (controlMessage[1] == '2') {
      hardShutdown();
      Serial.println("code: 0");
    } else if (controlMessage[1] == '3') {
      float multiplier = controlMessage.substring(4).toFloat();
      setTemperatureMultiplier(multiplier);
      Serial.println("code: 0");
    }  else if (controlMessage[1] == '4') {
      float multiplier = controlMessage.substring(4).toFloat();
      setVibrationMultiplier(multiplier);
      Serial.println("code: 0");
    }
    // Peltier Device API
    // Commands:
    // 0 - Set Intensity | address intensity1 direction1 intensity2 direction2 duration
    // 1 - Read Sensors | address
    // 2 - Set Minimum and Maximum temperatures | address minimumTemperatureSide1 maximumTemperatureSide1 minimumTemperatureSide2 maximumTemperatureSide2
    // 3 - Set Target Temperature | address target1 target2 duration
  } else if (controlMessage[0] == 'p') {
    // Convert ASCII to 3 digit number
    byte deviceAddress = controlMessage.substring(2, 5).toInt();
    if (controlMessage[1] == '0') {
      byte intensity1 = controlMessage.substring(5, 8).toInt();
      byte direction1 = controlMessage[8] - '0';
      byte intensity2 = controlMessage.substring(9, 12).toInt();
      byte direction2 = controlMessage[12] - '0';
      long duration = controlMessage.substring(13).toInt();
      peltierSetIntensity(deviceAddress, intensity1, direction1, intensity2, direction2, duration);
      Serial.println("code: 0");
    } else if (controlMessage[1] == '1') {
      // Read sensors
      peltierReadSensors(deviceAddress);
      // Write output to Serial - ts: timestamp, values: [value]
      Serial.print("ts:");
      Serial.print(millis());
      Serial.print(",values:[");
      for (int i = 0; i < 3; i++) {
        Serial.print(temperatures[i]);
        Serial.print(",");
      }
      Serial.print(temperatures[3]);
      Serial.println("]");
    } else if (controlMessage[1] == '2') {
      byte minimumTemperatureSide1 = controlMessage.substring(5, 8).toInt();
      byte maximumTemperatureSide1 = controlMessage.substring(8, 11).toInt();
      byte minimumTemperatureSide2 = controlMessage.substring(11, 14).toInt();
      byte maximumTemperatureSide2 = controlMessage.substring(14, 17).toInt();
      peltierSetMinimumMaximum(deviceAddress, minimumTemperatureSide1, maximumTemperatureSide1, minimumTemperatureSide2, maximumTemperatureSide2);
      Serial.println("code: 0");
    }  else if (controlMessage[1] == '3') {
      byte temperatureTarget1 = controlMessage.substring(5, 8).toInt();
      byte temperatureTarget2 = controlMessage.substring(8, 11).toInt();
      long duration = controlMessage.substring(11).toInt();
      peltierSetTarget(deviceAddress, temperatureTarget1, temperatureTarget2, duration);
      Serial.println("code: 0");
    }
    // Vibrator Device API
    // Commands:
    // 0 - Set Intensity | address intensity1 intensity2 duration
  } else if (controlMessage[0] == 'v') {
    // Convert ASCII to 3 digit number
    byte deviceAddress = controlMessage.substring(2, 5).toInt();
    if (controlMessage[1] == '0') {
      byte intensity1 = controlMessage.substring(5, 8).toInt();
      byte intensity2 = controlMessage.substring(8, 11).toInt();
      long duration = controlMessage.substring(11).toInt();
      vibratorSetIntensity(deviceAddress, intensity1, intensity2, duration);
      Serial.println("code: 0");
    }
    // Temperature Sensor API
    // Commands:
    // 0 - Read Sensor | address
  } else if (controlMessage[0] == 't') {
    // Convert ASCII to 3 digit number
    byte deviceAddress = controlMessage.substring(2, 5).toInt();
    if (controlMessage[1] == '0') {
      thermistorReadSensors(deviceAddress);
      // Write output to Serial - ts: timestamp, values: [value]
      Serial.print("ts:");
      Serial.print(millis());
      Serial.print(",values:[");
      for (int i = 0; i < 2; i++) {
        Serial.print(temperatures[i]);
        Serial.print(",");
      }
      Serial.print(temperatures[2]);
      Serial.println("]");
    }
    // Heater Device API
    // Commands:
    // 0 - Set Intensity | address intensity1 intensity2 duration
    // 1 - Read Sensors | address
    // 2 - Set Maximum temperature | address maximumTemperature
    // 3 - Set Target Temperature | address target1 target2 duration
  } else if (controlMessage[0] == 'h') {
    // Convert ASCII to 3 digit number
    byte deviceAddress = controlMessage.substring(2, 5).toInt();
    if (controlMessage[1] == '0') {
      byte intensity1 = controlMessage.substring(5, 8).toInt();
      byte intensity2 = controlMessage.substring(8, 11).toInt();
      long duration = controlMessage.substring(11).toInt();
      heaterSetIntensity(deviceAddress, intensity1, intensity2, duration);
      Serial.println("code: 0");
    } else if (controlMessage[1] == '1') {
      // Read sensors
      heaterReadSensors(deviceAddress);
      // Write output to Serial - ts: timestamp, values: [value]
      Serial.print("ts:");
      Serial.print(millis());
      Serial.print(",values:[");
      Serial.print(temperatures[0]);
      Serial.print(",");
      Serial.print(temperatures[1]);
      Serial.println("]");
    } else if (controlMessage[1] == '2') {
      byte maximumTemperature = controlMessage.substring(5, 8).toInt();
      heaterSetMaximum(deviceAddress, maximumTemperature);
      Serial.println("code: 0");
    }  else if (controlMessage[1] == '3') {
      byte temperatureTarget1 = controlMessage.substring(5, 8).toInt();
      byte temperatureTarget2 = controlMessage.substring(8, 11).toInt();
      long duration = controlMessage.substring(11).toInt();
      heaterSetTarget(deviceAddress, temperatureTarget1, temperatureTarget2, duration);
      Serial.println("code: 0");
    }
    // GSR Sensor API
    // Commands:
    // 0 - Read Sensor
  } else if (controlMessage[0] == 'G') {
    if (controlMessage[1] == '0') {
      uint16_t gsrReading = readGSR();
      Serial.print("ts:");
      Serial.print(millis());
      Serial.print(",value:");
      Serial.println(gsrReading);
    }
  }
}

// General API functions

// Retrieve list of connected I2C devices, along with type
void getDevices() {
  for (int i = 0; i < 128; i++) {
    Wire.beginTransmission(i);
    byte error = Wire.endTransmission();

    if (error == 0) {
      // Store device address and type
      if (gsrEnabled && ((i == 48) || (i == 96) || (i == 107))) {
        boards[i] = 'G';
      } else {
        boards[i] = i2cGetType(i);
      }
    } else {
      boards[i] = 0;
    }
  }
}

// Loop over all connected I2C actuators, sending a shutdown signal
void softShutdown() {
  for (int i = 0; i < 128; i++) {
    if (boards[i] == 'p') {
      peltierSetIntensity(i, 0, 0, 0, 0, 0);
    } else if (boards[i] == 'v') {
      vibratorSetIntensity(i, 0, 0, 0);
    } else if (boards[i] == 'h') {
      heaterSetIntensity(i, 0, 0, 0);
    }
  }
}

// TODO: Cut power to I2C device power source
void hardShutdown() {

}

// Multiply all temperature intensity values with this multiplier
void setTemperatureMultiplier(float multiplier) {
  temperatureMultiplier = multiplier;
}

// Multiply all vibration intensity values with this multiplier
void setVibrationMultiplier(float multiplier) {
  vibrationMultiplier = multiplier;
}

// #######################
// #I2C Library Functions#
// #######################

// General I2C Library

// Get Type | command
char i2cGetType(byte address) {
  Wire.beginTransmission(address);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom((int) address, 1);
  while (true) {
    if (Wire.available()) {
      return Wire.read();
    }
  }
}

// I2C Library for Peltier Slave Board API

// Set Intensity | command intensity1 direction1 intensity2 direction2 duration
void peltierSetIntensity(byte address, byte intensity1, byte direction1, byte intensity2, byte direction2, long duration) {
  Wire.beginTransmission(address);
  Wire.write(1);
  Wire.write((int) (intensity1 * temperatureMultiplier));
  Wire.write(direction1);
  Wire.write((int) (intensity2 * temperatureMultiplier));
  Wire.write(direction2);
  Wire.write((char*) &duration, 4);
  Wire.endTransmission();
}

// Read Sensors | command
void peltierReadSensors(byte address) {
  Wire.beginTransmission(address);
  Wire.write(2);
  Wire.endTransmission();

  Wire.requestFrom((int) address, 16);
  int i = 0;
  while (i < 4) {
    temperatures[i] = 0;
    int j = 0;
    // Get all 4 bytes of a temperature
    long temperature = 0;
    while (j < 4) {
      if (Wire.available()) {
        long temporaryVariable = 0;
        // Read byte and move it to correct position
        temporaryVariable = (temporaryVariable | Wire.read()) << (8 * j);
        // Store byte in correct position in temperature
        temperature = (temperature | temporaryVariable);
        j++;
      }
    }
    temperatures[i] = *(float*) &temperature;
    i++;
  }
}

// Set Minimum and Maximum temperatures | command minimumTemperatureSide1 maximumTemperatureSide1 minimumTemperatureSide2 maximumTemperatureSide2
void peltierSetMinimumMaximum(byte address, byte minimumTemperatureSide1, byte maximumTemperatureSide1, byte minimumTemperatureSide2, byte maximumTemperatureSide2) {
  Wire.beginTransmission(address);
  Wire.write(3);
  Wire.write(minimumTemperatureSide1);
  Wire.write(maximumTemperatureSide1);
  Wire.write(minimumTemperatureSide2);
  Wire.write(maximumTemperatureSide2);
  Wire.endTransmission();
}

// Set target temperatures | command target1 target2 duration
void peltierSetTarget(byte address, byte temperatureTarget1, byte temperatureTarget2, long duration) {
  Wire.beginTransmission(address);
  Wire.write(4);
  Wire.write(temperatureTarget1);
  Wire.write(temperatureTarget2);
  Wire.write((char*) &duration, 4);
  Wire.endTransmission();
}

// I2C Library for Vibrator Slave Board API

// Set Intensity | command intensity1 intensity2 duration
void vibratorSetIntensity(byte address, byte intensity1, byte intensity2, long duration) {
  Wire.beginTransmission(address);
  Wire.write(1);
  Wire.write((int) (intensity1 * vibrationMultiplier));
  Wire.write((int) (intensity2 * vibrationMultiplier));
  Wire.write((char*) &duration, 4);
  Wire.endTransmission();
}

// I2C Library for Heater Slave Board API

// Set Intensity | command intensity1 intensity2 duration
void heaterSetIntensity(byte address, byte intensity1, byte intensity2, long duration) {
  Wire.beginTransmission(address);
  Wire.write(1);
  Wire.write((int) (intensity1 * temperatureMultiplier));
  Wire.write((int) (intensity2 * temperatureMultiplier));
  Wire.write((char*) &duration, 4);
  Wire.endTransmission();
}

// Read Sensors | command
void heaterReadSensors(byte address) {
  Wire.beginTransmission(address);
  Wire.write(2);
  Wire.endTransmission();

  Wire.requestFrom((int) address, 8);
  int i = 0;
  while (i < 2) {
    temperatures[i] = 0;
    int j = 0;
    // Get all 4 bytes of a temperature
    long temperature = 0;
    while (j < 4) {
      if (Wire.available()) {
        long temporaryVariable = 0;
        // Read byte and move it to correct position
        temporaryVariable = (temporaryVariable | Wire.read()) << (8 * j);
        // Store byte in correct position in temperature
        temperature = (temperature | temporaryVariable);
        j++;
      }
    }
    temperatures[i] = *(float*) &temperature;
    i++;
  }
}

// Set Maximum temperature | command maximumTemperatureSide
void heaterSetMaximum(byte address, byte maximumTemperature) {
  Wire.beginTransmission(address);
  Wire.write(3);
  Wire.write(maximumTemperature);
  Wire.endTransmission();
}

// Set target temperatures | command target1 target2 duration
void heaterSetTarget(byte address, byte temperatureTarget1, byte temperatureTarget2, long duration) {
  Wire.beginTransmission(address);
  Wire.write(4);
  Wire.write(temperatureTarget1);
  Wire.write(temperatureTarget2);
  Wire.write((char*) &duration, 4);
  Wire.endTransmission();
}

// I2C Library for Thermistor Slave Board API

// Read Sensors | command
void thermistorReadSensors(byte address) {
  Wire.beginTransmission(address);
  Wire.write(1);
  Wire.endTransmission();

  Wire.requestFrom((int) address, 12);
  int i = 0;
  while (i < 3) {
    temperatures[i] = 0;
    int j = 0;
    // Get all 4 bytes of a temperature
    long temperature = 0;
    while (j < 4) {
      if (Wire.available()) {
        long temporaryVariable = 0;
        // Read byte and move it to correct position
        temporaryVariable = (temporaryVariable | Wire.read()) << (8 * j);
        // Store byte in correct position in temperature
        temperature = (temperature | temporaryVariable);
        j++;
      }
    }
    temperatures[i] = *(float*) &temperature;
    i++;
  }
}

// I2C Library for GSR Board

// Support function for writing to a GSR sensor register
uint8_t gsrWriteRegisterByte (uint8_t deviceAddress, uint8_t registerAddress, uint8_t newRegisterByte) {
  uint8_t result;
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  Wire.write(newRegisterByte);
  result = Wire.endTransmission();

  delay(5);
  if (result > 0) {
    Serial.print("FAIL in I2C register write! Error code: ");
    Serial.println(result);
  }

  return result;
}

// Support function for reading from a GSR sensor register
uint8_t gsrReadRegisterByte (uint8_t deviceAddress, uint8_t registerAddress) {
  uint8_t registerData;
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  Wire.endTransmission();
  Wire.requestFrom(deviceAddress,  (byte)1);
  registerData = Wire.read();

  return registerData;
}

// Initialize the GSR sensor to be ready for measurements
void initializeGSR() {
  // GPIO_A register. Set GPIO1 as analog.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_GPIO_A_REG, ECG_GSR_ENABLE_GPIO1_ANALOG);

  // GPIO_E register. Set GPIO2 as output.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_GPIO_E_REG, 0x04);

  // GPIO_O register. Enable GPIO2 output.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_GPIO_O_REG, 0x04);

  // GPIO_SR register. Set increased slew rate for GPIO1.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_GPIO_SR_REG, ECG_GSR_SET_SLEW_RATE_GPIO1);

  // EAF_CFG register. Enable EAF bias and Gain Stage.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_EAF_CFG_REG, ECG_GSR_ENABLE_BIAS_AND_GAIN);

  // EAF_GST register. Input selection, reference voltage, Gain. 0x40 -> 0x46 set gain to 1,2,4,8,16,32,64 respectively
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_EAF_GST_REG, 0x40);

  // EAF_BIAS register. Resistive biasing for GPIO1. Bits 5:7. 0: off, 1-4: GPIO0-3, 5: ECG_REF
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_EAF_BIAS_REG, ECG_GSR_SET_RES_BIAS_GPIO1);

  // MAN_SEQ_CFG register. Start Sequencer.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_MAN_SEQ_CFG_REG, ECG_GSR_START_SEQUENCER);

  // ADC_CFGB register. Enable ADC.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_ADC_CFGB_REG, ECG_GSR_ENABLE_ADC);

  // ADC_CHANNEL_MASK_L register. Select Electrical Front End.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_ADC_CHANNEL_MASK_L_REG, ECG_GSR_SELECT_EFE);
}

// Read GSR value from sensor
uint16_t readGSR(void) {
  uint8_t ADC_LOW_BYTE, ADC_HIGH_BYTE;
  uint16_t PPG_RESULT;

  // SEQ_START register. Start one ADC conversion.
  gsrWriteRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_SEQ_START_REG, ECG_GSR_START_ADC_CONVERSION);

  // Read lower 8 bits of raw ADC data.
  ADC_LOW_BYTE = gsrReadRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_ADC_DATA_L_REG);

  // Read higher 8 bits of raw ADC data.
  ADC_HIGH_BYTE = gsrReadRegisterByte(ECG_GSR_SLAVE_ADDRESS, ECG_GSR_ADC_DATA_H_REG);

  // Raw ADC result
  PPG_RESULT = (uint16_t) ADC_LOW_BYTE + ( (uint16_t) ADC_HIGH_BYTE << 8);
  return PPG_RESULT;
}
