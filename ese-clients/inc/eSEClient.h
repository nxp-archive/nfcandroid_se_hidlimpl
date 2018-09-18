/*******************************************************************************
 *
 *  Copyright 2018 NXP
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

#include <android/hardware/secure_element/1.0/ISecureElementHalCallback.h>
#include "phNxpEse_Api.h"
#include "../../../secure_element_extns/inc/eSEClientIntf.h"

#ifndef ESE_UPDATE_2_H_
#define ESE_UPDATE_2_H_

extern ese_update_state_t ese_update;
using ::android::hardware::secure_element::V1_0::ISecureElementHalCallback;


void checkEseClientUpdate();

SESTATUS perform_eSEClientUpdate();

void eSEClientUpdate_SE_Thread();

void seteSEClientState(uint8_t state);
typedef enum {
ESE = 0,
EUICC = 1,
}SEDomainID;
#endif /* ESE_UPDATE_2_H_ */
