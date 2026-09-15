#include "cmw_camera_conf.h"
#include "cmw_sensor_registry.h"

sensor_ctx_u sensor_ctx; /* Global variable to hold sensor contexts */


/* Registry of available sensors */
camera_sensor_t cmw_sensor_registry[] = {
#ifdef USE_IMX335_SENSOR
    {
        .name = "IMX335",
        .init = CMW_CAMERA_IMX335_Init,
        .set_defaultSensorValues = CMW_IMX335_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.imx335_ctx
    },
#endif
#ifdef USE_VD66GY_SENSOR
    {
        .name = "VD66GY",
        .init = CMW_CAMERA_VD66GY_Init,
        .set_defaultSensorValues = CMW_VD66GY_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.vd66gy_ctx
    },
#endif
#ifdef USE_VD56G3_SENSOR
    {
        .name = "VD56G3",
        .init = CMW_CAMERA_VD56G3_Init,
        .set_defaultSensorValues = CMW_VD56G3_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.vd56g3_ctx
    },
#endif
#ifdef USE_OV5640_SENSOR
    {
        .name = "OV5640",
        .init = CMW_CAMERA_OV5640_Init,
        .set_defaultSensorValues = CMW_OV5640_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.ov5640_ctx
    },
#endif
#ifdef USE_VD55G1_SENSOR
    {
        .name = "VD55G1",
        .init = CMW_CAMERA_VD55G1_Init,
        .set_defaultSensorValues = CMW_VD55G1_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.vd55g1_ctx
    },
#endif
#ifdef USE_VD65G4_SENSOR
    {
        .name = "VD65G4",
        .init = CMW_CAMERA_VD65G4_Init,
        .set_defaultSensorValues = CMW_VD65G4_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.vd65g4_ctx
    },
#endif
#ifdef USE_VD1943_SENSOR
    {
        .name = "VD1943",
        .init = CMW_CAMERA_VD1943_Init,
        .set_defaultSensorValues = CMW_VD1943_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.vd1943_ctx
    },
#endif
#ifdef USE_VD5943_SENSOR
    {
        .name = "VD5943",
        .init = CMW_CAMERA_VD5943_Init,
        .set_defaultSensorValues = CMW_VD5943_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.vd5943_ctx
    },
#endif
#ifdef USE_IMX477_SENSOR
    {
        .name = "IMX477",
        .init = CMW_CAMERA_IMX477_Init,
        .set_defaultSensorValues = CMW_IMX477_SetDefaultSensorValues,
        .sensor_ctx = &sensor_ctx.imx477_ctx
    },
#endif
};

const int cmw_sensor_registry_count = sizeof(cmw_sensor_registry) / sizeof(cmw_sensor_registry[0]);
