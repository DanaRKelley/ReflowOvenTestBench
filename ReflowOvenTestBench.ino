
const bool DEBUG = true;
const int AMBIENT_TEMP = 27; // oven ambient temp
const int DELAY_GRAPH_ms = 100;

static bool coolToggleSwitch = false;
static int ovenTimeSeconds = 0; // time in seconds
const int graphArrayDepth = 422;

static byte stempArray [graphArrayDepth];
static byte ftempArray [graphArrayDepth];
static byte gtempArray [graphArrayDepth];

static int profile_index = 0;	// defaults to profile 0

char *profileStrings[] = {
	"P,PROFILE A", "P,PROFILE B", "P,PROFILE C", "P,PROFILE D", "P,ILLEGAL PROFILE NUMBER"
};

char *ovenPhase[] = {"D,WARMING"};

char serialData[] = {'\n', '\r', '\n'};
	
void setup() {
	Serial.begin(9600);
	while (!Serial) {} // wait for serial port to open
	Serial.flush();
	Serial.print(F("\nReflow Oven Test Bench\n"));
	Serial.print(F("\nHow This Works\n"));
	delay(1000);
	printInformation();
	Serial.println(F("\nReady, Set,..... GO!\n"));
	delay(300);
}

void loop() {
	static int reset = 0;
	static bool graphTemperatures = false;
	static bool profile_cmd = false;
	static int sideTemp;
	static int frontTemp;
	static int goalTemp;
	static bool runTempProfile;
	
	if (collectSerialLine()) {
			int inByte = serialData[0];
			int inByte2 = serialData[1];
			switch (inByte) {
			case '\n': 
			case '\r':
			break;
			case 'S': 
				if (inByte2 == '\n') {
				// start the oven profile
				ovenTimeSeconds = 2; // even numbers to match oven
				graphTemperatures = true;
				sideTemp = AMBIENT_TEMP;
				frontTemp = AMBIENT_TEMP + (random(0,2) == 1 ? 5 : -5);
				goalTemp = AMBIENT_TEMP;
				if (random(0,3) == 2) sideTemp = AMBIENT_TEMP + 10;
				runTempProfile = false;
				// reset state
				sendOvenPhase(goalTemp, true);
				calcTemperature(&goalTemp, ovenTimeSeconds, true);
				initArrays();
			} else {
				if (inByte2 == 'P') { // send profile name
					Serial.println(profileStrings[profile_index]);
				} else if (inByte2 == 'S') { // send oven state
					//Serial.println("oven state");
					printOvenState();
				} else if (inByte2 == 'H') { // send header starup text
					printInformation();
				}
			}
			break;
			case 'R':
				reset = 1;
				ovenTimeSeconds = 0;
				graphTemperatures = false;
				sideTemp = AMBIENT_TEMP;
				frontTemp = AMBIENT_TEMP;
				goalTemp = AMBIENT_TEMP;
				sendOvenPhase(goalTemp, true);
				initArrays();
			break;
			case 'C':	// toggle the cooling fan state 
				coolToggleSwitch = !coolToggleSwitch;
				if (coolToggleSwitch) Serial.println(F("C,COOL ON"));
				else Serial.println(F("C,COOL OFF"));
			break;
			case 'P': {
				int pindex = inByte2 - '0'; // CONVERT CHAR TO INT
				if (pindex >= 0 && pindex <= 3) {
					profile_cmd = true;
					profile_index = pindex;
				} else {
					Serial.print(F("ERROR: Missing or Wrong Profile Number, should be P0, P1, P2, P3 is P"));
					Serial.println(pindex);
				}
			}
			break;
			default: {
				Serial.print(F("ERROR: illegal command received: "));
				Serial.write(inByte);
				Serial.println();
			}
		}
	}
	
	if (graphTemperatures) {
		delay(DELAY_GRAPH_ms); // speed up for test purposes
	} else delay(1000);
	
	if (reset) { 
		Serial.println(F("OVEN RESET"));
		delay(2000); 
		Serial.println(F("D,DONE"));
		reset = 0;
	}
	
	if (profile_cmd) {
		profile_cmd = false;
		Serial.println(profileStrings[profile_index]);
	}
	
	// issue three temperature profiles 2 to 420 second long
	
	if (!runTempProfile && sideTemp <= AMBIENT_TEMP)
		runTempProfile = true;
	else if (ovenTimeSeconds && runTempProfile) {
		// use even n7umbers
		
		if (ovenTimeSeconds >= 2 && ovenTimeSeconds <= 420) {
			sendOvenPhase(goalTemp, false);
			calcTemperature(&goalTemp, ovenTimeSeconds, false);
			sideTemp = random(0,5) == 1 ? goalTemp + 7 : goalTemp + 5;
			frontTemp = sideTemp + 11;
			sendTemperature(sideTemp,frontTemp, goalTemp, ovenTimeSeconds);
			saveTemperatureData(sideTemp,frontTemp, goalTemp, ovenTimeSeconds);
			ovenTimeSeconds += 2;
		} else {
			graphTemperatures = false;
			ovenTimeSeconds = 0;
			goalTemp = AMBIENT_TEMP;
			calcTemperature(&goalTemp, ovenTimeSeconds, true);
		}
		
	} else if (!runTempProfile && ovenTimeSeconds) {
		// reach ambient temperature before executing oven profile
		sendTemperature(sideTemp,frontTemp, goalTemp, 0);
		saveTemperatureData(sideTemp,frontTemp, goalTemp, ovenTimeSeconds);
		sideTemp--;
	}
}

