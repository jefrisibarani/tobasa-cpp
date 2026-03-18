#pragma once

#include <string>

namespace tbs {
namespace lis {

/** \ingroup LIS
 * LIS protocol
 */

// Message Protocols 
const std::string MSG_LIS2A             = "MSG_LIS2A";
const std::string MSG_HL7               = "MSG_HL7";
const std::string MSG_BCI               = "MSG_BCI";
const std::string MSG_DIRUI             = "MSG_DIRUI";

// LIS1A Devices
const std::string DEV_DEFAULT_LIS1A     = "Default_LIS1A";
const std::string DEV_TEST_LIS1A        = "DevTest_LIS1A";
const std::string DEV_INDIKO            = "Indiko";
const std::string DEV_DXH_500           = "DxH500";
const std::string DEV_GEM_3500          = "GEM3500";
const std::string DEV_SELECTRA          = "Selectra";

// HL7 Devices
const std::string DEV_DEFAULT_HL7       = "Default_HL7";
const std::string DEV_TEST_HL7          = "DevTest_HL7";

// BCI Devices
const std::string DEV_VIDAS             = "Vidas";
const std::string DEV_VITEK2_COMPACT    = "Vitek2Compact";

// DIRUI Devices
const std::string DEV_DIRUI_H_500       = "DiruiH-500";
const std::string DEV_DIRUI_BCC_3600    = "DiruiBcc3600";

} // namespace lis
} // namespace tbs