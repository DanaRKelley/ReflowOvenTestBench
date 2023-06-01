
// read characters from serial port
// build one or two character command
// 		character followed by \n or \r
//					or
//		two characters followed by \n or \r
//
//	return true when serialData is valid for processing

bool collectSerialLine() {
	static int state = 0; // empty state
	static int inByte = 0;
	bool dataValid = false;
	
	if (Serial.available() > 0) { // one or more characters?
		inByte = Serial.read();
		
		if (state == 0) {
			if (inByte != '\n' && inByte != '\r') {
				serialData[0] = inByte;
				state = 1; // one byte collected
			}
		} else if (state == 1) {
			if (inByte == '\n' || inByte == '\r') { // last character
				serialData[1] = '\n';
				serialData[2] = '\r';
				dataValid = true;
				state = 0;
			} else {
				serialData[1] = inByte; // second character
				state = 2;
			}
		} else if (state == 2) {
			if (inByte == '\n' || inByte == '\r') { // last character
				serialData[2] = '\n';
				dataValid = true;
				state = 0;
			} else {	// unexpected character string
				state = 0;
				Serial.print(F("INVALID COMMAND RECEIVED: "));
				Serial.println(serialData);
				
			}
		} else state = 0;
	}
	
	return dataValid;
}