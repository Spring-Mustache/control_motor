#include <assert.h>
#include <stdio.h>
#include "steer_s149_terminal_lead.h"
int main(void)
{
  M8S149Prediction_t p;
  uint16_t final;
  /* At comparable time-to-cutoff, observed terminal lag drives
   * positive bounded demand long BEFORE 180 RAW. */
  p=M8S149_Predict(500,500,790,790,640,640,440,440);
  assert(p.predicted_skew_ms==337);
  assert(p.request_ccr>0U&&p.request_ccr<=25U);
  final=M8S149_ChooseRight(175,175,200,175,p.request_ccr);
  assert(final==187U);           /* +12 max over last final */
  final=M8S149_ChooseRight(175,197,200,195,p.request_ccr);
  assert(final==198U);           /* chosen 175+23, within Level2 cap200 */
  final=M8S149_ChooseRight(125,145,150,145,p.request_ccr);
  assert(final==150U);           /* never >150 Level1 */
  /* If R already predicts a large early lead, no extra request. */
  p=M8S149_Predict(700,185,790,790,640,640,440,440);
  assert(p.predicted_skew_ms<50&&p.request_ccr==0U);
  /* Sensor unavailable and terminal cutoffs are hard no-op. */
  assert(M8S149_Predict(180,400,790,790,640,640,440,440).request_ccr==0U);
  assert(M8S149_Predict(400,180,790,790,640,640,440,440).request_ccr==0U);
  assert(M8S149_Predict(400,400,790,790,640,640,0,440).request_ccr==0U);
  assert(M8S149_Predict(400,400,790,790,0,640,440,440).request_ccr==0U);
  /* Never start a stopped motor / reduce an authorized hybrid PWM. */
  assert(M8S149_ChooseRight(0,0,200,180,25)==0U);
  assert(M8S149_ChooseRight(175,198,200,200,0)==198U);
  assert(M8S149_ChooseRight(175,175,175,175,25)==175U);
  /* Rise clamped even if terminal prediction requests full +25. */
  assert(M8S149_ChooseRight(175,175,200,175,25)==187U);
  puts("PASS M8-S14.9: time-to-180 predictive request bounded,");
  puts("      max-not-sum arbitration, Level1/2 caps, +12, PWM0.");
  return 0;
}
