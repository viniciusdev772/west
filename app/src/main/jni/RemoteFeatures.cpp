#include "RemoteFeatures.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <android/log.h>

namespace {

int ExtractFeatureId(const std::string& feature);
bool IsValidRemoteMenuEntry(const std::string& feature);

std::mutex gRemoteFeaturesMutex;
bool gRemoteFeaturesLoaded = false;
std::unordered_map<int, bool> gRemoteFeatures;
std::vector<std::string> gRemoteMenuEntries;

std::string ExtractJsonString(const std::string& json, const char* key) {
    if (!key || !*key) return "";
    const std::string token = std::string("\"") + key + "\":\"";
    const size_t start = json.find(token);
    if (start == std::string::npos) return "";

    std::string value;
    bool escape = false;
    size_t cursor = start + token.size();
    for (; cursor < json.size(); ++cursor) {
        const char ch = json[cursor];
        if (escape) {
            switch (ch) {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default: value.push_back(ch); break;
            }
            escape = false;
            continue;
        }

        if (ch == '\\') {
            escape = true;
            continue;
        }

        if (ch == '"') break;
        value.push_back(ch);
    }

    return value;
}

bool ParseRemoteMenuEntries(const std::string& response,
                            std::vector<std::string>* menuEntries,
                            std::unordered_map<int, bool>* enabledFeatures) {
    if (!menuEntries || !enabledFeatures) return false;

    const size_t menuItemsKey = response.find("\"menuItems\"");
    if (menuItemsKey != std::string::npos) {
        const size_t arrayStart = response.find('[', menuItemsKey);
        if (arrayStart != std::string::npos) {
            int depth = 0;
            size_t arrayEnd = std::string::npos;
            for (size_t i = arrayStart; i < response.size(); ++i) {
                if (response[i] == '[') ++depth;
                if (response[i] == ']') {
                    --depth;
                    if (depth == 0) {
                        arrayEnd = i;
                        break;
                    }
                }
            }

            if (arrayEnd != std::string::npos && arrayEnd > arrayStart) {
                std::vector<std::string> parsedEntries;
                std::unordered_map<int, bool> parsedFeatures;
                size_t cursor = arrayStart;

                while (cursor < arrayEnd) {
                    const size_t objectStart = response.find('{', cursor);
                    if (objectStart == std::string::npos || objectStart >= arrayEnd) break;
                    const size_t objectEnd = response.find('}', objectStart);
                    if (objectEnd == std::string::npos || objectEnd > arrayEnd) break;

                    const std::string itemJson = response.substr(objectStart, objectEnd - objectStart + 1);
                    const std::string entry = ExtractJsonString(itemJson, "entry");
                    const bool enabled = itemJson.find("\"enabled\":false") == std::string::npos;
                    if (!entry.empty()) {
                        if (!IsValidRemoteMenuEntry(entry)) {
                            cursor = objectEnd + 1;
                            continue;
                        }
                        if (enabled) {
                            parsedEntries.push_back(entry);
                        }
                        const int featureId = ExtractFeatureId(entry);
                        if (featureId > 0) parsedFeatures[featureId] = enabled;
                    }

                    cursor = objectEnd + 1;
                }

                if (!parsedFeatures.empty() || !parsedEntries.empty()) {
                    *menuEntries = parsedEntries;
                    *enabledFeatures = parsedFeatures;
                    return true;
                }
            }
        }
    }

    const size_t menuKey = response.find("\"menuEntries\"");
    if (menuKey == std::string::npos) return false;

    const size_t arrayStart = response.find('[', menuKey);
    if (arrayStart == std::string::npos) return false;

    int depth = 0;
    size_t arrayEnd = std::string::npos;
    for (size_t i = arrayStart; i < response.size(); ++i) {
        if (response[i] == '[') ++depth;
        if (response[i] == ']') {
            --depth;
            if (depth == 0) {
                arrayEnd = i;
                break;
            }
        }
    }
    if (arrayEnd == std::string::npos || arrayEnd <= arrayStart) return false;

    std::vector<std::string> parsedEntries;
    std::unordered_map<int, bool> parsedFeatures;

    size_t cursor = arrayStart + 1;
    while (cursor < arrayEnd) {
        const size_t valueStart = response.find('"', cursor);
        if (valueStart == std::string::npos || valueStart >= arrayEnd) break;

        std::string value;
        bool escape = false;
        size_t valueEnd = valueStart + 1;
        for (; valueEnd < arrayEnd; ++valueEnd) {
            const char ch = response[valueEnd];
            if (escape) {
                switch (ch) {
                    case '"': value.push_back('"'); break;
                    case '\\': value.push_back('\\'); break;
                    case '/': value.push_back('/'); break;
                    case 'b': value.push_back('\b'); break;
                    case 'f': value.push_back('\f'); break;
                    case 'n': value.push_back('\n'); break;
                    case 'r': value.push_back('\r'); break;
                    case 't': value.push_back('\t'); break;
                    default: value.push_back(ch); break;
                }
                escape = false;
                continue;
            }

            if (ch == '\\') {
                escape = true;
                continue;
            }

            if (ch == '"') break;
            value.push_back(ch);
        }

        if (!value.empty()) {
            if (!IsValidRemoteMenuEntry(value)) {
                cursor = valueEnd + 1;
                continue;
            }
            parsedEntries.push_back(value);
            const int featureId = ExtractFeatureId(value);
            if (featureId > 0) parsedFeatures[featureId] = true;
        }

        cursor = valueEnd + 1;
    }

    if (parsedEntries.empty()) return false;
    *menuEntries = parsedEntries;
    *enabledFeatures = parsedFeatures;
    return true;
}

int ExtractFeatureId(const std::string& feature) {
    const size_t collapseAdd = feature.find("CollapseAdd_");
    if (collapseAdd == std::string::npos) return -1;

    size_t cursor = collapseAdd + std::strlen("CollapseAdd_");
    size_t end = cursor;
    while (end < feature.size() && std::isdigit(static_cast<unsigned char>(feature[end]))) {
        ++end;
    }

    if (end == cursor) return -1;
    return std::atoi(feature.substr(cursor, end - cursor).c_str());
}

bool IsStructuralMenuEntry(const std::string& feature) {
    return feature.rfind("Category_", 0) == 0 ||
           feature.rfind("Collapse_", 0) == 0 ||
           feature.rfind("CollapseAdd_Category_", 0) == 0;
}

bool IsValidRemoteMenuEntry(const std::string& feature) {
    if (feature.empty()) return false;
    if (IsStructuralMenuEntry(feature)) return true;

    const int featureId = ExtractFeatureId(feature);
    if (featureId > 0) return true;

    __android_log_print(ANDROID_LOG_WARN, "MOD_REMOTE_FEATURES",
                        "Entrada remota ignorada por nao possuir feat id explicito: %s",
                        feature.c_str());
    return false;
}

}  // namespace

