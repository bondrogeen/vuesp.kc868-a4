#ifndef IR_H
#define IR_H

#include "../const/protocol.h"

void setupIr();
void loopIr(uint32_t now);
void onWsEventIr(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

extern Ir ir;

#endif
