/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "ImageData.h"
// #include <cstring>
// #include <iostream>
// using namespace std;

/**
/* To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 * You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 */
#include <demos/lv_demos.h>
#include <examples/lv_examples.h>

using namespace esp_panel::drivers;
using namespace esp_panel::board;

uint8_t palette[] = {
    0xff, 0xff, 0xff, 0xff, /*Color of index 0*/
    0x00, 0x00, 0x00, 0xff, /*Color of index 1*/
};

uint8_t reverse_palette[] = {
    0x00, 0x00, 0x00, 0xff, /*Color of index 0*/
    0xff, 0xff, 0xff, 0xff, /*Color of index 1*/
};

uint8_t* flipY_1bpp(const uint8_t* pixel_data, size_t width, size_t height)
{
    // 计算每行占用的字节数（不考虑额外对齐）
    const size_t rowBytes = (width + 7) / 8;
    // 总字节数
    const size_t totalBytes = rowBytes * height;

    // 分配新的缓冲区来存放翻转后的图像
    uint8_t* flipped = new uint8_t[totalBytes];

    // 对每一行进行翻转拷贝
    for (size_t y = 0; y < height; ++y)
    {
        // 源图第 y 行的起始地址
        const uint8_t* src = pixel_data + y * rowBytes;
        // 目标图翻转后应对应的行地址: (height - 1 - y)
        uint8_t* dst = flipped + (height - 1 - y) * rowBytes;

        // 拷贝一整行（这里一行 rowBytes 字节）
        memcpy(dst, src, rowBytes);
    }

    return flipped;
}

/* 运行时创建图像数据 */
lv_img_dsc_t* create_indexed_1bit_image(const uint8_t *palette,
                                       size_t palette_size,
                                       const uint8_t *pixel_data,
                                       size_t pixel_size,
                                       uint32_t w,
                                       uint32_t h,
                                       bool reverse = false)
{
    // 2. 分配内存：调色盘 + 像素数据
    size_t total_size = palette_size + pixel_size;
    uint8_t *buf = (uint8_t *)malloc(total_size);

    // 3. 拷贝调色盘数据到 buf
    if (reverse) {
        memcpy(buf, reverse_palette, palette_size);
    } else {
        memcpy(buf, palette, palette_size);
    }

    // 4. 调色盘之后紧接索引像素数据
    memcpy(buf + palette_size, flipY_1bpp(pixel_data, w, h), pixel_size);

    lv_img_dsc_t *img_desc = (lv_img_dsc_t *)malloc(sizeof(lv_img_dsc_t));

    /* 初始化 lv_img_dsc_t 结构体 */
    img_desc->header.cf = LV_IMG_CF_INDEXED_1BIT;
    img_desc->header.always_zero = 0;
    img_desc->header.reserved = 0;
    img_desc->header.w = w;
    img_desc->header.h = h;
    img_desc->data_size = total_size;
    img_desc->data = buf;

    return img_desc;
}

void setup()
{
    Serial.begin(115200);

    Serial.println("Initializing board");
    Board *board = new Board();
    board->init();
#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB)
    {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    assert(board->begin());

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);

    // lv_img_dsc_t *my_img = create_indexed_1bit_image(
    //     palette,
    //     sizeof(palette),
    //     (default_icon + 62),
    //     48000,
    //     800, //800, // 宽
    //     480, //480  // 高,
    //     true
    // );

    lv_img_dsc_t *my_img = create_indexed_1bit_image(
        palette,
        sizeof(palette),
        wifi_connect_qr,
        2210,
        130, //800, // 宽
        130, //480  // 高,
        true
    );

    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, my_img);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    /**
     * Try an example. Don't forget to uncomment header.
     * See all the examples online: https://docs.lvgl.io/master/examples.html
     * source codes: https://github.com/lvgl/lvgl/tree/e7f88efa5853128bf871dde335c0ca8da9eb7731/examples
     */
    //  lv_example_btn_1();

    /**
     * Or try out a demo.
     * Don't forget to uncomment header and enable the demos in `lv_conf.h`. E.g. `LV_USE_DEMO_WIDGETS`
     */
    // lv_demo_widgets();
    // lv_demo_benchmark();
    // lv_demo_music();
    // lv_demo_stress();

    /* Release the mutex */
    lvgl_port_unlock();
}

void loop()
{
    Serial.println("IDLE loop");
    delay(1000);
}
