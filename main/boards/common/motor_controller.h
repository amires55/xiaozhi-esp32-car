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
        gpio_config_t c = { .pin_bit_mask = (1ULL << pin), .mode = GPIO_MODE_OUTPUT };
        gpio_config(&c);
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
        if (s <= 0) { Stop(); return; }
        int d = s * 255 / 100; L(d,1,0); R(d,1,0); Note("▶前进");
    }
    void Backward(int s) {
        if (s <= 0) { Stop(); return; }
        int d = s * 255 / 100; L(d,0,1); R(d,0,1); Note("◀后退");
    }
    void TurnLeft(int s) {
        if (s <= 0) { Stop(); return; }
        int d = s * 255 / 100; L(d,0,1); R(d,1,0); Note("↺左转");
    }
    void TurnRight(int s) {
        if (s <= 0) { Stop(); return; }
        int d = s * 255 / 100; L(d,1,0); R(d,0,1); Note("↻右转");
    }
    void Stop() { L(0,0,0); R(0,0,0); Note("■停止"); }
};

extern MotorController* g_motor;

inline void MotorControl(const std::string& action, int speed) {
    if (!g_motor) return;
    if (action == "forward") g_motor->Forward(speed);
    else if (action == "backward") g_motor->Backward(speed);
    else if (action == "turn_left") g_motor->TurnLeft(speed);
    else if (action == "turn_right") g_motor->TurnRight(speed);
    else if (action == "stop") g_motor->Stop();
    else ESP_LOGW(TAG, "Unknown motor action: %s", action.c_str());
}

#endif
