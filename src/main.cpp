#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Maixduino_OV7740.h>
#include <Wire.h>

#ifdef M5STICKV
#define AXP192_ADDR 0x34
#define PIN_SDA 29
#define PIN_SCL 28
#endif

TFT_eSPI lcd;
//OV7740 supports YUV only
Maixduino_OV7740 camera(FRAMESIZE_QVGA, PIXFORMAT_YUV422);

bool axp192_init() {
    Serial.printf("AXP192 init.\n");
    sysctl_set_power_mode(SYSCTL_POWER_BANK3,SYSCTL_POWER_V33);

    Wire.begin((uint8_t) PIN_SDA, (uint8_t) PIN_SCL, 400000);
    Wire.beginTransmission(AXP192_ADDR);
    int err = Wire.endTransmission();
    if (err) {
        Serial.printf("Power management ic not found.\n");
        return false;
    }
    Serial.printf("AXP192 found.\n");

    // Clear the interrupts
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x46);
    Wire.write(0xFF);
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x23);
    Wire.write(0x08); //K210_VCore(DCDC2) set to 0.9V
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x33);
    Wire.write(0xC1); //190mA Charging Current
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x36);
    Wire.write(0x6C); //4s shutdown
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x91);
    Wire.write(0xF0); //LCD Backlight: GPIO0 3.3V
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x90);
    Wire.write(0x02); //GPIO LDO mode
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x28);
    Wire.write(0xF0); //VDD2.8V net: LDO2 3.3V,  VDD 1.5V net: LDO3 1.8V
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x27);
    Wire.write(0x2C); //VDD1.8V net:  DC-DC3 1.8V
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x12);
    Wire.write(0xFF); //open all power and EXTEN
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x23);
    Wire.write(0x08); //VDD 0.9v net: DC-DC2 0.9V
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x31);
    Wire.write(0x03); //Cutoff voltage 3.2V
    Wire.endTransmission();
    Wire.beginTransmission(AXP192_ADDR);
    Wire.write(0x39);
    Wire.write(0xFC); //Turnoff Temp Protect (Sensor not exist!)
    Wire.endTransmission();

    fpioa_set_function(23, (fpioa_function_t)(FUNC_GPIOHS0 + 26));
    gpiohs_set_drive_mode(26, GPIO_DM_OUTPUT);
    gpiohs_set_pin(26, GPIO_PV_HIGH); //Disable VBUS As Input, BAT->5V Boost->VBUS->Charing Cycle

    msleep(20);
    return true;
}

void setup() {
    Serial.begin(115200);
    axp192_init();

    /* LCD init */
    lcd.begin();
    lcd.setRotation(1);

    /* DVP init */
    Serial.printf("DVP init\n");
    if (!camera.begin()) {
        Serial.printf("camera init fail\n");
        while (true) {}
    } else {
        Serial.printf("camera init success\n");
    }
    camera.run(true);
}

void loop() {
    uint8_t *img = camera.snapshot();
    if (img == nullptr || img == 0) {
        Serial.printf("snap fail\n");
        return;
    }

    lcd.pushImage(0, 0, camera.width(), camera.height(), (uint16_t*)img);
}