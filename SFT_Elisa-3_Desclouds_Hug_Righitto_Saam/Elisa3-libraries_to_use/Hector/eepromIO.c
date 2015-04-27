#include "eepromIO.h"
#include <avr/eeprom.h>

void writeCalibrationToFlash() {	
//	eeprom_update_block(calibration, (uint8_t*) CALIB_DATA_START_ADDR, 144);                                        //missing function
//	eeprom_update_word ((uint16_t*) CALIB_CHECK_ADDRESS, 0xAA55);   // to let know the calibration data are valid   //missing function
	eeprom_write_block(calibration, (uint8_t*) CALIB_DATA_START_ADDR, 144);
	eeprom_write_word ((uint16_t*) CALIB_CHECK_ADDRESS, 0xAA55);   // to let know the calibration data are valid
}

void readCalibrationFromFlash() {
	eeprom_read_block (calibration, (uint8_t*) CALIB_DATA_START_ADDR, 144);
}




