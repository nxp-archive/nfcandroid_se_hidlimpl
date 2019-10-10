/******************************************************************************
 *
 *  Copyright (C) 2018-2019 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include "NxpEse.h"
#include "phNxpEse_Api.h"
#include <android-base/stringprintf.h>
#include <android-base/logging.h>
#include "eSEClient.h"

namespace vendor {
namespace nxp {
namespace nxpese {
namespace V1_0 {
namespace implementation {
using android::base::StringPrintf;
//using android::hardware::secure_element::V1_0::implementation::SecureElement;
static android::sp<ISecureElementHalCallback> seCallback;
static android::sp<ISecureElementHalCallback> virtualISOCallback;
// Methods from ::vendor::nxp::nxpese::V1_0::INxpEse follow.
  Return<void> NxpEse::setSeCallBack(const android::sp<ISecureElementHalCallback>& clientCallback) {
    seCallback = clientCallback;
    return Void();
  }
  Return<void> NxpEse::setVirtualISOCallBack(const android::sp<ISecureElementHalCallback>& clientCallback){
    virtualISOCallback = clientCallback;
    return Void();
  }
  void NxpEse::initSEService() {
    ESESTATUS status = ESESTATUS_SUCCESS;
    ESESTATUS deInitStatus = ESESTATUS_SUCCESS;
    phNxpEse_initParams initParams;
    memset(&initParams, 0x00, sizeof(phNxpEse_initParams));
    initParams.initMode = ESE_MODE_NORMAL;
    initParams.mediaType = ESE_PROTOCOL_MEDIA_SPI_APDU_GATE;

    if(!seCallback)
      return;

    status = phNxpEse_open(initParams);
    if (status != ESESTATUS_SUCCESS) {
      goto exit;
    }

    status = phNxpEse_SetEndPoint_Cntxt(0);
    if (status != ESESTATUS_SUCCESS) {
        goto exit1;
    }
    status = phNxpEse_init(initParams);
    if (status != ESESTATUS_SUCCESS) {
      goto exit1;
    }
    status = phNxpEse_ResetEndPoint_Cntxt(0);
    if (status != ESESTATUS_SUCCESS) {
      goto exit2;
    }

    LOG(INFO) << "ESE SPI init complete !!!";
exit2:
    deInitStatus = phNxpEse_deInit();
exit1:
    status = phNxpEse_close(deInitStatus);
    exit:
    if (status == ESESTATUS_SUCCESS)
    {
      seCallback->onStateChange(true);
    }
    else
    {
      LOG(ERROR) << "eSE-Hal Init failed";
      seCallback->onStateChange(false);
    }
  }

  void NxpEse::initVIrtualISOService() {
    ESESTATUS status = ESESTATUS_SUCCESS;
    ESESTATUS deInitStatus = ESESTATUS_SUCCESS;
    phNxpEse_initParams initParams;
    memset(&initParams, 0x00, sizeof(phNxpEse_initParams));
    initParams.initMode = ESE_MODE_NORMAL;
    initParams.mediaType = ESE_PROTOCOL_MEDIA_SPI_APDU_GATE;

    if(!virtualISOCallback)
      return;

    status = phNxpEse_SetEndPoint_Cntxt(1);
    if (status != ESESTATUS_SUCCESS) {
      goto exit1;
    }
    status = phNxpEse_init(initParams);
    if (status != ESESTATUS_SUCCESS) {
      goto exit1;
    }
    status = phNxpEse_ResetEndPoint_Cntxt(1);
    if (status != ESESTATUS_SUCCESS) {
      goto exit2;
    }

    LOG(INFO) << "ESE SPI init complete !!!";
exit2:
    deInitStatus = phNxpEse_deInit();
exit1:
    status = phNxpEse_close(deInitStatus);

    if (status == ESESTATUS_SUCCESS)
    {
      virtualISOCallback->onStateChange(true);
    }
    else
    {
      LOG(ERROR) << "eSE-Hal Init failed";
      virtualISOCallback->onStateChange(false);
    }
  }
Return<void> NxpEse::ioctlHandler(uint64_t ioctlType,
                           ese_nxp_IoctlInOutData_t inpOutData) {
  switch(ioctlType)
  {
    case HAL_ESE_IOCTL_NFC_JCOP_DWNLD:
    {
      //nfc_nci_IoctlInOutData_t* inpOutData = (nfc_nci_IoctlInOutData_t*)inpOutData;
      int update_state = inpOutData.inp.data.nxpCmd.p_cmd[0];
      if(update_state == ESE_JCOP_UPDATE_COMPLETED ||
        update_state == ESE_LS_UPDATE_COMPLETED) {
        seteSEClientState(update_state);
        eSEClientUpdate_SE_Thread();
      }
    }
    break;
  }
    return Void();
}

Return<void> NxpEse::ioctl(uint64_t ioctlType,
                           const hidl_vec<uint8_t>& inOutData,
                           ioctl_cb _hidl_cb) {
  ese_nxp_IoctlInOutData_t inpOutData;
  ese_nxp_IoctlInOutData_t* pInOutData =
      (ese_nxp_IoctlInOutData_t*)&inOutData[0];

  /*data from proxy->stub is copied to local data which can be updated by
   * underlying HAL implementation since its an inout argument*/
  memcpy(&inpOutData, pInOutData, sizeof(ese_nxp_IoctlInOutData_t));
  ESESTATUS status = phNxpEse_spiIoctl(ioctlType, &inpOutData);
  ioctlHandler(ioctlType, inpOutData);
  /*copy data and additional fields indicating status of ioctl operation
   * and context of the caller. Then invoke the corresponding proxy callback*/
  inpOutData.out.ioctlType = ioctlType;
  inpOutData.out.context = pInOutData->inp.context;
  inpOutData.out.result = status;
  if(ioctlType == HAL_ESE_IOCTL_GET_ESE_UPDATE_STATE)
  {
    inpOutData.out.data.status = (getJcopUpdateRequired() | (getLsUpdateRequired() << 8));
  }
  EseData outputData;
  outputData.setToExternal((uint8_t*)&inpOutData.out,
                           sizeof(ese_nxp_ExtnOutputData_t));
  LOG(ERROR) << "GET ESE update state2 = "<< inpOutData.out.data.status;
  _hidl_cb(outputData);
  return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

}  // namespace implementation
}  // namespace V1_0
}  // namespace nxpese
}  // namespace nxp
}  // namespace vendor
