#pragma pack(push, 1)
#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "zcl_multistate_input.h"
#include "zcl_onoff_configuration.h"
#include "custom_zcl/zcl_temperature_measurement.h"
#pragma pack(pop)

#include "telink_size_t_hack.h"

#include "hal/zigbee.h"
#include "zigbee/consts.h"
#include "telink_zigbee_hal.h"

// Storage for Telink endpoint configuration
static af_simple_descriptor_t endpoint_descriptors[MAX_ENDPOINTS];
static u16 in_clusters[MAX_IN_CLUSTERS];
static u16 out_clusters[MAX_OUT_CLUSTERS];
static zcl_specClusterInfo_t cluster_infos[MAX_IN_CLUSTERS + MAX_OUT_CLUSTERS];
static zclAttrInfo_t attr_tables[MAX_ATTRS];

// HAL state tracking
static hal_zigbee_endpoint *hal_endpoints = NULL;
static uint8_t hal_endpoints_cnt = 0;
static hal_attribute_change_callback_t attribute_change_callback = NULL;

static cluster_registerFunc_t get_register_func_by_cluster_id(u16 cluster_id) {
  if (cluster_id == ZCL_CLUSTER_GEN_BASIC) { // Basic cluster
    return zcl_basic_register;
  }
  if (cluster_id == ZCL_CLUSTER_GEN_IDENTIFY) { // Identify cluster
    return zcl_identify_register;
  }
  if (cluster_id == ZCL_CLUSTER_GEN_GROUPS) { // Groups cluster
    return zcl_group_register;
  }
  if (cluster_id ==
      ZCL_CLUSTER_GEN_ON_OFF_SWITCH_CONFIG) { // On/Off Switch Configuration
    return zcl_onoff_configuration_register;
  }
  if (cluster_id == ZCL_CLUSTER_GEN_LEVEL_CONTROL) { // Level Control cluster
    return zcl_level_register;
  }
  if (cluster_id == ZCL_CLUSTER_GEN_ON_OFF) { // On/Off cluster
    return zcl_onOff_register;
  }
  if (cluster_id ==
      ZCL_CLUSTER_GEN_MULTISTATE_INPUT_BASIC) { // Multistate Input
    return zcl_multistate_input_register;
  }
  if (cluster_id == ZCL_CLUSTER_TEMPERATURE_MEASUREMENT) { // Temperature Measurement
    return zcl_temperature_measurement_register;
  }
  return NULL;
}

static status_t cmd_callback(u8 endpoint, u8 clusterId, u8 cmdId,
                             void *cmdPayload) {
  hal_zigbee_cluster *cluster = hal_zigbee_find_cluster(
      hal_endpoints, hal_endpoints_cnt, endpoint, clusterId);
  if (cluster && cluster->cmd_callback) {
    return cluster->cmd_callback(endpoint, clusterId, cmdId, cmdPayload);
  }
  return (ZCL_STA_SUCCESS);
}

static status_t cmd_callback_on_off(zclIncomingAddrInfo_t *pAddrInfo, u8 cmdId,
                                    void *cmdPayload) {
  return cmd_callback(pAddrInfo->dstEp, ZCL_CLUSTER_GEN_ON_OFF, cmdId,
                      cmdPayload);
}

static cluster_forAppCb_t get_cmd_callback_by_cluster_id(u16 cluster_id) {
  if (cluster_id == ZCL_CLUSTER_GEN_ON_OFF) { // On/Off cluster
    return cmd_callback_on_off;
  }
  return NULL;
}

static void zcl_incoming_message_callback(zclIncoming_t *pInHdlrMsg) {
  if (pInHdlrMsg->hdr.cmd == ZCL_CMD_WRITE ||
      pInHdlrMsg->hdr.cmd == ZCL_CMD_WRITE_NO_RSP) {
    if (attribute_change_callback == NULL) {
      return;
    }
    zclWriteCmd_t *writeCmd = (zclWriteCmd_t *)pInHdlrMsg->attrCmd;
    for (u8 i = 0; i < writeCmd->numAttr; i++) {
      printf("Attr write on endpoint %d, cluster %d, attribute %d\r\n",
             pInHdlrMsg->msg->indInfo.dst_ep,
             pInHdlrMsg->msg->indInfo.cluster_id, writeCmd->attrList[i].attrID);
      attribute_change_callback(pInHdlrMsg->msg->indInfo.dst_ep,
                                pInHdlrMsg->msg->indInfo.cluster_id,
                                writeCmd->attrList[i].attrID);
    }
  }
