#pragma once
template<typename... Args> void hostLog(const char*, const char*, Args...) {}
#define ESP_LOGI(...) hostLog(__VA_ARGS__)
#define ESP_LOGW(...) hostLog(__VA_ARGS__)
#define ESP_LOGE(...) hostLog(__VA_ARGS__)
