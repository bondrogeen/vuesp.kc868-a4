#include "./device.h"

#include "../config/ConfigStore.h"

Buffer myBuffer = {KEY_BUFFER};

Device device = {KEY_DEVICE, 0, 0, 0, 0, 0, 2, "text"};

uint32_t lastTimeDevice = 0;
uint32_t lastLoopInput = 0;

static const uint8_t relayPins[PORTS_LEN] = {GPIO_RELAY1, GPIO_RELAY2, GPIO_RELAY3, GPIO_RELAY4};
static const uint8_t inputPins[PORTS_LEN] = {GPIO_INPUT1, GPIO_INPUT2, GPIO_INPUT3, GPIO_INPUT4};

struct PortDevice {
  uint8_t isPressed;
  uint8_t count;
  uint8_t value;
  uint8_t valueOld;
  uint32_t pressStart;
};

PortDevice portsDevice[PORTS_LEN] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
};

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}

// Inputs are opto-isolated and active low. Keep the raw levels in the bitmask
// (bit 1 = inactive, bit 0 = active) to match the dashboard and input().
void getInput() {
  uint8_t value = 0;
  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    if (digitalRead(inputPins[i]) == HIGH) value |= (1 << i);
  }
  device.input = value;
}

// Relays are driven directly through 74HCT125 + ULN2003A; high = relay on.
void setOutput() {
  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    digitalWrite(relayPins[i], (device.output & (1 << i)) ? HIGH : LOW);
  }
}

void getOutput() {
  uint8_t value = 0;
  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    if (digitalRead(relayPins[i]) == HIGH) value |= (1 << i);
  }
  device.output = value;
}

void getGPIO() {
  getInput();
  onSendDevice();
}

void getData() {
  getInput();
}

void loopInput(uint32_t now) {
  for (int i = 0; i < PORTS_LEN; i++) {
    const uint32_t time = now - portsDevice[i].pressStart;

    if (time > REPEAT_START_TIME) {
      if (portsDevice[i].isPressed && portsDevice[i].count == 1) {
        scriptRunner.emitEvent("port_r", 2, i, portsDevice[i].value);
      } else {
        portsDevice[i].count = 0;
      }
    }
    if (time > LONG_PRESS_TIME && time < REPEAT_START_TIME) {
      if (!portsDevice[i].isPressed && portsDevice[i].count == 1) {
        scriptRunner.emitEvent("port_l", 2, i, portsDevice[i].value);
        portsDevice[i].count = 0;
      }
    }
    if (time > CLICK_WINDOW) {
      if (!portsDevice[i].isPressed && portsDevice[i].count) {
        scriptRunner.emitEvent("port_c", 3, i, portsDevice[i].value, portsDevice[i].count);
        portsDevice[i].count = 0;
      }
    }
  }
}

void findPort() {
  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    const uint8_t valueNew = (device.input & (1 << i)) != 0;
    if (portsDevice[i].value != valueNew) {
      portsDevice[i].isPressed = portsDevice[i].valueOld != valueNew;
      if (portsDevice[i].isPressed) {
        portsDevice[i].count++;
        portsDevice[i].pressStart = millis();
      }
      portsDevice[i].value = valueNew;
    }
  }
}

void deviceGPIO(Port* port, uint8_t type) {
  (void)port;
  (void)type;
}

static char displayBuffer[65] = "5";

bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
#if VUESP_LOG_LEVEL >= 3
  Serial.print(id);
  Serial.print(write ? "WRITE" : "READ");
  Serial.print(" kind=");
  Serial.println(kind);
#endif

  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        char buf[65];
        uint8_t len = value.stringVal.len;
        if (len > 64) len = 64;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
        strcpy(displayBuffer, buf);
#if VUESP_LOG_LEVEL >= 3
        Serial.print("DISPLAY: ");
        Serial.println(buf);
#endif
      } else if (kind == KIND_INT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%d", value.intVal);
      } else if (kind == KIND_UINT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%u", value.uintVal);
      } else if (kind == KIND_FLOAT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%.2f", value.floatVal);
      }
      return true;
    }

    if (kind == KIND_STRING) {
      value.stringVal.data = (uint8_t*)displayBuffer;
      value.stringVal.len = strlen(displayBuffer);
    } else if (kind == KIND_INT) {
      value.intVal = atoi(displayBuffer);
    } else if (kind == KIND_UINT) {
      value.uintVal = (uint32_t)atoi(displayBuffer);
    } else if (kind == KIND_FLOAT) {
      value.floatVal = atof(displayBuffer);
    }
    return true;
  }
  return false;
}

bool httpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;

  result.type = VAL_INT;
  result.intVal = 200;
  return true;
}

bool inputHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount != 1) return false;
  const uint32_t pin = params[0].uintVal;
  if (pin >= 1 && pin <= PORTS_LEN) {
    result.type = VAL_UINT;
    result.intVal = bitRead(device.input, pin - 1);
    return true;
  }
  return false;
}

bool outputHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 1) return false;
  const uint32_t pin = params[0].uintVal;
  if (pin >= PORTS_LEN) return false;

  if (paramCount >= 2) {
    const uint32_t value = params[1].uintVal;
    if (value) {
      bitSet(device.output, pin);
    } else {
      bitClear(device.output, pin);
    }
    setOutput();
    onSendDevice();
  }

  result.type = VAL_UINT;
  result.intVal = bitRead(device.output, pin);
  return true;
}

void setupDevice() {
  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.registerFunction("http", httpHandler);
  scriptRunner.registerFunction("input", inputHandler);
  scriptRunner.registerFunction("output", outputHandler);

  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
    pinMode(inputPins[i], INPUT);
  }

  device.output = 0;
  setOutput();
  getInput();

  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    const uint8_t valueNew = (device.input & (1 << i)) != 0;
    portsDevice[i].value = valueNew;
    portsDevice[i].valueOld = valueNew;
    portsDevice[i].count = 0;
    portsDevice[i].isPressed = 0;
  }
}

void setupFirstDevice() {
  const TConfigStatus status =
      configStore.read("device", &device, sizeof(device), CONFIG_DEVICE_VERSION);
  if (status == TConfigStatus::Ok) {
    device.command = 0;
  }
#if VUESP_LOG_LEVEL >= 1
  else if (status != TConfigStatus::NotFound) {
    Serial.printf("[config] device read: %d\n", (int)status);
  }
#endif
}

void loopDevice(uint32_t now) {
  if (now - lastLoopInput > 20) {
    lastLoopInput = now;
    getInput();
    findPort();
    loopInput(now);
  }

  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSendDevice();
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  }

  if (tasks[KEY_DEVICE]) {
    if (device.command == 2) {
      setOutput();
    } else if (device.command == 4 || device.command == CMD_SAVE) {
      const TConfigStatus status =
          configStore.write("device", &device, sizeof(device), CONFIG_DEVICE_VERSION);
#if VUESP_LOG_LEVEL >= 1
      if (status != TConfigStatus::Ok) {
        Serial.printf("[config] device write: %d\n", (int)status);
      }
#endif
    } else {
      getData();
    }

    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  }
}
