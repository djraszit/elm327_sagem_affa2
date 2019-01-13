/*
 * elm327.h
 *
 *  Created on: 26 sie 2017
 *      Author: djraszit
 */

#ifndef INC_ELM327_H_
#define INC_ELM327_H_


//commands
#define ELM327_RESET				"Z"
#define ELM327_ECHO					"E"
#define ELM327_LINEFEED				"L"
#define ELM327_SET_PROTOCOL			"SP"
#define ELM327_LOW_POWER_MODE		"LP"
#define ELM327_WARM_START			"WS"
#define ELM327_READ_INPUT_VOLTAGE	"RV"
#define ELM327_CLOSE_PROTOCOL		"CP"
#define ELM327_FAST_INIT			"FI"

enum {
	ATCMD_NOT_SENT, ATCMD_REQUEST, ATCMD_SENT, ATCMD_OK
};

enum {
	DATA_REQUEST_NOT_SENT, DATA_REQUEST, DATA_SENT, DATA_REPLY
};

enum {
	PROTOCOL_CLOSE, PROTOCOL_OPEN
};

//modes
enum ELM327_modes{
	show_current_data = 1,
	show_freeze_frame_data,
	show_diagnostic_trouble_codes,
	clear_trouble_codes_and_stored_values,
	test_results_oxygen_sensors,
	test_results_non_continuously_monitored,
	show_pending_trouble_codes,
	special_control_mode,
	request_vehicle_information,
	request_permanent_trouble_codes
};

//pids mode 1
#define PIDS_SUPPORTED_1_20		0x00
#define FREEZE_DTC				0x02
#define FUEL_SYSTEM_STATUS		0x03
#define CALCULATED_ENGINE_LOAD	0x04
#define ENGINE_COOLANT_TEMP		0x05
#define ENGINE_RPM				0x0c
#define VEHICLE_SPEED			0x0d
#define TIMING_ADVANCE			0x0e
#define INTAKE_AIR_TEMP			0x0f
#define THROTTLE_POSITION		0x11
#define END_ARG					0xffff

void elm327_send_at_cmd(char* cmd);
void elm327_send(char* cmd);
uint8_t elm327_request(int mode, ...);
int16_t elm327_calculate_data(uint8_t * buf, char * text);


#endif /* INC_ELM327_H_ */
