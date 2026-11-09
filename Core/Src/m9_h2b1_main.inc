/* Included by main.c after the existing steering/safety helpers.
 * Full persistent two-axis owner. No extra I2C, no blocking UART.
 * Physical acceptance and powered tuning are distinct; no current loop.
 */
static void M9B1_MainInput(M9B1Input *in)
{
  (void)memset(in,0,sizeof(*in));
  in->now_ms=HAL_GetTick();
  in->raw[0]=steer_pos_left_raw;in->raw[1]=steer_pos_right_raw;
  in->sample_ms[0]=steer_pos_left_last_sample_ms;
  in->sample_ms[1]=steer_pos_right_last_sample_ms;
  in->sample_seq[0]=steer_pos_left_new_sample_count;
  in->sample_seq[1]=steer_pos_right_new_sample_count;
  in->valid[0]=(uint8_t)(debug_as5600_left_comm_ok &&
      AS5600_DebugReadingValid(1U,debug_as5600_left_status));
  in->valid[1]=(uint8_t)(debug_as5600_right_comm_ok &&
      AS5600_DebugReadingValid(1U,debug_as5600_right_status));
  in->enabled=m9_h2b1_enable;
  in->nc_mask=Steering_TestRawLimitMask();
  in->safety_ok=(uint8_t)(!STEER_SAFETY_INCIDENT_LOCKOUT &&
      steer_safety_motion_permit==1U && !steer_safety_trip_latched &&
      !control_stop_latched && control_mode!=CONTROL_MODE_STOP &&
      !steer_safety_neutral_required && !ps2_cancel_hold &&
      !ps2_spin_rf_latched && PS2_Pad.Connected==1U && PS2_Pad.Mode==0x73U &&
      !debug_as5600_poll_enable);
  /* Ordinary wheel traction is independent. Spin and every new steering
   * owner are excluded. No requirement for wheels_stopped here. */
  in->owners_free=(uint8_t)(!steer_pos_active && !steer_manual_active &&
      !steer_cal_active && !steer_test_running && !steer_test_arm &&
      !steer_test_request && !voice_pos_request && !voice_pos_owned &&
      !voice_rotate_stage && !WheelControl_SpinIsActive());
  in->geometry_ok=(uint8_t)(
      Steering_SafetyHardEnvelopeValid(1U,in->raw[0]) &&
      Steering_SafetyHardEnvelopeValid(0U,in->raw[1]));
  if(m9_h2b1_hold.active &&
     (!Steering_SafetyHardEnvelopeValid(1U,m9_h2b1_hold.goal[0]) ||
      !Steering_SafetyHardEnvelopeValid(0U,m9_h2b1_hold.goal[1])))
    in->geometry_ok=0U;
}
static uint32_t M9B1_HAL_Lock(void *context)
{
  uint32_t irq=__get_PRIMASK();(void)context;__disable_irq();return irq;
}
static void M9B1_HAL_Unlock(void *context,uint32_t irq)
{
  (void)context;if(irq==0U)__enable_irq();
}
static uint8_t M9B1_HAL_Allowed(void *context)
{
  M9B1Input in;(void)context;M9B1_MainInput(&in);
  if(!in.enabled || !in.safety_ok || !in.owners_free || !in.geometry_ok ||
     in.nc_mask || !in.valid[0] || !in.valid[1] ||
     (uint32_t)(in.now_ms-in.sample_ms[0])>m9_h2b1_config.sample_stale_ms ||
     (uint32_t)(in.now_ms-in.sample_ms[1])>m9_h2b1_config.sample_stale_ms ||
     m9_h2b1_nc_epoch!=m9_h2b1_seen_nc_epoch ||
     (EXTI->PR & (GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_11)))return 0U;
  if(!m9_h2b1_hold.lease &&
     (__HAL_TIM_GET_COMPARE(&htim12,TIM_CHANNEL_2)!=0U ||
      __HAL_TIM_GET_COMPARE(&htim14,TIM_CHANNEL_1)!=0U))return 0U;
  return 1U;
}
static void M9B1_HAL_Pwm(void *context,uint8_t axis,uint16_t value)
{
  (void)context;
  if(axis==0U)__HAL_TIM_SET_COMPARE(&htim12,TIM_CHANNEL_2,value);
  else __HAL_TIM_SET_COMPARE(&htim14,TIM_CHANNEL_1,value);
}
static void M9B1_HAL_Direction(void *context,uint8_t axis,int8_t raw_sign)
{
  GPIO_PinState level;(void)context;
  /* Exact M8 mapping: LEFT RAW+ DIR0; RIGHT RAW+ DIR1. */
  if(axis==0U){
    level=(raw_sign>0)?GPIO_PIN_RESET:GPIO_PIN_SET;
    HAL_GPIO_WritePin(MOTOR_DIR_TURN_left_GPIO_Port,MOTOR_DIR_TURN_left_Pin,level);
  }else{
    level=(raw_sign>0)?GPIO_PIN_SET:GPIO_PIN_RESET;
    HAL_GPIO_WritePin(MOTOR_DIR_TURN_right_GPIO_Port,MOTOR_DIR_TURN_right_Pin,level);
  }
}
static uint32_t M9B1_HAL_Now(void *context){(void)context;return HAL_GetTick();}
static const M9B1Port m9_h2b1_port={0,M9B1_HAL_Lock,M9B1_HAL_Unlock,
  M9B1_HAL_Allowed,M9B1_HAL_Pwm,M9B1_HAL_Direction,M9B1_HAL_Now};

