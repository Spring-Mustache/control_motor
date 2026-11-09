/* Executed inside the fixture main with the REAL production profile3. */
#if defined(M9B1_USE_FINAL_PROFILE)
 steer_pos_left_raw=1886;steer_pos_right_raw=662;sample_at(0);begin_center(2);
 for(ms=10;ms<=60000;ms+=10)pass(ms);
 assert(m9_h2b1_hold.active && !m9_h2b1_hold.ever_applied);
 /* Actual idle poll: one read every10ms, alternating sides, without a
  * debug activation; old50ms cadence retained when no HOLD is active. */
 steer_pos_idle_last_ms=clock_ms;idle_reads=0;steer_pos_idle_next_side=0;
 for(unsigned t=0;t<100;t++){SysTick_Handler();Steering_PositionTelemetryIdleUpdate();M9B1_Update();}
 assert(idle_reads==10 && idle_sides==3);
 /* >32RAW has no cancellation; M8 cap300/100 reached under stiction. */
 steer_pos_left_raw=1850;steer_pos_right_raw=698;
 for(unsigned t=0;t<700;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 assert(m9_h2b1_hold.active && hardware_pwm[0]==300 && hardware_pwm[1]==100);
 assert(hardware_dir[0]==GPIO_PIN_RESET && hardware_dir[1]==GPIO_PIN_RESET);
 assert(m9_b1_pulse_armed==3);
 /* Goal feedback wins immediately over boost and velocity memory. */
 steer_pos_left_raw=1886;steer_pos_right_raw=662;pass(clock_ms+10);
 assert(!hardware_pwm[0] && !hardware_pwm[1] && !m9_b1_pulse_armed);
 /* Restart an eligible target. Main stops running, but actual SysTick
  * cuts both PWM within the50ms lease and latches revocation on resume. */
 Turn_Motors_Stop();steer_pos_left_raw=1850;steer_pos_right_raw=698;
 sample_at(clock_ms);begin_center(2);
 for(unsigned t=0;t<200;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 assert(hardware_pwm[0] && hardware_pwm[1]);
 for(unsigned t=0;t<50;t++)SysTick_Handler();
 assert(!hardware_pwm[0] && !hardware_pwm[1] && m9_b1_pulse_expired);
 pass(clock_ms);assert(!m9_h2b1_hold.active && m9_h2b1_hold.reason==M9B1_SERVICE_GAP);
 for(unsigned t=0;t<300;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 assert(!m9_h2b1_hold.active && !hardware_pwm[0] && !hardware_pwm[1]);
 /* Fast main cannot renew an old sensor sample indefinitely. */
 sample_at(clock_ms);begin_center(2);
 for(unsigned t=0;t<200;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 for(unsigned t=0;t<80;t++){SysTick_Handler();M9B1_Update();}
 assert(!hardware_pwm[0] && !hardware_pwm[1] && !m9_h2b1_hold.active);
 /* Every safety/owner input revokes the real port, including bounced NC.
  * Returning the input to normal must not resume the old goal. */
 for(unsigned fault=0;fault<8;fault++){
  sample_at(clock_ms);begin_center(2);
  for(unsigned t=0;t<200;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
  assert(hardware_pwm[0]);
  switch(fault){
   case 0:control_stop_latched=1;break;
   case 1:manual_direction=1;break;
   case 2:debug_as5600_left_comm_ok=0;break;
   case 3:EXTI->PR=GPIO_PIN_10;break;
   case 4:PS2_Pad.Connected=0;break;
   case 5:steer_pos_left_raw=2300;break;
   case 6:Steering_SafetyOnLimitInterrupt(STEER_LIMIT_L_MIN_MASK);break;
   default:PS2_Pad.Select=1;break;
  }
  M9B1_PreOwnerService();
  assert(!m9_h2b1_hold.active && !hardware_pwm[0] && !hardware_pwm[1]);
  control_stop_latched=manual_direction=PS2_Pad.Select=0;debug_as5600_left_comm_ok=1;
  EXTI->PR=0;PS2_Pad.Connected=1;steer_pos_left_raw=1850;
  pass(clock_ms+10);assert(!m9_h2b1_hold.active && !m9_b1_pulse_armed);
 }
 /* Timer wrap and fresh voice angle BOTH-DONE handoff. */
 sample_at(UINT32_MAX-30);steer_pos_active=1;steer_pos_command=4;
 steer_pos_left_state=steer_pos_right_state=STEER_AXIS_DONE;
 steer_pos_left_result=steer_pos_right_result=2;
 voice_pos_goal_l=1870;voice_pos_goal_r=680;Steering_PositionFinish(2);
 assert(m9_h2b1_hold.active && m9_h2b1_hold.source==3);
 for(unsigned t=0;t<200;t++){SysTick_Handler();if(t%10==0)pass(clock_ms);}
 assert(m9_h2b1_hold.active && hardware_pwm[0] && hardware_pwm[1]);
 Turn_Motors_Stop();assert(!m9_b1_pulse_armed);
 idle_reads=0;steer_pos_idle_last_ms=clock_ms;
 for(unsigned t=0;t<100;t++){SysTick_Handler();Steering_PositionTelemetryIdleUpdate();M9B1_Update();}
 assert(idle_reads==2);
 puts("PASS FINAL real main/HAL/SysTick: quiet60s,10ms alternating poll, sustained300/100, feedback cutoff, lease50ms, stale sensor80ms, STOP/NC/owner/link/range, no auto-rearm, wrap, angle handoff and Rotate");
 return 0;
#endif
