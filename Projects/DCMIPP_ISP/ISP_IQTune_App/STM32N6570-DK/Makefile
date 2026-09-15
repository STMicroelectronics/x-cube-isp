######################################
# quiet mode
######################################
V = 0
ifeq ($(V), 0)
  quiet = quiet_
else
  quiet =
endif
quiet_CC  = @echo "  CC $@"; $(CC)
quiet_LD  = @echo "  LD $@"; $(CC)
quiet_AS  = @echo "  AS $@"; $(AS)
quiet_SZ  = @echo "  SZ $@"; $(SZ)
quiet_HEX  = @echo "  HEX $@"; $(HEX)
quiet_BIN  = @echo "  BIN $@"; $(BIN)

######################################
# helpers
######################################
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

######################################
# target
######################################
TARGET = Project
BINARY_DIR = Binary
ifdef APP_GIT_SHA1_STRING
C_DEFS += -DAPP_GIT_SHA1_STRING=\"$(APP_GIT_SHA1_STRING)\"
endif
ifdef APP_VERSION_STRING
C_DEFS += -DAPP_VERSION_STRING=\"$(APP_VERSION_STRING)\"
endif

######################################
# building variables
######################################
OPT = -Os -g3

#######################################
# paths
#######################################
# Build path
BUILD_TOP = build
BUILD_DIR = $(BUILD_TOP)/Projects/DCMIPP_ISP/ISP_IQTune_App
PROJECT_ROOT := $(abspath ../../../../)

######################################
# source
######################################
# C sources
C_SOURCES += Src/app.c
C_SOURCES += Src/isp_conf.c
C_SOURCES += Src/dump_spinner.c
C_SOURCES += Src/stm32n6xx_it.c
C_SOURCES += Src/usbx_conf.c
C_SOURCES += ../../common/STM32N6570-DK/Src/main.c
C_SOURCES += ../../common/STM32N6570-DK/Src/utils.c
C_SOURCES += ../../common/STM32N6570-DK/Src/syscalls.c
C_SOURCES += ../../common/STM32N6570-DK/Src/threadx_hal.c

C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/cmw_camera.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/cmw_sensor_registry.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/cmw_utils.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_imx335.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_vd55g1.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_vd56g3.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_vd65g4.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_vd66gy.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_vd1943.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_vd5943.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/cmw_imx477.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/imx335/imx335.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/imx335/imx335_reg.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/vd55g1/vd55g1.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/vd6g/vd6g.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/vd1943/vd1943.c
C_SOURCES += ../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/imx477/imx477.c


C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usb_desc.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/usbx/Src/usbx.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_ae_algo.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_algo.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_awb_algo.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_cmd_parser.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_core.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_services.c
C_SOURCES += ../../../../Middlewares/ST/STM32_ISP_Library/isp/Src/isp_tool_com.c



C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_event_flags_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_event_flags_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_event_flags_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_event_flags_set.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_mutex_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_mutex_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_mutex_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_mutex_put.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_semaphore_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_semaphore_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_semaphore_put.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_thread_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_thread_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_thread_info_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_thread_resume.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_thread_suspend.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_thread_terminate.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/txe_semaphore_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_event_flags_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_event_flags_cleanup.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_event_flags_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_event_flags_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_event_flags_set.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_initialize_high_level.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_initialize_kernel_enter.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_initialize_kernel_setup.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_system_preempt_check.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_cleanup.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_initialize.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_prioritize.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_priority_change.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_mutex_put.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_semaphore_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_semaphore_cleanup.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_semaphore_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_semaphore_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_semaphore_initialize.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_semaphore_put.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_create.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_delete.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_identify.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_info_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_initialize.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_resume.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_shell_entry.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_sleep.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_suspend.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_system_resume.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_system_suspend.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_terminate.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_timeout.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_thread_time_slice.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_time_get.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_timer_expiration_process.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_timer_initialize.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_timer_system_activate.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_timer_system_deactivate.c
C_SOURCES += ../../../../Middlewares/ST/threadx/common/src/tx_timer_thread_entry.c

C_SOURCES += ../../../../Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_thread_secure_stack.c



C_SOURCES += ../../../../Middlewares/ST/threadx/utility/execution_profile_kit/tx_execution_profile.c

C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_alternate_setting_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_alternate_setting_set.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_class_register.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_class_unregister.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_clear_feature.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_configuration_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_configuration_set.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_control_request_process.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_descriptor_send.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_disconnect.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_endpoint_stall.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_get_status.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_host_wakeup.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_initialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_delete.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_set.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_interface_start.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_microsoft_extension_register.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_set_feature.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_tasks_run.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_abort.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_all_request_abort.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_request.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_transfer_run.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_device_stack_uninitialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_system_error_handler.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_system_initialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_system_tasks_run.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_system_uninitialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_delay_ms.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_descriptor_parse.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_event_flags_create.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_event_flags_delete.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_event_flags_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_event_flags_set.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_mutex_create.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_mutex_delete.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_mutex_off.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_mutex_on.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_long_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_long_get_big_endian.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_long_put.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_allocate.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_allocate_mulc_safe.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_byte_pool_create.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_byte_pool_search.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_copy.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_free.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_memory_set.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_long_put_big_endian.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_semaphore_create.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_semaphore_delete.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_semaphore_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_semaphore_put.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_set_interrupt_handler.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_short_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_short_get_big_endian.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_short_put.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_short_put_big_endian.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_string_length_check.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_thread_create.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_thread_delete.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_thread_resume.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_thread_suspend.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_timer_create.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_timer_delete.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_unicode_to_string.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/core/src/ux_utility_virtual_address.c


