#pragma pack(push, 1)
#include "zcl_include.h"
#pragma pack(pop)

#include "zigbee/consts.h"

_CODE_ZCL_ status_t zcl_temperature_measurement_register(u8 endpoint, u16 manuCode,
                                                         u8 attrNum,
                                                         const zclAttrInfo_t attrTbl[],
                                                         cluster_forAppCb_t cb) {
  return (zcl_registerCluster(endpoint, ZCL_CLUSTER_TEMPERATURE_MEASUREMENT,
                              manuCode, attrNum, attrTbl, NULL, cb));
}
