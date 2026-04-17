#pragma once

#include <string>
#include <vector>

bool HasRemoteFeaturesLoaded();
bool StoreRemoteFeaturesResponse(const std::string& responseText,
                                 std::string* failureReason = nullptr);
void ResetRemoteFeatures();
bool IsFeatureAllowedByRemoteState(int featureId);
std::vector<std::string> BuildVisibleFeatureList(const char* const* features, int featureCount);