C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_activate.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_bulkin_thread.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_bulkout_thread.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_control_request.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_deactivate.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_entry.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_initialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_ioctl.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_read.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_tasks_run.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_unitialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_write.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_cdc_acm_write_with_callback.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_storage_verify.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_storage_write.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_activate.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_change.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_control_request.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_deactivate.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_entry.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_initialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_ioctl.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_max_payload_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_read_payload_free.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_read_payload_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_read_task_function.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_read_thread_entry.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_reception_start.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_stream_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_tasks_run.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_transmission_start.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_uninitialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_write_payload_commit.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_write_payload_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_write_task_function.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_device_classes/src/ux_device_class_video_write_thread_entry.c


C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_callback.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_create.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_destroy.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_reset.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_stall.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_endpoint_status.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_frame_number_get.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_function.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_initialize.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_initialize_complete.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_interrupt_handler.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_transfer_abort.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_transfer_request.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_transfer_run.c
C_SOURCES += ../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers/ux_dcd_stm32_uninitialize.c

C_SOURCES += ../../../../Drivers/CMSIS_Device/ST/STM32N6xx/Source/Templates/system_stm32n6xx_fsbl.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_cortex.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_dcmipp.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_dma2d.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_gpio.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_uart.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_i2c.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_i2c_ex.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_ltdc.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_ltdc_ex.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rif.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_ramcfg.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_cacheaxi.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pwr.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pwr_ex.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rcc.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_rcc_ex.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_xspi.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_bsec.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pcd.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_hal_pcd_ex.c
C_SOURCES += ../../../../Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_usb.c

C_SOURCES += ../../../../Drivers/BSP/STM32N6570-DK/stm32n6570_discovery.c
C_SOURCES += ../../../../Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_bus.c
C_SOURCES += ../../../../Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_lcd.c
C_SOURCES += ../../../../Drivers/BSP/STM32N6570-DK/stm32n6570_discovery_xspi.c

C_SOURCES += ../../../../Drivers/BSP/Components/aps256xx/aps256xx.c
C_SOURCES += ../../../../Drivers/BSP/Components/mx66uw1g45g/mx66uw1g45g.c


