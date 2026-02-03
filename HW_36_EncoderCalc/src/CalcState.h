#include <stdint.h>

typedef enum {
    CalcState_Ready = 0,
    CalcState_GotFirst = 1,
    CalcState_GotOperation = 2,
    CalcState_GotSecond = 3,
    CalcState_GotResult = 4
} CalcState;

void UpdateState();