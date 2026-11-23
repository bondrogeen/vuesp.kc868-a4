#include "./ir.h"

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <stddef.h>

#include "../config/ConfigStore.h"
#include "../device/device.h"
#include "../tasks/tasks.h"
#include "../webserver/webserver.h"

Ir ir = {KEY_IR};

static IRrecv irrecv(GPIO_IR_RX);
static IRsend irsend(GPIO_IR_TX);
static decode_results results;
static bool receiverStarted = false;

static uint16_t read16(const uint8_t* data) {
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static void sanitize() {
  if (ir.enabled > 1) ir.enabled = 1;
  if (ir.sendProtocol != 2) ir.sendProtocol = 1;
  if (ir.sendRepeats > 20) ir.sendRepeats = 20;
  if (ir.rawLen > IR_RAW_MAX) ir.rawLen = IR_RAW_MAX;
}

static void saveConfig() {
  ir.key = KEY_IR;
  const TConfigStatus status = configStore.write("ir", &ir, sizeof(ir), CONFIG_IR_VERSION);
#if VUESP_LOG_LEVEL >= 1
  if (status != TConfigStatus::Ok) {
    Serial.printf("[config] ir write: %d\n", (int)status);
  }
#endif
}

static void startReceiver() {
  irrecv.enableIRIn();
  receiverStarted = true;
  ir.status = 1;
}

static void stopReceiver() {
  if (receiverStarted) irrecv.disableIRIn();
  receiverStarted = false;
  ir.status = 0;
}

static void sendIr() {
  if (wsClientCount() == 0) return;
  wsSendAll((uint8_t*)&ir, sizeof(ir));
}

void onWsEventIr(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  if (!arg || !data || task != KEY_IR) return;
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->len != sizeof(ir) || len < sizeof(ir)) return;

  const uint8_t command = data[1];
  if (command == CMD_SET) {
    ir.enabled = data[offsetof(Ir, enabled)] ? 1 : 0;
  } else if (command == CMD_WRITE) {
    ir.sendProtocol = data[offsetof(Ir, sendProtocol)];
    ir.sendRepeats = data[offsetof(Ir, sendRepeats)];
    ir.sendAddress = read16(data + offsetof(Ir, sendAddress));
    ir.sendCode = read16(data + offsetof(Ir, sendCode));
    if (ir.sendProtocol == 2) {
      uint16_t rawLen = read16(data + offsetof(Ir, rawLen));
      if (rawLen > IR_RAW_MAX) rawLen = IR_RAW_MAX;
      ir.rawLen = rawLen;
      memcpy(ir.raw, data + offsetof(Ir, raw), rawLen * sizeof(uint16_t));
    }
  }

  ir.command = command;
  tasks[KEY_IR] = 1;
}

void setupIr() {
  const TConfigStatus status = configStore.read("ir", &ir, sizeof(ir), CONFIG_IR_VERSION);
  if (status != TConfigStatus::Ok) {
    ir.key = KEY_IR;
    ir.command = CMD_GET;
    ir.status = 0;
    ir.enabled = 1;
    ir.sendProtocol = 1;
    ir.sendRepeats = 0;
    ir.rawLen = 0;
  }

  sanitize();
  irsend.begin();
  if (ir.enabled) {
    startReceiver();
  } else {
    stopReceiver();
  }
}

void loopIr(uint32_t now) {
  if (receiverStarted && irrecv.decode(&results)) {
    ir.protocol = (uint16_t)results.decode_type;
    const String protocolName = typeToString(results.decode_type, results.repeat);
    strncpy(ir.protocolName, protocolName.c_str(), sizeof(ir.protocolName) - 1);
    ir.protocolName[sizeof(ir.protocolName) - 1] = '\0';
    ir.bits = results.bits;
    ir.address = results.address;
    ir.code = results.command;
    ir.repeats = results.repeat;
    ir.value = (uint32_t)results.value;
    ir.rawLen = results.rawlen < IR_RAW_MAX ? results.rawlen : IR_RAW_MAX;
    for (uint16_t i = 0; i < ir.rawLen; i++) {
      ir.raw[i] = results.rawbuf[i];
    }
    ir.count++;
    ir.updated = millis();
    ir.command = CMD_READ;
    sendIr();
    irrecv.resume();
  }

  if (!tasks[KEY_IR]) return;
  tasks[KEY_IR] = 0;

  switch (ir.command) {
    case CMD_SET:
      sanitize();
      saveConfig();
      if (ir.enabled) {
        startReceiver();
      } else {
        stopReceiver();
      }
      break;
    case CMD_RESTORE:
      ir.enabled = 1;
      ir.sendProtocol = 1;
      ir.sendRepeats = 0;
      ir.rawLen = 0;
      sanitize();
      saveConfig();
      startReceiver();
      break;
    case CMD_WRITE:
      if (ir.sendProtocol == 2 && ir.rawLen > 0) {
        irsend.sendRaw(ir.raw, ir.rawLen, 38);
      } else {
        irsend.sendNEC(irsend.encodeNEC(ir.sendAddress, ir.sendCode), 32, ir.sendRepeats);
      }
      ir.sent++;
      ir.updated = millis();
      break;
    default:
      break;
  }

  ir.status = receiverStarted ? 1 : 0;
  sendIr();
}
