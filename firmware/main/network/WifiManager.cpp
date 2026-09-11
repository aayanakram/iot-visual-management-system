#include "WifiManager.hpp"

#include <cstring>

#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "WifiManager";

WifiManager::WifiManager(
    const char* ssid,
    const char* password,
    EventQueue& eventQueue
)
    : ssid_(ssid),
      password_(password),
      eventQueue_(eventQueue)
{
}

WifiManager::~WifiManager()
{
    // unregister event handlers if they were created successfully
    if (wifiEventHandler_ != nullptr)
    {
        esp_event_handler_instance_unregister(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifiEventHandler_
        );
    }

    if (ipEventHandler_ != nullptr)
    {
        esp_event_handler_instance_unregister(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            ipEventHandler_
        );
    }

    // stop Wi-Fi only if initialization completed
    if (initialized_)
    {
        esp_wifi_stop();
        esp_wifi_deinit();
    }

    if (netif_ != nullptr)
    {
        esp_netif_destroy(netif_);
        netif_ = nullptr;
    }
}

bool WifiManager::initialize()
{
    if (initialized_)
    {
        return true;
    }

    // initialize the TCP/IP stack used by ESP-IDF networking
    if (esp_netif_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize network interface");
        return false;
    }

    // create the default event loop
    // ESP_ERR_INVALID_STATE means one already exists, which is acceptable
    const esp_err_t eventLoopResult = esp_event_loop_create_default();

    if (eventLoopResult != ESP_OK &&
        eventLoopResult != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Failed to create default event loop");
        return false;
    }

    // create the default Wi-Fi station network interface
    netif_ = esp_netif_create_default_wifi_sta();

    if (netif_ == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create Wi-Fi station interface");
        return false;
    }

    wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();

    if (esp_wifi_init(&wifiInitConfig) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize Wi-Fi driver");
        return false;
    }

    // register callbacks for both Wi-Fi and IP events
    if (esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &WifiManager::eventHandler,
            this,
            &wifiEventHandler_) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register Wi-Fi event handler");
        return false;
    }

    if (esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &WifiManager::eventHandler,
            this,
            &ipEventHandler_) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to register IP event handler");
        return false;
    }

    wifi_config_t wifiConfig = {};

    // copy credentials into ESP-IDF's fixed-size configuration buffers
    std::strncpy(
        reinterpret_cast<char*>(wifiConfig.sta.ssid),
        ssid_,
        sizeof(wifiConfig.sta.ssid) - 1
    );

    std::strncpy(
        reinterpret_cast<char*>(wifiConfig.sta.password),
        password_,
        sizeof(wifiConfig.sta.password) - 1
    );

    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set Wi-Fi station mode");
        return false;
    }

    if (esp_wifi_set_config(WIFI_IF_STA, &wifiConfig) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure Wi-Fi station");
        return false;
    }

    if (esp_wifi_start() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start Wi-Fi");
        return false;
    }

    initialized_ = true;

    ESP_LOGI(TAG, "Wi-Fi initialized");
    return true;
}

bool WifiManager::connect()
{
    if (!initialized_)
    {
        return false;
    }

    reconnectEnabled_ = true;
    const esp_err_t result = esp_wifi_connect();

    if (result != ESP_OK)
    {
        ESP_LOGW(TAG, "Wi-Fi connection attempt failed");
        return false;
    }

    ESP_LOGI(TAG, "Wi-Fi connection requested");
    return true;
}

void WifiManager::disconnect()
{
    if (!initialized_)
    {
        return;
    }

    reconnectEnabled_ = false;
    esp_wifi_disconnect();
}

bool WifiManager::isConnected() const
{
    return connected_;
}

void WifiManager::eventHandler(
    void* handlerArg,
    esp_event_base_t eventBase,
    std::int32_t eventId,
    void* eventData
)
{
    auto* manager = static_cast<WifiManager*>(handlerArg);

    if (manager != nullptr)
    {
        manager->handleWifiEvent(
            eventBase,
            eventId,
            eventData
        );
    }
}

void WifiManager::handleWifiEvent(
    esp_event_base_t eventBase,
    std::int32_t eventId,
    void* eventData
)
{
    (void)eventData;

    if (eventBase == WIFI_EVENT)
    {
        if (eventId == WIFI_EVENT_STA_START)
        {
            ESP_LOGI(TAG, "Wi-Fi station started");
            esp_wifi_connect();
        }
        else if (eventId == WIFI_EVENT_STA_DISCONNECTED)
        {
            connected_ = false;

            ESP_LOGW(TAG, "Wi-Fi disconnected");

            publishConnectionEvent(EventType::WifiDisconnected);

            // Retry ordinary connection loss; respect an explicit disconnect.
            if (reconnectEnabled_ && esp_wifi_connect() != ESP_OK)
            {
                ESP_LOGW(TAG, "Wi-Fi reconnect request failed");
            }
        }
    }
    else if (eventBase == IP_EVENT &&
             eventId == IP_EVENT_STA_GOT_IP)
    {
        connected_ = true;

        ESP_LOGI(TAG, "Wi-Fi connected and IP address acquired");

        publishConnectionEvent(EventType::WifiConnected);
    }
}

void WifiManager::publishConnectionEvent(EventType type)
{
    Event event;

    event.type = type;
    event.sourceId = 0;
    event.value = connected_ ? 1 : 0;
    event.timestampMs =
        static_cast<std::uint64_t>(esp_timer_get_time() / 1000);
    event.sequenceId = 0;

    eventQueue_.send(event);
}
