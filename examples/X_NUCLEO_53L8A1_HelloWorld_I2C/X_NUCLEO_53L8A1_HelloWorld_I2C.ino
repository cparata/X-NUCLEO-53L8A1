 /**
 ******************************************************************************
 * @file    X_NUCLEO_53L8A1_HelloWorld_I2C.ino
 * @author  STMicroelectronics
 * @version V1.0.0
 * @date    16 January 2023
 * @brief   Arduino test application for the X-NUCLEO-53L8A1 based on VL53L8CX
 *          proximity sensor.
 *          This application makes use of C++ classes obtained from the C
 *          components' drivers.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2021 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include <Wire.h>
#include <vl53l8cx_class.h>


#ifdef ARDUINO_SAM_DUE
  #define DEV_I2C Wire1
#else
  #define DEV_I2C Wire
#endif
#define SerialPort Serial

#define LPN_PIN A3
#define I2C_RST_PIN -1
#define PWREN_PIN 11

void print_result(VL53L8CX_ResultsData *Result);
void clear_screen(void);
void handle_cmd(uint8_t cmd);
void display_commands_banner(void);

// Components.
VL53L8CX sensor_vl53l8cx_top(&DEV_I2C, LPN_PIN, I2C_RST_PIN);

bool EnableAmbient = false;
bool EnableSignal = false;
uint8_t res = VL53L8CX_RESOLUTION_4X4;
char report[256];

/* Setup ---------------------------------------------------------------------*/
void setup()
{

  // Enable PWREN pin if present
  if (PWREN_PIN >= 0) {
    pinMode(PWREN_PIN, OUTPUT);
    digitalWrite(PWREN_PIN, HIGH);
    delay(10);
  }

  // Initialize serial for output.
  SerialPort.begin(460800);

  // Initialize I2C bus.
  DEV_I2C.begin();

  // Configure VL53L8CX component.
  sensor_vl53l8cx_top.begin();
  Serial.println("HERE");

  sensor_vl53l8cx_top.init_sensor();
  Serial.println("HERE");

  // Start Measurements
  sensor_vl53l8cx_top.vl53l8cx_start_ranging();
    Serial.println("HERE");

}

void loop()
{
  VL53L8CX_ResultsData Results;
  uint8_t NewDataReady = 0;
  uint8_t status;
  //Serial.println("HERE");

  do {
    status = sensor_vl53l8cx_top.vl53l8cx_check_data_ready(&NewDataReady);
  } while (!NewDataReady);

  if ((!status) && (NewDataReady != 0)) {
    status = sensor_vl53l8cx_top.vl53l8cx_get_ranging_data(&Results);
    print_result(&Results);
  }

  if (Serial.available()>0)
  {
    handle_cmd(Serial.read());
  }
  delay(1000);
}

