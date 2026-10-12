# 💧 Submersible Hydrostatic Level Transmitter (RS485 / 4-20mA) - Tokoasia Guide

[![Tokoasia Verified](https://img.shields.io/badge/Tokoasia-Verified-brightgreen?style=flat-square)](https://github.com/tokoasia)
[![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20Blynk%20IoT-blue?style=flat-square)](https://github.com/tokoasia)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](LICENSE)

Repositori resmi sensor ketinggian air celup (Hydrostatic Level Transmitter) RS485 Modbus RTU dan analog 4-20mA dari **Tokoasia** (SKU: ID1450).

---

## 📋 Fitur Utama
- Mengukur kedalaman air / tandon / sumur dengan presisi tinggi berbasis tekanan hidrostatik.
- Terintegrasi dengan **Blynk IoT 2.0** cloud monitoring & LCD 1602 I2C.
- Kalibrasi digital & konversi otomatis ke centimeter / persentase volume.

---

## 🔌 Skema Wiring
- Sensor VCC: 12V-24V DC Eksternal
- Sensor GND: Common GND
- RS485 A/B: Modul MAX485 ke ESP32 GPIO 16 (RX2) & GPIO 17 (TX2)

---

## 💻 Isi Repositori
- `esp32_blynk/ModBus_ESP32_RS485Level.ino`: Telemetri IoT ESP32 + Blynk + ModbusMaster.
- `calibration/Calibration_CheckScalePressure.ino`: Tool kalibrasi pembacaan tekanan air.

---
🛒 Beli produk resmi di [Shopee Tokoasia](https://shopee.co.id/tokoasia) | [store.tokoasia.me](https://store.tokoasia.me)
