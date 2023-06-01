
//

void printOvenState() {
	Serial.println(F("Oven State Begin"));
	
	// Profile Selected
	Serial.println(profileStrings[profile_index]);
	
	// Cooling Fan Toggle Switch
	if (coolToggleSwitch) Serial.println(F("D,COOL ON"));
	else Serial.println(F("D,COOL OFF"));
	
	// Oven Temperature graphs if operational
	if (true || ovenTimeSeconds) {
		Serial.println(ovenPhase[0]);
		Serial.println(F("Sending Over Graph Data"));
		sendSavedTemperatureData();
	}
	
	Serial.println(F("Oven State End"));
}