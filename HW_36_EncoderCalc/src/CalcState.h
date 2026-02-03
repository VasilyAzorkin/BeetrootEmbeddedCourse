#include <stdint.h>

typedef enum {
    CalcState_Ready = 0,
    CalcState_GotFirst = 1,
    CalcState_GotOperation = 2,
    CalcState_GotResult = 3
} CalcState;

void UpdateState();