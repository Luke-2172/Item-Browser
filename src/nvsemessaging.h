#pragma once
#include <cstdint>
// ABI prefixes from xNVSE's public PluginAPI.h. Do not access later fields.
struct NVSELoadInterface {
 uint32_t nvseVersion,runtimeVersion,editorVersion,isEditor;
 bool (*RegisterCommand)(void*);
 void (*SetOpcodeBase)(uint32_t);
 void* (*QueryInterface)(uint32_t);
 uint32_t (*GetPluginHandle)();
};
struct BrowserNVSEMessage {const char* sender;uint32_t type,dataLen;void* data;};
struct BrowserNVSEMessaging {
 uint32_t version;
 bool (*RegisterListener)(uint32_t,const char*,void (*)(BrowserNVSEMessage*));
};
constexpr uint32_t BrowserMessagingInterface=2;
constexpr uint32_t BrowserMainGameLoop=20;