const int WARMINGTIME	=  30;	// time to ramp to warming temperature
const int SOAKTIME 		= 120;	// time to ramp into soak temperature range
const int RAMPTIME		= 150;	// time to ramp to reflow temperature
const int REFLOWTIME	= 210;	// time to cool down
const int COOLTIME		= 240;	// time to cool to below soak 
const int DONETIME		= 410;

const int WARMTEMP 		= 100;
const int SOAKTEMP 		= 150;
const int RAMPTEMP		= 183;
const int REFLOWTEMP	= 210;
const int PEAKTEMP		= 220;
const int COOLTEMP		= 183;
const int DONETEMP		=  60;

//void calcTemperature(int *side, int *front, int *goal, int timeSeconds, bool reset) {
void calcTemperature(int *goal, int timeSeconds, bool reset) {
	//
	int myGoal = *goal;
	static int savedTemp = 0;
	static byte latch = 0;
	
	if (reset) { latch = 0x01; }
	// Per Oven Phase Calculations
	//
	// startTemperature : phase started at this temperature
	// timeInPhase		: total time is in this phase
	// phaseLapsedTime	: phase has used this much time
	// 	: phaseLapsedTime/timeInPhase
	// deltaTemp		: phaseMaxTemp - startTemperature
	//
	// newTemp = startTemperature + phaseTimeFactor * deltaTemp
	static float deltaTemp = 0;
	static float phaseTimeFactor = 0;
	
	if (!reset && timeSeconds <= SOAKTIME) {
		// calculate temperature starting from 0 to soak time
		if (latch & 0x01) { 
			if (DEBUG) Serial.println(F("Entering Warming Phase"));
			 latch = latch << 1;
		}
		savedTemp = AMBIENT_TEMP;
		deltaTemp = (float)(WARMTEMP - savedTemp);
		phaseTimeFactor = (float)(timeSeconds - 0) / (float)(SOAKTIME - 0);
		
	} else if (!reset && timeSeconds <= RAMPTIME) {
		// calculate temperature starting from soak time (~120s) to ramp time (~150s)
		// latch previous phase base temp
		if (latch & 0x02) { 
			if (DEBUG) Serial.println(F("Entering Soak Phase"));
			savedTemp = myGoal; 
			deltaTemp = (float)(RAMPTEMP - myGoal);
			latch = latch << 1;
		}
		phaseTimeFactor = (float)(timeSeconds - SOAKTIME) / (float)(RAMPTIME - SOAKTIME);
		
	} else if (!reset && timeSeconds <= REFLOWTIME) {
		// calculate temperature starting from ramp time to reflow time
		// latch previous phase base temp
		if (latch & 0x04) { 
			if (DEBUG) Serial.println(F("Entering Ramp to Reflow Phase"));
			savedTemp = myGoal; 
			deltaTemp = (float)(REFLOWTEMP - myGoal);
			latch = latch << 1;
		}
		phaseTimeFactor = (float)(timeSeconds - RAMPTIME) / (float)(REFLOWTIME - RAMPTIME);
		
	} else if (!reset && timeSeconds <= COOLTIME) {
		// calculate temperature starting from reflow time to cool time
		// latch previous phase base temp
		if (latch & 0x08) { 
			if (DEBUG) Serial.println(F("Entering Reflow Phase"));
			savedTemp = myGoal;
			deltaTemp = (float)(COOLTEMP - myGoal);
			latch = latch << 1;
		}
		phaseTimeFactor = (float)(timeSeconds - REFLOWTIME) / (float)(COOLTIME - REFLOWTIME);
		
	} else if (!reset && timeSeconds <= DONETIME && myGoal > AMBIENT_TEMP+4) {
		if (latch & 0x10) { 
			if (DEBUG) Serial.println(F("Entering Cool Down Phase"));
			latch = latch << 1;
		}
		phaseTimeFactor = 0.0;
		deltaTemp = 0.0;
		savedTemp = myGoal - 2;
		
	} else if (!reset) { savedTemp = myGoal; phaseTimeFactor = 0.0; deltaTemp = 0.0; }
	
	
	if (!reset) {
		myGoal = savedTemp + phaseTimeFactor * deltaTemp;
	}
	
	*goal = myGoal;
}

