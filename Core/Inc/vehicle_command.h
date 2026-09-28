#ifndef VEHICLE_COMMAND_H_
#define VEHICLE_COMMAND_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Nguon tao ra yeu cau chuyen dong.
 * Gia tri nay se duoc bo phan phan xu uu tien su dung o milestone sau.
 */
typedef enum
{
    VEH_SOURCE_NONE = 0,
    VEH_SOURCE_PS2,
    VEH_SOURCE_VOICE,
    VEH_SOURCE_SAFETY
} VehicleSource_t;

/* Lenh cho nhom bon dong co dan dong. */
typedef enum
{
    VEH_DRIVE_HOLD = 0,
    VEH_DRIVE_STOP,
    VEH_DRIVE_FORWARD,
    VEH_DRIVE_BACKWARD
} VehicleDriveCommand_t;

/* Lenh cho nhom hai dong co danh lai. */
typedef enum
{
    VEH_STEER_HOLD = 0,
    VEH_STEER_LEFT,
    VEH_STEER_CENTER,
    VEH_STEER_RIGHT
} VehicleSteeringCommand_t;

/*
 * Goi yeu cau chuyen dong dung chung cho PS2, VC-02 va Safety Manager.
 *
 * drive             : Trang thai mong muon cua bon banh dan dong.
 * steering          : Trang thai mong muon cua hai banh danh lai.
 * target_speed_rpm  : Do lon toc do mong muon. Chieu chuyen dong nam trong
 *                     truong drive, vi vay gia tri nay luon khong am.
 * duration_ms      : 0 = khong tu dong het han; >0 = het han sau thoi gian nay.
 * issued_at_ms     : Moc thoi gian tao lenh, thuong lay tu HAL_GetTick().
 *
 * HOLD nghia la request khong thay doi nhom co cau do. Vi du, request chi
 * danh lai trai co drive = VEH_DRIVE_HOLD va steering = VEH_STEER_LEFT.
 */
typedef struct
{
    VehicleSource_t source;
    VehicleDriveCommand_t drive;
    VehicleSteeringCommand_t steering;
    uint16_t target_speed_rpm;
    uint32_t duration_ms;
    uint32_t issued_at_ms;
} VehicleRequest_t;

/* Dat request ve trang thai rong/khong hop le. */
void VehicleRequest_Reset(VehicleRequest_t *request);

/* Tao request va tu dong chuan hoa cac lenh STOP/NONE. */
VehicleRequest_t VehicleRequest_Create(VehicleSource_t source,
                                       VehicleDriveCommand_t drive,
                                       VehicleSteeringCommand_t steering,
                                       uint16_t target_speed_rpm,
                                       uint32_t duration_ms,
                                       uint32_t issued_at_ms);

/* Kiem tra noi dung request co hop le hay khong. */
bool VehicleRequest_IsValid(const VehicleRequest_t *request);

/*
 * Kiem tra lenh da het thoi gian thuc hien chua.
 * Ham van hoat dong dung khi bo dem uint32_t tran vong.
 */
bool VehicleRequest_IsExpired(const VehicleRequest_t *request,
                              uint32_t current_time_ms);

/* Chuoi ten phuc vu debug UART; khong dung de ra quyet dinh dieu khien. */
const char *VehicleDriveCommand_ToString(VehicleDriveCommand_t command);
const char *VehicleSteeringCommand_ToString(VehicleSteeringCommand_t command);
const char *VehicleSource_ToString(VehicleSource_t source);

#ifdef __cplusplus
}
#endif

#endif /* VEHICLE_COMMAND_H_ */
