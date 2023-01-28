//  Copyright 2017 by Malte Janduda
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#ifndef UDP_LOGGING_MAX_PAYLOAD_LEN
#define UDP_LOGGING_MAX_PAYLOAD_LEN 2048
#endif

#include "esp_system.h"
#include "esp_log.h"
#include "WiFi.h"
#include "AsyncUDP.h"


static char buf[UDP_LOGGING_MAX_PAYLOAD_LEN];

AsyncUDP udp;



int udp_logging_vprintf( const char *str, va_list l ) {
    
	int ret = vsnprintf(buf, sizeof(buf), str, l);
	udp.write((uint8_t *)buf, (size_t)ret);
	return vprintf( str, l );
}

int udp_logging_init(const IPAddress addr, unsigned long port, vprintf_like_t func) {


	uint32_t t_end_timer=millis()+5000;

	ESP_LOGI("UDP_LOGGING", "initializing udp logging...");
	if (udp.connect(addr, port))
	{
		while (!udp.connected() && (t_end_timer - millis() > 0))
		{
			ESP_LOGV("UDP_LOGGING", "UDP Wait for connection");
		}
		ESP_LOGV("UDP_LOGGING", "set esp_log_set_vprintf");
		esp_log_set_vprintf(func);
	}
	else
	{
		return 0;
	}

    return 0;
}


void udp_close(){
	udp.close();
}