#ifndef CAMERA_SENSOR_REGISTRY_H
#define CAMERA_SENSOR_REGISTRY_H

#include "cmw_sensors_if.h"
#include "cmw_camera.h"

#ifdef USE_IMX335_SENSOR
#include "cmw_imx335.h"
#endif

#ifdef USE_OV5640_SENSOR
#include "cmw_ov5640.h"
#endif

#ifdef USE_VD55G1_SENSOR
#include "cmw_vd55g1.h"
#endif

#ifdef USE_VD56G3_SENSOR
#include "cmw_vd56g3.h"
#endif

#ifdef USE_VD65G4_SENSOR
#include "cmw_vd65g4.h"
#endif

#ifdef USE_VD66GY_SENSOR
#include "cmw_vd66gy.h"
#endif

#ifdef USE_VD1943_SENSOR
#include "cmw_vd1943.h"
#endif

#ifdef USE_VD5943_SENSOR
#include "cmw_vd5943.h"
#endif

#ifdef USE_IMX477_SENSOR
#include "cmw_imx477.h"
#endif

typedef union {
    #ifdef USE_IMX335_SENSOR
    CMW_IMX335_t imx335_ctx;
    #endif
    #ifdef USE_OV5640_SENSOR
    CMW_OV5640_t ov5640_ctx;
    #endif
    #ifdef USE_VD55G1_SENSOR
    CMW_VD55G1_t vd55g1_ctx;
    #endif
    #ifdef USE_VD56G3_SENSOR
    CMW_VD56G3_t vd56g3_ctx;
    #endif
    #ifdef USE_VD65G4_SENSOR
    CMW_VD65G4_t vd65g4_ctx;
    #endif
    #ifdef USE_VD66GY_SENSOR
    CMW_VD66GY_t vd66gy_ctx;
    #endif
    #ifdef USE_VD1943_SENSOR
    CMW_VD1943_t vd1943_ctx;
    #endif
    #ifdef USE_VD5943_SENSOR
    CMW_VD5943_t vd5943_ctx;
    #endif
    #ifdef USE_IMX477_SENSOR
    CMW_IMX477_t imx477_ctx;
    #endif
} sensor_ctx_u;


extern camera_sensor_t cmw_sensor_registry[];
extern const int cmw_sensor_registry_count;

#endif // CAMERA_SENSOR_REGISTRY_H
