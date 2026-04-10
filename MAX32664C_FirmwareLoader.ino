#include <Arduino.h>
#include <Wire.h>

#include "max32664c_firmware_image.h"

#define HUB_ADDR 0x55
#define MFIO D7 //Put your Pin
#define RSTN D6 //Put your Pin

static const uint16_t HUB_I2C_CLOCK_HZ = 400000;
static const uint32_t FW_PAGE_SIZE = 8192;
static const uint32_t FW_CRC_SIZE = 16;
static const uint32_t FW_WRITE_SIZE = FW_PAGE_SIZE + FW_CRC_SIZE;
static const uint32_t FW_FIRST_PAGE_OFFSET = 0x4C;
static const uint16_t DEFAULT_CMD_DELAY_MS = 10;
static const uint16_t PAGE_WRITE_DELAY_MS = 680;

static const uint8_t TARGET_FW_MAJOR = 30;
static const uint8_t TARGET_FW_MINOR = 13;
static const uint8_t TARGET_FW_PATCH = 31;

static uint8_t pageBuffer[FW_WRITE_SIZE + 2];

static void hubWake()
{
  digitalWrite(MFIO, LOW);
  delayMicroseconds(300);
}

static void hubSleep()
{
  digitalWrite(MFIO, HIGH);
}

static void resetToApplicationMode()
{
  pinMode(MFIO, OUTPUT);
  pinMode(RSTN, OUTPUT);

  digitalWrite(MFIO, HIGH);
  digitalWrite(RSTN, LOW);
  delay(20);

  digitalWrite(RSTN, HIGH);
  delay(1500);
}

static bool appReadCommand(uint8_t family, uint8_t index, uint8_t *rx, size_t rxLen)
{
  hubWake();

  Wire.beginTransmission(HUB_ADDR);
  Wire.write(family);
  Wire.write(index);
  if (Wire.endTransmission() != 0) {
    hubSleep();
    return false;
  }

  delay(DEFAULT_CMD_DELAY_MS);

  if (Wire.requestFrom(HUB_ADDR, (uint8_t)rxLen) != (int)rxLen) {
    hubSleep();
    return false;
  }

  for (size_t i = 0; i < rxLen; ++i) {
    rx[i] = Wire.read();
  }

  delay(DEFAULT_CMD_DELAY_MS);
  hubSleep();

  return rx[0] == 0;
}

static bool readFirmwareVersion(uint8_t &major, uint8_t &minor, uint8_t &patch)
{
  uint8_t rx[4] = {0};

  if (!appReadCommand(0xFF, 0x03, rx, sizeof(rx))) {
    return false;
  }

  major = rx[1];
  minor = rx[2];
  patch = rx[3];
  return true;
}

static bool bootloaderTransmit(const uint8_t *tx, size_t txLen, uint8_t *rx, size_t rxLen)
{
  Wire.beginTransmission(HUB_ADDR);
  for (size_t i = 0; i < txLen; ++i) {
    Wire.write(tx[i]);
  }
  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(DEFAULT_CMD_DELAY_MS);

  if (Wire.requestFrom(HUB_ADDR, (uint8_t)rxLen) != (int)rxLen) {
    return false;
  }

  for (size_t i = 0; i < rxLen; ++i) {
    rx[i] = Wire.read();
  }

  delay(DEFAULT_CMD_DELAY_MS);
  return rx[0] == 0;
}

static bool bootloaderWritePage(const uint8_t *firmware, uint32_t offset)
{
  pageBuffer[0] = 0x80;
  pageBuffer[1] = 0x04;
  memcpy(&pageBuffer[2], &firmware[offset], FW_WRITE_SIZE);

  Wire.beginTransmission(HUB_ADDR);
  Wire.write(pageBuffer, sizeof(pageBuffer));
  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(PAGE_WRITE_DELAY_MS);

  if (Wire.requestFrom(HUB_ADDR, (uint8_t)1) != 1) {
    return false;
  }

  uint8_t status = Wire.read();
  delay(DEFAULT_CMD_DELAY_MS);

  return status == 0;
}

static bool eraseApplication()
{
  const uint8_t tx[] = {0x80, 0x03};

  Wire.beginTransmission(HUB_ADDR);
  Wire.write(tx, sizeof(tx));
  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(1500);

  if (Wire.requestFrom(HUB_ADDR, (uint8_t)1) != 1) {
    return false;
  }

  return Wire.read() == 0;
}

static bool leaveBootloader()
{
  uint8_t rx[4] = {0};

  digitalWrite(RSTN, HIGH);
  digitalWrite(MFIO, LOW);
  delay(2000);

  digitalWrite(RSTN, LOW);
  delay(5);

  digitalWrite(MFIO, HIGH);
  delay(15);

  digitalWrite(RSTN, HIGH);
  delay(1700);

  if (!appReadCommand(0x02, 0x00, rx, 2) || rx[1] != 0x00) {
    Serial.println("Failed to re-enter application mode");
    return false;
  }

  uint8_t major = 0;
  uint8_t minor = 0;
  uint8_t patch = 0;
  if (!readFirmwareVersion(major, minor, patch)) {
    Serial.println("Failed to read FW version after update");
    return false;
  }

  Serial.print("Firmware after update: ");
  Serial.print(major);
  Serial.print(".");
  Serial.print(minor);
  Serial.print(".");
  Serial.println(patch);

  return true;
}

