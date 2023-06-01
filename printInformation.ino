
void printInformation() {
	
	Serial.println(F("Send Oven a Serial ASCII Command to execute a Test"));
	Serial.println(F("Commands: are one or two letter commands"));
	Serial.println(F("'R' - reset oven, oven returns text response 'OVEN RESET'"));
	Serial.println(F("	  followed by 'D,COOL' then after a few seconds 'D,READY'"));
	Serial.println(F("	  COOL and READY are displayed in Display Phase icon"));
	Serial.println(F(""));
	Serial.println(F("'S' - start oven, oven returns 'D,WARMING'"));
	Serial.println(F("	  followed by 'D,SOAK', 'D,RAMP', 'D,REFLOW', 'D,COOL', 'D,DONE'"));
	Serial.println(F("	  each phase will take a few seconds while sending plot data"));
	Serial.println(F("	  data format: 'T,int,int,int,int' representing time in SECONDS,"));
	Serial.println(F("	  and SIDE, FRONT, GOAL temperatures"));
	Serial.println(F(""));
	Serial.println(F("	  the oven if too warm on startup will first enter a cooling cycle."));
	Serial.println(F("	  in this even a 'D,COOL' is returned while it cools down"));
	Serial.println(F(""));
	Serial.println(F("'C' - toggle oven cooling fan,'C,COOL ON' or 'C,COOL OFF'"));
	Serial.println(F("	  display 'COOL ON or OFF' in Display Phase icon"));
	Serial.println(F("	  it does not impact oven operations"));
	Serial.println(F(""));
	Serial.println(F("'P0, P1, P2, or P3' - oven returns 'P,PROFILE NAME'"));
	Serial.println(F(""));
	Serial.println(F("'SP' - return Profile Name"));
	Serial.println(F(""));
	Serial.println(F("'SS' - return all system state, includes all graph data"));
	Serial.println(F(""));
	
	
	
	
	
	
	
	
	
}
