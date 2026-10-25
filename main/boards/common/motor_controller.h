#ifndef __MOTOR_CONTROLLER_H__
#define __MOTOR_CONTROLLER_H__

#include "mcp_server.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "display.h"
#include "board.h"

#define TAG "Motor"

class MotorController {
private:
    gpio_num_t ena_pin_, in1_pin_, in2_pin_;
    gpio_num_t enb_pin_, in3_pin_, in4_pin_;

    void InitPwm(gpio_num_t pin, ledc_channel_t ch) {
        if (pin == GPIO_NUM_NC) return;
        ledc_timer_config_t timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .timer_num = LEDC_TIMER_0,
            .freq_hz = 5000,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        ledc_timer_config(&timer);
        ledc_channel_config_t ch_cfg = {
            .gpio_num = pin,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = ch,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0,
        };
        ledc_channel_config(&ch_cfg);
    }

void InitOut(gpio_num_t pin) {
    if (pin == GPIO_NUM_NC) return;

    gpio_config_t c = {};
    c.pin_bit_mask = (1ULL << pin);
    c.mode = GPIO_MODE_OUTPUT;
    c.pull_up_en = GPIO_PULLUP_DISABLE;
    c.pull_down_en = GPIO_PULLDOWN_DISABLE;
    c.intr_type = GPIO_INTR_DISABLE;

    esp_err_t err = gpio_config(&c);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO %d initialization failed: %s",
                 (int)pin, esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "GPIO %d initialized as output", (int)pin);
    }
}

    void L(int duty, int in1, int in2) {
        if (in1_pin_ != GPIO_NUM_NC) { gpio_set_level(in1_pin_, in1); gpio_set_level(in2_pin_, in2); }
        if (ena_pin_ != GPIO_NUM_NC) { ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty); ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); }
    }

    void R(int duty, int in3, int in4) {
        if (in3_pin_ != GPIO_NUM_NC) { gpio_set_level(in3_pin_, in3); gpio_set_level(in4_pin_, in4); }
        if (enb_pin_ != GPIO_NUM_NC) { ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty); ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1); }
    }

    void Note(const char* s) {
        auto d = Board::GetInstance().GetDisplay();
        if (d) d->ShowNotification(s, 2000);
        ESP_LOGI(TAG, "%s", s);
    }

public:
    MotorController(gpio_num_t ena, gpio_num_t in1, gpio_num_t in2,
                    gpio_num_t enb, gpio_num_t in3, gpio_num_t in4)
        : ena_pin_(ena), in1_pin_(in1), in2_pin_(in2),
          enb_pin_(enb), in3_pin_(in3), in4_pin_(in4) {
        InitOut(in1); InitOut(in2); InitOut(in3); InitOut(in4);
        InitPwm(ena, LEDC_CHANNEL_0);
        InitPwm(enb, LEDC_CHANNEL_1);
        if (in1 != GPIO_NUM_NC) gpio_set_level(in1, 0);
if (in2 != GPIO_NUM_NC) gpio_set_level(in2, 0);
if (in3 != GPIO_NUM_NC) gpio_set_level(in3, 0);
if (in4 != GPIO_NUM_NC) gpio_set_level(in4, 0);
        ESP_LOGI(TAG, "OK");
    }

void Forward(int s) {
    gpio_set_level(in1_pin_, 1);
    gpio_set_level(in2_pin_, 0);
    gpio_set_level(in3_pin_, 1);
    gpio_set_level(in4_pin_, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Note("FORWARD TEST");
}
    void Backward(int s) {
        int d = 255; L(d,0,1); R(d,0,1); Note("◀后退");
    }
    void TurnLeft(int s) {
        int d = 255; L(d,0,1); R(d,1,0); Note("↺左转");
    }
    void TurnRight(int s) {
        int d = 255; L(d,1,0); R(d,0,1); Note("↻右转");
    }
    void Stop() { L(0,0,0); R(0,0,0); Note("■停止"); }
};

extern MotorController* g_motor;
inline void MotorControl(const std::string& action, int speed) {
    if (!g_motor) {
        ESP_LOGE(TAG, "Motor controller is NULL");
        return;
    }

    ESP_LOGI(TAG, "Motor command: action='%s', speed=%d",
             action.c_str(), speed);

    std::string cmd = action;

    // Remove leading and trailing spaces
    size_t first = cmd.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        ESP_LOGW(TAG, "Empty motor action");
        return;
    }

    size_t last = cmd.find_last_not_of(" \t\r\n");
    cmd = cmd.substr(first, last - first + 1);

    // Convert to lowercase
    for (char& c : cmd) {
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
    }

    if (cmd == "forward") {
        g_motor->Forward(speed);
    } else if (cmd == "backward") {
        g_motor->Backward(speed);
    } else if (cmd == "turn_left") {
        g_motor->TurnLeft(speed);
    } else if (cmd == "turn_right") {
        g_motor->TurnRight(speed);
    } else if (cmd == "stop") {
        g_motor->Stop();
    } else {
        ESP_LOGW(TAG, "Unknown motor action: '%s'", cmd.c_str());
    }
}

#endif
