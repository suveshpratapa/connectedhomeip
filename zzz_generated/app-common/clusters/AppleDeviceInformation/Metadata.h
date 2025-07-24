// DO NOT EDIT MANUALLY - Generated file
//
// Cluster metadata information for cluster AppleDeviceInformation (cluster code: 323615744/0x1349FC00)
// based on src/controller/data_model/controller-clusters.matter
#pragma once

#include <app/data-model-provider/MetadataTypes.h>
#include <lib/core/DataModelTypes.h>

#include <cstdint>

#include <clusters/AppleDeviceInformation/Ids.h>

namespace chip {
namespace app {
namespace Clusters {
namespace AppleDeviceInformation {

inline constexpr uint32_t kRevision = 1;

namespace Attributes {
namespace SupportsWED {
inline constexpr DataModel::AttributeEntry kMetadataEntry(SupportsWED::Id, BitFlags<DataModel::AttributeQualityFlags>(),
                                                          Access::Privilege::kView, std::nullopt);
} // namespace SupportsWED

} // namespace Attributes

namespace Commands {} // namespace Commands
} // namespace AppleDeviceInformation
} // namespace Clusters
} // namespace app
} // namespace chip
