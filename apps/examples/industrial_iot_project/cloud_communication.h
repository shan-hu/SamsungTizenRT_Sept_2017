//
//  cloud_communication.h
//  industrial_iot_project
//
//  Created by Tao Ma-SSI on 4/20/17.
//  Copyright © 2017 Tao Ma-SSI. All rights reserved.
//

#ifndef _CLOUD_COMMUNICATION_H_
#define _CLOUD_COMMUNICATION_H_


artik_error SendMessageToCloud(char*);

// for vibration threshold getting from the cloud
extern int motor_control_vib_global;

// for vibration reporting factor from the cloud
extern float vibration_normalization_factor;

#endif /* cloud_communication_h */
