#include "iot/thing.h"
#include "application.h"

#include <esp_log.h>

#define TAG "AcControl"

namespace iot {

// 这里仅定义 AcControl 的属性和方法，不包含具体的实现
class AcControl : public Thing {

private:
        bool ac_power = false;

public:
    AcControl() : Thing("AcControl", "一个空调开关") {

        // 定义设备的属性
        properties_.AddBooleanProperty("ac_power", "空调是否打开", [this]() -> bool {
            return ac_power;
        });

        // 定义设备可以被远程执行的指令
        methods_.AddMethod("TurnOnAc", "打开空调", ParameterList(), [this](const ParameterList& parameters) {
            ac_power = true;
            ESP_LOGW(TAG, "已打开空调");
            Application::GetInstance().IotControlWebrtcStatus(true);
        });

        methods_.AddMethod("TurnOffAc", "关闭空调", ParameterList(), [this](const ParameterList& parameters) {
            ac_power = false;
            ESP_LOGW(TAG, "已关闭空调");
            Application::GetInstance().IotControlWebrtcStatus(false);
        });
    }
};

} // namespace iot

DECLARE_THING(AcControl);