static void M9B1_Stop(uint8_t reason)
{
  if(!m9_h2b1_hold.active && !m9_h2b1_hold.lease)return;
  M9B1_Cut(&m9_h2b1_hold,&m9_h2b1_port,reason);
  m9_h2b1_pending=1U;
}
static uint8_t M9B1_KeepOutput(void)
{
  /* Retain a valid target also during velocity initialization or while
   * the power profile is locked. This is not an actuator permission:
   * only M9B1_Apply can claim an authorized HOLDING power lease. */
  return (uint8_t)(m9_h2b1_hold.active && M9B1_HAL_Allowed(0));
}
static void M9B1_PreOwnerService(void)
{
  M9B1Input in;uint8_t buttons=0U;
  if(!m9_h2b1_hold.active && !m9_h2b1_hold.lease)return;
  M9B1_MainInput(&in);
  if(!in.enabled){M9B1_Stop(M9B1_DISABLED);return;}
  if(!in.safety_ok){M9B1_Stop(M9B1_STOP);return;}
  if(in.nc_mask || m9_h2b1_nc_epoch!=m9_h2b1_seen_nc_epoch ||
     (EXTI->PR & (GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_11))){
    M9B1_Stop(M9B1_NC);return;
  }
  if(PS2_Pad.R1)buttons|=1U;
  if(PS2_Pad.R2)buttons|=2U;
  /* A held R1 after DONE is not a new command. A fresh edge, manual
   * direction, completed Voice target, SELECT or spin releases HOLD
   * BEFORE the old owners can write their first PWM. */
  if(!in.owners_free || Steering_ManualRequestDirection()!=0 ||
     (buttons & (uint8_t)~steer_pos_previous_buttons) ||
     PS2_Pad.Select || PS2_Pad.L1 || PS2_Pad.L2){
    M9B1_Stop(M9B1_NEW_OWNER);return;
  }
  if(!in.geometry_ok || !in.valid[0] || !in.valid[1] ||
     (uint32_t)(in.now_ms-in.sample_ms[0])>m9_h2b1_config.sample_stale_ms ||
     (uint32_t)(in.now_ms-in.sample_ms[1])>m9_h2b1_config.sample_stale_ms)
    M9B1_Stop(M9B1_SENSOR);
}
static void M9B1_StartAfterDone(uint8_t result)
{
  M9B1Input in;uint32_t irq;uint16_t gl,gr;uint8_t source;
  if(!m9_h2b1_enable || result!=2U || !steer_pos_active ||
     steer_pos_left_state!=STEER_AXIS_DONE ||
     steer_pos_right_state!=STEER_AXIS_DONE ||
     steer_pos_left_result!=2U || steer_pos_right_result!=2U ||
     steer_pos_limit_trip_mask!=0U ||
     (steer_pos_command!=1U && steer_pos_command!=4U))return;
  gl=(steer_pos_command==1U)?STEER_POS_LEFT_CENTER_RAW:voice_pos_goal_l;
  gr=(steer_pos_command==1U)?STEER_POS_RIGHT_CENTER_RAW:voice_pos_goal_r;
  source=(steer_pos_command==4U)?3U:(voice_pos_owned?2U:1U);
  M9B1_Stop(M9B1_NEW_OWNER);
  M9B1_MainInput(&in);
  /* Explicit completed-position handoff: Finish has already written
   * both CCR0, and clears its active/owned flags immediately on return.
   * No other owner may be present, including an unconsumed request. */
  if(steer_manual_active || steer_cal_active || steer_test_running ||
     steer_test_arm || steer_test_request || voice_rotate_stage ||
     WheelControl_SpinIsActive() || voice_pos_request ||
     !Steering_SafetyHardEnvelopeValid(1U,gl) ||
     !Steering_SafetyHardEnvelopeValid(0U,gr))return;
  in.owners_free=1U;
  irq=M9B1_HAL_Lock(0);
  if(Steering_TestRawLimitMask()==0U &&
     !(EXTI->PR & (GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_11)) &&
     __HAL_TIM_GET_COMPARE(&htim12,TIM_CHANNEL_2)==0U &&
     __HAL_TIM_GET_COMPARE(&htim14,TIM_CHANNEL_1)==0U){
    M9B1_MainInput(&in); /* refresh safety/age inside the handoff critical section */
    in.owners_free=1U; /* the verified DONE position owner is ending */
    m9_h2b1_seen_nc_epoch=m9_h2b1_nc_epoch;
    if(m9_h2b1_sequence!=0xffffffffUL)++m9_h2b1_sequence;
    if(m9_h2b1_pending && m9_h2b1_dropped!=0xffffffffUL)++m9_h2b1_dropped;
    (void)M9B1_Begin(&m9_h2b1_hold,&m9_h2b1_config,&in,
                     m9_h2b1_sequence,source,gl,gr);
    m9_h2b1_pending=1U;m9_h2b1_report_ms=in.now_ms;
  }
  M9B1_HAL_Unlock(0,irq);
}
static void M9B1_Update(void)
{
  M9B1Input in;uint8_t previous;
  if(!m9_h2b1_hold.active && !m9_h2b1_hold.lease)return;
  M9B1_MainInput(&in);previous=m9_h2b1_hold.state;
  if(m9_h2b1_nc_epoch!=m9_h2b1_seen_nc_epoch){M9B1_Stop(M9B1_NC);return;}
  M9B1_Step(&m9_h2b1_hold,&m9_h2b1_config,&in);
  (void)M9B1_Apply(&m9_h2b1_hold,&m9_h2b1_config,&m9_h2b1_port);
  if(previous!=m9_h2b1_hold.state ||
     (uint32_t)(in.now_ms-m9_h2b1_report_ms)>=5000U){
    m9_h2b1_pending=1U;m9_h2b1_report_ms=in.now_ms;
  }
}