bool StoreRemoteFeaturesResponse(const std::string& responseText,
                                 std::string* failureReason) {
    if (failureReason) failureReason->clear();
    std::vector<std::string> parsedEntries;
    std::unordered_map<int, bool> parsedFeatures;
    if (!ParseRemoteMenuEntries(responseText, &parsedEntries, &parsedFeatures)) {
        __android_log_print(ANDROID_LOG_WARN, "MOD_REMOTE_FEATURES",
                            "Falha ao parsear menu remoto body=%s",
                            responseText.empty() ? "<empty>" : responseText.c_str());
        if (failureReason) *failureReason = "Resposta invalida ao carregar recursos remotos.";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(gRemoteFeaturesMutex);
        gRemoteMenuEntries = parsedEntries;
        gRemoteFeatures = parsedFeatures;
        gRemoteFeaturesLoaded = true;
    }

    __android_log_print(ANDROID_LOG_INFO, "MOD_REMOTE_FEATURES",
                        "Menu remoto atualizado: %zu entradas", parsedEntries.size());
    return true;
}

bool HasRemoteFeaturesLoaded() {
    std::lock_guard<std::mutex> lock(gRemoteFeaturesMutex);
    return gRemoteFeaturesLoaded && !gRemoteMenuEntries.empty();
}

void ResetRemoteFeatures() {
    std::lock_guard<std::mutex> lock(gRemoteFeaturesMutex);
    gRemoteMenuEntries.clear();
    gRemoteFeatures.clear();
    gRemoteFeaturesLoaded = false;
}

bool IsFeatureAllowedByRemoteState(int featureId) {
    std::lock_guard<std::mutex> lock(gRemoteFeaturesMutex);
    if (!gRemoteFeaturesLoaded) return true;
    const auto it = gRemoteFeatures.find(featureId);
    if (it == gRemoteFeatures.end()) return false;
    return it->second;
}

std::vector<std::string> BuildVisibleFeatureList(const char* const* features, int featureCount) {
    std::vector<std::string> visibleFeatures;
    if (!features || featureCount <= 0) return visibleFeatures;
    {
        std::lock_guard<std::mutex> lock(gRemoteFeaturesMutex);
        if (gRemoteFeaturesLoaded && !gRemoteMenuEntries.empty()) {
            return gRemoteMenuEntries;
        }
    }

    visibleFeatures.reserve(static_cast<size_t>(featureCount));
    for (int i = 0; i < featureCount; ++i) {
        const std::string feature = features[i] ? features[i] : "";
        if (feature.empty()) continue;
        visibleFeatures.push_back(feature);
    }

    return visibleFeatures;
}
