#include "vehicle_command.h"

#include <stddef.h>

void VehicleRequest_Reset(VehicleRequest_t *request)
{
    if (request == NULL) {
        return;
    }

    request->source = VEH_SOURCE_NONE;
    request->drive = VEH_DRIVE_HOLD;
    request->steering = VEH_STEER_HOLD;
    request->target_speed_rpm = 0U;
    request->duration_ms = 0U;
    request->issued_at_ms = 0U;
}

VehicleRequest_t VehicleRequest_Create(VehicleSource_t source,
                                       VehicleDriveCommand_t drive,
                                       VehicleSteeringCommand_t steering,
                                       uint16_t target_speed_rpm,
                                       uint32_t duration_ms,
                                       uint32_t issued_at_ms)
{
    VehicleRequest_t request;

    request.source = source;
    request.drive = drive;
    request.steering = steering;
    request.target_speed_rpm = target_speed_rpm;
    request.duration_ms = duration_ms;
    request.issued_at_ms = issued_at_ms;

    if ((drive == VEH_DRIVE_STOP) || (drive == VEH_DRIVE_HOLD)) {
        request.target_speed_rpm = 0U;
    }

    return request;
}

bool VehicleRequest_IsValid(const VehicleRequest_t *request)
{
    if (request == NULL) {
        return false;
    }

    if ((request->drive < VEH_DRIVE_HOLD) ||
        (request->drive > VEH_DRIVE_BACKWARD)) {
        return false;
    }

    if ((request->steering < VEH_STEER_HOLD) ||
        (request->steering > VEH_STEER_RIGHT)) {
        return false;
    }

    if ((request->source <= VEH_SOURCE_NONE) ||
        (request->source > VEH_SOURCE_SAFETY)) {
        return false;
    }

    if ((request->drive == VEH_DRIVE_HOLD) &&
        (request->steering == VEH_STEER_HOLD)) {
        return false;
    }

    switch (request->drive) {
    case VEH_DRIVE_FORWARD:
    case VEH_DRIVE_BACKWARD:
        return request->target_speed_rpm > 0U;

    case VEH_DRIVE_HOLD:
    case VEH_DRIVE_STOP:
        return request->target_speed_rpm == 0U;

    default:
        return false;
    }
}

bool VehicleRequest_IsExpired(const VehicleRequest_t *request,
                              uint32_t current_time_ms)
{
    uint32_t elapsed_ms;

    if (!VehicleRequest_IsValid(request)) {
        return true;
    }

    if (request->duration_ms == 0U) {
        return false;
    }

    elapsed_ms = current_time_ms - request->issued_at_ms;
    return elapsed_ms >= request->duration_ms;
}

const char *VehicleDriveCommand_ToString(VehicleDriveCommand_t command)
{
    switch (command) {
    case VEH_DRIVE_HOLD:     return "HOLD";
    case VEH_DRIVE_STOP:     return "STOP";
    case VEH_DRIVE_FORWARD:  return "FORWARD";
    case VEH_DRIVE_BACKWARD: return "BACKWARD";
    default:                 return "UNKNOWN_DRIVE_COMMAND";
    }
}

const char *VehicleSteeringCommand_ToString(VehicleSteeringCommand_t command)
{
    switch (command) {
    case VEH_STEER_HOLD:   return "HOLD";
    case VEH_STEER_LEFT:   return "LEFT";
    case VEH_STEER_CENTER: return "CENTER";
    case VEH_STEER_RIGHT:  return "RIGHT";
    default:               return "UNKNOWN_STEERING_COMMAND";
    }
}

const char *VehicleSource_ToString(VehicleSource_t source)
{
    switch (source) {
    case VEH_SOURCE_NONE:   return "NONE";
    case VEH_SOURCE_PS2:    return "PS2";
    case VEH_SOURCE_VOICE:  return "VOICE";
    case VEH_SOURCE_SAFETY: return "SAFETY";
    default:                return "UNKNOWN_SOURCE";
    }
}