void print_result(VL53L8CX_ResultsData *Result)
{
  int8_t i, j, k, l;
  uint8_t zones_per_line;
  uint8_t number_of_zones = res;

  zones_per_line = (number_of_zones == 16) ? 4 : 8;

  display_commands_banner();

  SerialPort.print("Cell Format :\n\n");
  
  for (l = 0; l < VL53L8CX_NB_TARGET_PER_ZONE; l++)
  {
    snprintf(report, sizeof(report)," \033[38;5;10m%20s\033[0m : %20s\n", "Distance [mm]", "Status");
    SerialPort.print(report);

    if(EnableAmbient || EnableSignal)
    {
      snprintf(report, sizeof(report)," %20s : %20s\n", "Signal [kcps/spad]", "Ambient [kcps/spad]");
      SerialPort.print(report);
    }
  }

  SerialPort.print("\n\n");

  for (j = 0; j < number_of_zones; j += zones_per_line)
  {
    for (i = 0; i < zones_per_line; i++) 
      SerialPort.print(" -----------------");
    SerialPort.print("\n");
    
    for (i = 0; i < zones_per_line; i++)
      SerialPort.print("|                 ");
    SerialPort.print("|\n");
  
    for (l = 0; l < VL53L8CX_NB_TARGET_PER_ZONE; l++)
    {
      // Print distance and status 
      for (k = (zones_per_line - 1); k >= 0; k--)
      {
        if (Result->nb_target_detected[j+k]>0)
        {
          snprintf(report, sizeof(report),"| \033[38;5;10m%5ld\033[0m  :  %5ld ",
              (uint32_t)Result->distance_mm[(VL53L8CX_NB_TARGET_PER_ZONE * (j+k)) + l],
              (long)Result->target_status[(VL53L8CX_NB_TARGET_PER_ZONE * (j+k)) + l]);
              SerialPort.print(report);
        }
        else
        {
          snprintf(report, sizeof(report),"| %5s  :  %5s ", "X", "X");
          SerialPort.print(report);
        }
      }
      SerialPort.print("|\n");

      if (EnableAmbient || EnableSignal )
      {
        // Print Signal and Ambient 
        for (k = (zones_per_line - 1); k >= 0; k--)
        {
          if (Result->nb_target_detected[j+k]>0)
          {
            if (EnableSignal)
            {
              snprintf(report, sizeof(report),"| %5ld  :  ", (long)Result->signal_per_spad[(VL53L8CX_NB_TARGET_PER_ZONE * (j+k)) + l]);
              SerialPort.print(report);
            }
            else
            {
              snprintf(report, sizeof(report),"| %5s  :  ", "X");
              SerialPort.print(report);
            }
            if (EnableAmbient)
            {
              snprintf(report, sizeof(report),"%5ld ", (long)Result->ambient_per_spad[j+k]);
              SerialPort.print(report);
            }
            else
            {
              snprintf(report, sizeof(report),"%5s ", "X");
              SerialPort.print(report);
            }
          }
          else
          {
            snprintf(report, sizeof(report),"| %5s  :  %5s ", "X", "X");
            SerialPort.print(report);
          }
        }
        SerialPort.print("|\n");
      }
    }
  }
  for (i = 0; i < zones_per_line; i++)
   SerialPort.print(" -----------------");
  SerialPort.print("\n");
}

void toggle_resolution(void)
{
  sensor_vl53l8cx_top.vl53l8cx_stop_ranging();

  switch (res)
  {
    case VL53L8CX_RESOLUTION_4X4:
      res = VL53L8CX_RESOLUTION_8X8;
      break;

    case VL53L8CX_RESOLUTION_8X8:
      res = VL53L8CX_RESOLUTION_4X4;
      break;

    default:
      break;
  }
  sensor_vl53l8cx_top.vl53l8cx_set_resolution(res);
  sensor_vl53l8cx_top.vl53l8cx_start_ranging();
}

void toggle_signal_and_ambient(void)
{
  EnableAmbient = (EnableAmbient) ? false : true;
  EnableSignal = (EnableSignal) ? false : true;
}

void clear_screen(void)
{
  snprintf(report, sizeof(report),"%c[2J", 27); /* 27 is ESC command */
  SerialPort.print(report);
}

void display_commands_banner(void)
{
  snprintf(report, sizeof(report),"%c[2H", 27); /* 27 is ESC command */
  SerialPort.print(report);

  Serial.print("53L8A1 Simple Ranging demo application\n");
  Serial.print("--------------------------------------\n\n");

  Serial.print("Use the following keys to control application\n");
  Serial.print(" 'r' : change resolution\n");
  Serial.print(" 's' : enable signal and ambient\n");
  Serial.print(" 'c' : clear screen\n");
  Serial.print("\n");
}

void handle_cmd(uint8_t cmd)
{
  switch (cmd)
  {
    case 'r':
      toggle_resolution();
      clear_screen();
      break;

    case 's':
      toggle_signal_and_ambient();
      clear_screen();
      break;

    case 'c':
      clear_screen();
      break;

    default:
      break;
  }
}
