#ifndef SCHEDULERSH
#define SCHEDULERSH

#include "lwp.h"

typedef struct scheduler *scheduler;

extern scheduler AlwaysZero;
extern scheduler ChangeOnSIGTSTP;
extern scheduler ChooseHighestColor;
extern scheduler ChooseLowestColor;
extern scheduler RoundRobin;

extern struct scheduler rr_publish;

#endif
