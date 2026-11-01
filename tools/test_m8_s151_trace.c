#include "steer_s151_trace.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(M8S151_RIGHT_ACTIVE_CAP==200U);
 assert(M8S151_RIGHT_PROPOSED_CAP==300U);
 assert(M8S151_SignedDelta(10U,65530U)==16);
 assert(M8S151_SignedDelta(65530U,10U)==-16);
 assert(M8S151_SignedDelta(50U,60U)==-10);
 assert(M8S151_DeltaValid(50U,1U)==1U);
 assert(M8S151_DeltaValid(9U,1U)==0U);
 assert(M8S151_DeltaValid(251U,1U)==0U);
 puts("M8-S15.1 HOST PASS");
 return 0;
}