# ASM sources
ASM_SOURCES += ../../../../Drivers/CMSIS_Device/ST/STM32N6xx/Source/Templates/gcc/startup_stm32n657xx_fsbl.s
ASM_SOURCES_S += Src/tx_initialize_low_level.S
THREADX_PORT_SOURCES += Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_thread_schedule.S
THREADX_PORT_SOURCES += Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_thread_secure_stack_allocate.S
THREADX_PORT_SOURCES += Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_thread_secure_stack_free.S
THREADX_PORT_SOURCES += Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_thread_secure_stack_initialize.S
THREADX_PORT_SOURCES += Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_thread_stack_build.S
THREADX_PORT_SOURCES += Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_timer_interrupt.S
THREADX_PORT_SOURCES = $(wildcard ../../../../Middlewares/ST/threadx/ports/cortex_m55/gnu/src/*.S)
THREADX_PORT_SOURCES := $(filter-out ../../../../Middlewares/ST/threadx/ports/cortex_m55/gnu/src/tx_initialize_low_level.S,$(THREADX_PORT_SOURCES))



ASM_SOURCES_S += $(THREADX_PORT_SOURCES)


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
READELF = $(GCC_PATH)/$(PREFIX)readelf
LD = $(CC)
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
CP = $(PREFIX)objcopy
READELF = $(PREFIX)readelf
LD = $(CC)
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary
FLASHER = STM32_Programmer_CLI
SIGNER = STM32_SigningTool_CLI
OBJCOPY = arm-none-eabi-objcopy
EL = "$(shell dirname "$(shell which $(FLASHER))")/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr"

#######################################
# CFLAGS
#######################################
CPU = -mcpu=cortex-m55 -mcmse -mthumb
FPU = -mfpu=fpv5-d16 -mfloat-abi=hard

# mcu
MCU = $(CPU) $(FPU)

# C defines
C_DEFS += -DSTM32N657xx
C_DEFS += -DUSE_FULL_ASSERT
C_DEFS += -DUSE_FULL_LL_DRIVER
C_DEFS += -DISP_MW_TUNING_TOOL_SUPPORT
C_DEFS += -DCMW_USE_WITHOUT_ISP
C_DEFS += -DISP_ENABLE_UVC
C_DEFS += -DUSBL_PACKET_PER_MICRO_FRAME=2
C_DEFS += -DUX_INCLUDE_USER_DEFINE_FILE
C_DEFS += -DTX_SINGLE_MODE_SECURE
C_DEFS += -DTX_INCLUDE_USER_DEFINE_FILE
C_DEFS += -DTX_EXECUTION_PROFILE_ENABLE


# C includes
# Patched files
C_INCLUDES += -IInc
C_INCLUDES += -I../../common/STM32N6570-DK/Inc
C_INCLUDES += -I../../../../Drivers/BSP/STM32N6570-DK
C_INCLUDES += -I../../../../Drivers/BSP/Components/Common
C_INCLUDES += -I../../../../Drivers/CMSIS/Include
C_INCLUDES += -I../../../../Drivers/CMSIS_Device/ST/STM32N6xx/Include
C_INCLUDES += -I../../../../Drivers/STM32N6xx_HAL_Driver/Inc
C_INCLUDES += -I../../../../Middlewares/ST/STM32_ISP_Library/isp/Inc
C_INCLUDES += -I../../../../Middlewares/ST/STM32_ISP_Library/isp/usbx/Inc
C_INCLUDES += -I../../../../Middlewares/ST/STM32_Camera_Middleware
C_INCLUDES += -I../../../../Middlewares/ST/STM32_Camera_Middleware/sensors
C_INCLUDES += -I../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/imx335
C_INCLUDES += -I../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/vd6g
C_INCLUDES += -I../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/vd55g1
C_INCLUDES += -I../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/vd1943
C_INCLUDES += -I../../../../Middlewares/ST/STM32_Camera_Middleware/sensors/imx477
C_INCLUDES += -I../../../../Middlewares/ST/threadx/common/inc
C_INCLUDES += -I../../../../Middlewares/ST/threadx/ports/cortex_m55/gnu/inc
C_INCLUDES += -I../../../../Middlewares/ST/threadx/utility/execution_profile_kit
C_INCLUDES += -I../../../../Middlewares/ST/usbx/common/usbx_stm32_device_controllers
C_INCLUDES += -I../../../../Middlewares/ST/usbx/common/core/inc
C_INCLUDES += -I../../../../Middlewares/ST/usbx/common/usbx_device_classes/inc
C_INCLUDES += -I../../../../Middlewares/ST/usbx/ports/generic/inc/

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fstack-usage -fdata-sections -ffunction-sections -fcyclomatic-complexity --specs=nano.specs
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fstack-usage -fdata-sections -ffunction-sections -fcyclomatic-complexity --specs=nano.specs
# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

CFLAGS += -std=gnu11

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32CubeIDE/STM32N657xx.ld

# libraries
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) $(LDFLAGS_OTHERS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
# Uncomment to enable %f formatted output
LDFLAGS_OTHERS += -u _printf_float
# Avoid 'build/Project.elf has a LOAD segment with RWX permissions' warning
LDFLAGS_OTHERS += -Wl,--no-warn-rwx-segments
LDFLAGS += -Wl,--print-memory-usage

#######################################
# base build
#######################################
.PHONY: all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

.PHONY: sign
sign: $(BUILD_DIR)/$(TARGET)_sign.hex

#######################################
# build the application
#######################################
REL_C_SOURCES := $(patsubst $(PROJECT_ROOT)/%,%,$(abspath $(C_SOURCES)))
REL_ASM_SOURCES := $(patsubst $(PROJECT_ROOT)/%,%,$(abspath $(ASM_SOURCES)))
REL_ASM_SOURCES_S := $(patsubst $(PROJECT_ROOT)/%,%,$(abspath $(ASM_SOURCES_S)))

OBJECTS = $(addprefix $(BUILD_DIR)/,$(REL_C_SOURCES:.c=.o))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(REL_ASM_SOURCES:.s=.o))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(REL_ASM_SOURCES_S:.S=.o))


$(BUILD_DIR)/%.o: $(PROJECT_ROOT)/%.c Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$($(quiet)CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: $(PROJECT_ROOT)/%.S Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$($(quiet)CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: $(PROJECT_ROOT)/%.s Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$($(quiet)AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).list: $(OBJECTS)
	$(file > $@, $(OBJECTS))

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile $(BUILD_DIR)/$(TARGET).list
	$($(quiet)LD) @$(BUILD_DIR)/$(TARGET).list $(LDFLAGS) -o $@
	$($(quiet)SZ) $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR)/$(TARGET)_sign.bin: $(BUILD_DIR)/$(TARGET).bin
	$(SIGNER) -s -bin $< -nk -t ssbl -hv 2.3 -o $(BUILD_DIR)/$(TARGET)_sign.bin

$(BUILD_DIR)/$(TARGET)_sign.hex: $(BUILD_DIR)/$(TARGET)_sign.bin
	$(CP) -I binary $< --change-addresses 0x70100000 -O ihex $@

$(BUILD_DIR):
	mkdir -p $@

#######################################
# clean up
#######################################
.PHONY: clean
clean:
	-rm -fR $(BUILD_TOP)

#######################################
# dependencies
#######################################
-include $(call rwildcard,$(BUILD_TOP),*.d)