static bool programHubFirmware(const uint8_t *firmware, uint32_t size)
{
  uint8_t rx[4] = {0};
  uint8_t tx[18] = {0};

  if (size < (FW_FIRST_PAGE_OFFSET + 12 + 16)) {
    Serial.println("Firmware image too small");
    return false;
  }

  const uint8_t numPages = firmware[0x44];
  const uint32_t requiredSize = FW_FIRST_PAGE_OFFSET + ((uint32_t)numPages * FW_WRITE_SIZE);
  if (size < requiredSize) {
    Serial.println("Firmware image size does not match page count");
    return false;
  }

  pinMode(MFIO, OUTPUT);
  pinMode(RSTN, OUTPUT);

  Serial.println("Entering bootloader mode...");
  digitalWrite(RSTN, LOW);
  delay(20);

  digitalWrite(MFIO, LOW);
  delay(20);

  digitalWrite(RSTN, HIGH);
  delay(200);

  tx[0] = 0x01;
  tx[1] = 0x00;
  tx[2] = 0x08;
  if (!bootloaderTransmit(tx, 3, rx, 1)) {
    Serial.println("Failed to set bootloader mode");
    return false;
  }

  tx[0] = 0x02;
  tx[1] = 0x00;
  if (!bootloaderTransmit(tx, 2, rx, 2) || rx[1] != 0x08) {
    Serial.println("Device did not enter bootloader mode");
    return false;
  }

  tx[0] = 0x81;
  tx[1] = 0x00;
  if (!bootloaderTransmit(tx, 2, rx, 4)) {
    Serial.println("Failed to read bootloader version");
    return false;
  }

  Serial.print("Bootloader version: ");
  Serial.print(rx[1]);
  Serial.print(".");
  Serial.print(rx[2]);
  Serial.print(".");
  Serial.println(rx[3]);

  tx[0] = 0x81;
  tx[1] = 0x01;
  if (!bootloaderTransmit(tx, 2, rx, 3)) {
    Serial.println("Failed to read bootloader page size");
    return false;
  }

  const uint16_t pageSize = ((uint16_t)rx[1] << 8) | rx[2];
  Serial.print("Bootloader page size: ");
  Serial.println(pageSize);

  if (pageSize != FW_PAGE_SIZE) {
    Serial.println("Unexpected page size");
    return false;
  }

  Serial.print("Firmware pages: ");
  Serial.println(numPages);

  tx[0] = 0x80;
  tx[1] = 0x02;
  tx[2] = 0x00;
  tx[3] = numPages;
  if (!bootloaderTransmit(tx, 4, rx, 1)) {
    Serial.println("Failed to set page count");
    return false;
  }

  tx[0] = 0x80;
  tx[1] = 0x00;
  memcpy(&tx[2], &firmware[0x28], 11);
  if (!bootloaderTransmit(tx, 13, rx, 1)) {
    Serial.println("Failed to write init vector");
    return false;
  }

  tx[0] = 0x80;
  tx[1] = 0x01;
  memcpy(&tx[2], &firmware[0x34], 16);
  if (!bootloaderTransmit(tx, 18, rx, 1)) {
    Serial.println("Failed to write auth vector");
    return false;
  }

  Serial.println("Erasing existing application...");
  if (!eraseApplication()) {
    Serial.println("Application erase failed");
    return false;
  }

  Serial.println("Writing firmware pages...");
  uint32_t pageOffset = FW_FIRST_PAGE_OFFSET;
  for (uint8_t page = 0; page < numPages; ++page) {
    Serial.print("Page ");
    Serial.print(page + 1);
    Serial.print(" / ");
    Serial.println(numPages);

    if (!bootloaderWritePage(firmware, pageOffset)) {
      Serial.print("Page write failed at offset 0x");
      Serial.println(pageOffset, HEX);
      return false;
    }

    pageOffset += FW_WRITE_SIZE;
  }

  Serial.println("Firmware transfer complete");
  return leaveBootloader();
}

static void printCurrentFirmwareVersion()
{
  uint8_t major = 0;
  uint8_t minor = 0;
  uint8_t patch = 0;

  if (!readFirmwareVersion(major, minor, patch)) {
    Serial.println("FW read failed");
    return;
  }

  Serial.print("Current hub FW: ");
  Serial.print(major);
  Serial.print(".");
  Serial.print(minor);
  Serial.print(".");
  Serial.println(patch);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Wire.setPins(PIN_WIRE_SCL, PIN_WIRE_SDA);
  Wire.begin();
  Wire.setClock(HUB_I2C_CLOCK_HZ);

  pinMode(MFIO, OUTPUT);
  pinMode(RSTN, OUTPUT);

  resetToApplicationMode();

  Serial.println();
  Serial.println("=== MAX32664C firmware loader ===");
  printCurrentFirmwareVersion();

  uint8_t major = 0;
  uint8_t minor = 0;
  uint8_t patch = 0;
  const bool versionReadOk = readFirmwareVersion(major, minor, patch);
  const bool alreadyUpToDate =
    versionReadOk &&
    major == TARGET_FW_MAJOR &&
    minor == TARGET_FW_MINOR &&
    patch == TARGET_FW_PATCH;

  if (alreadyUpToDate) {
    Serial.println("Target firmware already installed");
    return;
  }

  Serial.println("Starting firmware update...");
  if (programHubFirmware(MAX32664C_HSP2_WHRM_AEC_SCD_WSPO2_C_30_13_31,
                         sizeof(MAX32664C_HSP2_WHRM_AEC_SCD_WSPO2_C_30_13_31))) {
    Serial.println("Firmware update completed successfully");
  } else {
    Serial.println("Firmware update failed");
  }
}

void loop()
{
  static uint32_t lastPrint = 0;

  if (millis() - lastPrint >= 5000) {
    lastPrint = millis();
    printCurrentFirmwareVersion();
  }
}