void sendTemperature(int side, int front, int goal, int timeSeconds) {
	Serial.print(F("T,"));
			Serial.print(timeSeconds, DEC);
			Serial.print(F(","));
			Serial.print(side, DEC);
			Serial.print(F(","));
			Serial.print(front, DEC);
			Serial.print(F(","));
			Serial.print(goal, DEC);
			Serial.println(F(""));
}



void sendOvenPhase(int temp, bool reset) {
	static byte state = 0x0;
	bool printOvenPhase = false;
	
	if (reset) { 
		state = 0x01;
	} else if (state & 0x01 && temp >= AMBIENT_TEMP) {
		ovenPhase[0] = "D,WARMING";
		state = state << 1;
		printOvenPhase = true;
	} else if (state & 0x02 && temp >= WARMTEMP) {
		ovenPhase[0] = "D,SOAK"; 
		state = state << 1;
		printOvenPhase = true;
	} else if (state & 0x04 && temp >= RAMPTEMP) {
		ovenPhase[0] = "D,RAMP"; 
		state = state << 1;
		printOvenPhase = true;
	} else if (state & 0x08 && temp >= REFLOWTEMP) {
		ovenPhase[0] = "D,REFLOW"; 
		state = state << 1;
		printOvenPhase = true;
	} else if (state & 0x10 && temp <= DONETEMP) {
		ovenPhase[0] = "D,DONE";  
		state = 0;
		printOvenPhase = true;
	}
	
	if (printOvenPhase) {
		Serial.println(ovenPhase[0]);
		printOvenPhase = false;
	}
}

void initArrays() {
	for (int i = 0; i < graphArrayDepth; i++) {
		stempArray[i] = ftempArray[i] = gtempArray[i] = 0;
	}
}

void saveTemperatureData(int sideTemp, int frontTemp, int goalTemp, int ptr) {
	stempArray [ptr] = sideTemp;
	ftempArray [ptr] = frontTemp;
	gtempArray [ptr] = goalTemp;
}

void sendSavedTemperatureData () {
	for (int i = 0; i < graphArrayDepth; i++) {
		bool valid = stempArray[i] || ftempArray[i] || gtempArray[i];
		if (valid) 
			sendTemperature(stempArray [i], ftempArray [i], gtempArray [i], i);
	}
}






