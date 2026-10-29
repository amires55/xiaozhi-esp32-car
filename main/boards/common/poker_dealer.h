#ifndef POKER_DEALER_H
#define POKER_DEALER_H

#include <algorithm>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Hardware for Freenove ESP32-S3 Display 2.8 LCD:
// L298N IN1 -> GPIO2; IN2 -> GPIO3; ENA jumper enabled.
// SG90 signal -> GPIO14. GPIO10 is LCD CS and must not be used.
class PokerDealer {
public:
    static constexpr gpio_num_t MOTOR_IN1 = GPIO_NUM_2;
    static constexpr gpio_num_t MOTOR_IN2 = GPIO_NUM_3;
    static constexpr gpio_num_t SERVO_GPIO = GPIO_NUM_14;

    // Starting values; calibrate on the real mechanism.
    static constexpr uint32_t CARD_PULSE_MS = 500;
    static constexpr int SERVO_MIN_DEG = 20;
    static constexpr int SERVO_MAX_DEG = 160;
    static constexpr int COMMUNITY_DEG = 90;

    bool Init() {
        gpio_config_t motor_cfg{};
        motor_cfg.pin_bit_mask = (1ULL << MOTOR_IN1) | (1ULL << MOTOR_IN2);
        motor_cfg.mode = GPIO_MODE_OUTPUT;
        motor_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        motor_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        motor_cfg.intr_type = GPIO_INTR_DISABLE;
        if (gpio_config(&motor_cfg) != ESP_OK) return false;
        StopMotor();

        ledc_timer_config_t timer{};
        timer.speed_mode = LEDC_LOW_SPEED_MODE;
        timer.duty_resolution = LEDC_TIMER_16_BIT;
        timer.timer_num = LEDC_TIMER_1;
        timer.freq_hz = 50;
        timer.clk_cfg = LEDC_AUTO_CLK;
        if (ledc_timer_config(&timer) != ESP_OK) return false;

        ledc_channel_config_t channel{};
        channel.gpio_num = SERVO_GPIO;
        channel.speed_mode = LEDC_LOW_SPEED_MODE;
        channel.channel = LEDC_CHANNEL_2;
        channel.intr_type = LEDC_INTR_DISABLE;
        channel.timer_sel = LEDC_TIMER_1;
        channel.duty = 0;
        channel.hpoint = 0;
        if (ledc_channel_config(&channel) != ESP_OK) return false;

        SetServoAngle(COMMUNITY_DEG);
        vTaskDelay(pdMS_TO_TICKS(400));
        ESP_LOGI(TAG, "Poker dealer initialized");
        return true;
    }

    void DealHoleCards(int players) {
        players = std::clamp(players, 6, 9);
        // Two rounds; one card to each player per round.
        for (int round = 0; round < 2; ++round) {
            for (int player = 0; player < players; ++player) {
                SetPlayerPosition(player, players);
                vTaskDelay(pdMS_TO_TICKS(250));
                DispenseOneCard();
                vTaskDelay(pdMS_TO_TICKS(250));
            }
        }
        SetServoAngle(COMMUNITY_DEG);
    }

    void DealFlop() {
        BurnOneCard();
        SetServoAngle(COMMUNITY_DEG);
        vTaskDelay(pdMS_TO_TICKS(250));
        for (int i = 0; i < 3; ++i) {
            DispenseOneCard();
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }

    void DealTurn() {
        BurnOneCard();
        SetServoAngle(COMMUNITY_DEG);
        vTaskDelay(pdMS_TO_TICKS(250));
        DispenseOneCard();
    }

    void DealRiver() {
        BurnOneCard();
        SetServoAngle(COMMUNITY_DEG);
        vTaskDelay(pdMS_TO_TICKS(250));
        DispenseOneCard();
    }

    void TestOneCard() { DispenseOneCard(); }

private:
    static constexpr const char* TAG = "PokerDealer";

    void SetServoAngle(int degrees) {
        degrees = std::clamp(degrees, 0, 180);
        // Typical SG90 pulse range: 500-2500 us in a 20 ms period.
        const uint32_t pulse_us = 500 + (static_cast<uint32_t>(degrees) * 2000) / 180;
        const uint32_t duty = (pulse_us * 65535UL) / 20000UL;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    }

    void SetPlayerPosition(int player, int players) {
        const int span = SERVO_MAX_DEG - SERVO_MIN_DEG;
        const int angle = SERVO_MIN_DEG + (player * span) / (players - 1);
        SetServoAngle(angle);
    }

    void RunMotor(uint32_t ms) {
        gpio_set_level(MOTOR_IN1, 1);
        gpio_set_level(MOTOR_IN2, 0);
        vTaskDelay(pdMS_TO_TICKS(ms));
        StopMotor();
    }

    void StopMotor() {
        gpio_set_level(MOTOR_IN1, 0);
        gpio_set_level(MOTOR_IN2, 0);
    }

    void DispenseOneCard() { RunMotor(CARD_PULSE_MS); }

    void BurnOneCard() {
        DispenseOneCard(); // The fed card is discarded.
        vTaskDelay(pdMS_TO_TICKS(250));
    }
};

#endif
