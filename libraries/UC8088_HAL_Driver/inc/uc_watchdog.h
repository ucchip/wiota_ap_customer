// Copyright 2017 ETH Zurich and University of Bologna.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 0.51 (the “License”); you may not use this file except in
// compliance with the License.  You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef __UC_WATCHDOG_H__
#define __UC_WATCHDOG_H__

#include <pulpino.h>

#define WDG_ENABLE_MASK         0x1
#define WDG_FEED_MASK           0x1

#define PARAM_WDG_STATE(NewState)   ((NewState==ENABLE)||(NewState==DISABLE))
#define PARAM_WDG(WDG)              (WDG==UC_WATCHDOG)

// pointer to mem of event unit - PointerEventunit
void WDG_FEED(WDG_TYPE *WDG);
void WDG_Cmd(WDG_TYPE *WDG, FunctionalState NewState);
void WDG_SetReload(WDG_TYPE* WDG, uint32_t value);

#endif
