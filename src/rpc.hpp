#pragma once

#include <rpc.h>
#include <optional>
#include <string>
#include <variant>
#include <unordered_set>
#include <vector>

#include "include/cef_v8.h"
#include "include/cef_values.h"

// UUID
inline UUID CreateUuid() {
  UUID uuid;
  RPC_STATUS resultCode = UuidCreate(&uuid);
  if (resultCode == RPC_S_OK) {
    return uuid;
  } else {
    throw std::runtime_error("UuidCreate() failed with error code: " +
                             std::to_string(resultCode));
  }
}

inline std::string uuid_to_string(const UUID& m) {
  RPC_CSTR str;
  RPC_STATUS resultCode = UuidToStringA(const_cast<UUID*>(&m), &str);
  if (resultCode == RPC_S_OK) {
    std::string result = std::string(reinterpret_cast<char*>(str));
    RpcStringFreeA(&str);
    return result;
  } else {
    throw std::runtime_error("UuidToStringA() failed with error code: " +
                             std::to_string(resultCode));
  }
}

inline CefRefPtr<CefValue> CefV8ValueToCefValueInternal(
    CefRefPtr<CefV8Value> v8Value,
    std::vector<CefRefPtr<CefV8Value>>& visitedStack,
    int depth = 0) {
  auto result = CefValue::Create();
  if (!v8Value || v8Value->IsNull() || v8Value->IsUndefined()) {
    result->SetNull();
    return result;
  }

  if (v8Value->IsBool()) {
    result->SetBool(v8Value->GetBoolValue());
    return result;
  }

  if (v8Value->IsInt()) {
    result->SetInt(v8Value->GetIntValue());
    return result;
  }

  if (v8Value->IsUInt()) {
    result->SetInt(static_cast<int>(v8Value->GetUIntValue()));
    return result;
  }

  if (v8Value->IsDouble()) {
    result->SetDouble(v8Value->GetDoubleValue());
    return result;
  }

  if (v8Value->IsString()) {
    result->SetString(v8Value->GetStringValue());
    return result;
  }

  if (v8Value->IsDate()) {
    result->SetDouble(v8Value->GetDateValue().val);
    return result;
  }

  // Guard against deep recursion stack overflows
  if (depth > 64) {
    result->SetNull();
    return result;
  }

  // Handle Arrays & Objects (Reference Types)
  if (v8Value->IsArray() || v8Value->IsObject()) {
    // Check if this exact V8 object is already an ancestor on the current
    // traversal path
    for (const auto& ancestor : visitedStack) {
      if (v8Value->IsSame(ancestor)) {
        // Cycle detected! Break the recursion with null.
        result->SetNull();
        return result;
      }
    }

    // Push to current ancestry stack
    visitedStack.push_back(v8Value);

    if (v8Value->IsArray()) {
      auto list = CefListValue::Create();
      int len = v8Value->GetArrayLength();
      for (int i = 0; i < len; ++i) {
        CefRefPtr<CefV8Value> elem = v8Value->GetValue(i);
        list->SetValue(
            i, CefV8ValueToCefValueInternal(elem, visitedStack, depth + 1));
      }
      result->SetList(list);
    } else {
      auto dict = CefDictionaryValue::Create();
      std::vector<CefString> keys;
      v8Value->GetKeys(keys);
      for (const auto& key : keys) {
        CefRefPtr<CefV8Value> val = v8Value->GetValue(key);
        if (val && !val->IsFunction()) {
          dict->SetValue(
              key, CefV8ValueToCefValueInternal(val, visitedStack, depth + 1));
        }
      }
      result->SetDictionary(dict);
    }

    // Pop when unrolling call stack
    visitedStack.pop_back();
    return result;
  }

  result->SetNull();
  return result;
}

inline CefRefPtr<CefValue> CefV8ValueToCefValue(CefRefPtr<CefV8Value> v8Value) {
  std::vector<CefRefPtr<CefV8Value>> visitedStack;
  return CefV8ValueToCefValueInternal(v8Value, visitedStack, 0);
}