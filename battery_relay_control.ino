// ============================================================
// BATTERY CHARGE / DISCHARGE RELAY CONTROL
// Arduino UNO
// ============================================================
//
// MODE:
//   1 = CHARGING MODE
//   2 = DISCHARGING MODE
//
// ------------------------------------------------------------
// CHARGING MODE (MODE = 1)
// ------------------------------------------------------------
// A0  -> Charging Current (Ic)
// A2  -> Battery Voltage (Vb)
// A3  -> Supply Voltage (Vs)
// D6  -> Relay
//
// ------------------------------------------------------------
// DISCHARGING MODE (MODE = 2)
// ------------------------------------------------------------
// A1  -> Discharging Current (Id)
// A2  -> Battery Voltage (Vb)
// D6  -> Relay
//
// ------------------------------------------------------------
// RELAY LOGIC
// ------------------------------------------------------------
// LOW  = Relay ON
// HIGH = Relay OFF
//
// Temporary thresholds:
//   Ic >= 7.0 A -> 5 sec continuous -> Relay ON
//   Id >= 7.0 A -> 5 sec continuous -> Relay ON
//   Vb > 14.0 V -> 5 sec continuous -> Relay ON
//
// Immediate thresholds:
//   Ic >= 7.5 A -> Permanent Relay ON
//   Id >= 7.5 A -> Permanent Relay ON
//   Vb >= 14.5 V -> Permanent Relay ON
//
// Temporary relay operation:
//   Relay ON for 2 minutes
//   After 2 minutes -> check active conditions again
//
//   If condition is still active:
//       Relay remains ON for another 2 minutes
//
//   If condition is cleared:
//       Relay OFF
//
// Permanent ON:
//   Remains ON until Arduino RESET
//
// ============================================================


// ============================================================
// MODE SELECTION
// ============================================================

// 1 = CHARGING
// 2 = DISCHARGING

const int MODE = 2;


// ============================================================
// PIN DEFINITIONS
// ============================================================

const int PIN_IC    = A0;    // Charging current
const int PIN_ID    = A1;    // Discharging current
const int PIN_VB    = A2;    // Battery voltage
const int PIN_VS    = A3;    // Supply voltage

const int PIN_RELAY = 6;     // Relay output


const float ADC_REF = 5.0;
const int ADC_RES = 1023;


const float CURRENT_SENSE_RATIO = 0.1;
// Battery voltage divider
const float BATTERY_VOLTAGE_DIVIDER = 4.26;
// Supply voltage divider
const float SUPPLY_VOLTAGE_DIVIDER = 5.54;


const float IC_THRESHOLD  = 1.0;     // Charging current threshold
const float IC_IMMEDIATE  = 1.5;     // Charging current immediate

const float ID_THRESHOLD  = 4.0;     // Discharging current threshold
const float ID_IMMEDIATE  = 5.0;     // Discharging current immediate


const float VB_THRESHOLD = 14.0;      // Temporary threshold
const float VB_IMMEDIATE = 14.5;      // Permanent threshold


const unsigned long STABLE_TIME = 5000UL;      // 5 seconds

const unsigned long RELAY_ON_TIME = 120000UL;  // 2 minutes


// Current relay state
bool relayOn = false;


// Permanent latch
bool permanentOn = false;


// 5-second monitoring timers
unsigned long icTriggerStart = 0;
unsigned long idTriggerStart = 0;
unsigned long vbTriggerStart = 0;


// 2-minute relay timer
unsigned long relayOnStart = 0;


// ============================================================
// SETUP
// ============================================================

void setup()
{
    // Relay output
    pinMode(PIN_RELAY, OUTPUT);

    // Relay OFF during startup
    digitalWrite(PIN_RELAY, HIGH);

    // Serial monitor
    Serial.begin(9600);

    Serial.println();
    Serial.println("========================================");
    Serial.println(" Battery Charge / Discharge Controller");
    Serial.println("========================================");


    // --------------------------------------------------------
    // MODE INFORMATION
    // --------------------------------------------------------

    if (MODE == 1)
    {
        Serial.println("MODE = 1 : CHARGING");

        Serial.println("A0 = Charging Current (Ic)");
        Serial.println("A2 = Battery Voltage (Vb)");
        Serial.println("A3 = Supply Voltage (Vs)");

        Serial.println("A1 = IGNORED");
    }

    else if (MODE == 2)
    {
        Serial.println("MODE = 2 : DISCHARGING");

        Serial.println("A1 = Discharging Current (Id)");
        Serial.println("A2 = Battery Voltage (Vb)");

        Serial.println("A0 = IGNORED");
        Serial.println("A3 = IGNORED");
    }

    else
    {
        Serial.println("ERROR: INVALID MODE!");

        while (1)
        {
            digitalWrite(PIN_RELAY, HIGH);
        }
    }

    Serial.println("----------------------------------------");
    Serial.println("Relay: LOW = ON");
    Serial.println("Relay: HIGH = OFF");
    Serial.println("----------------------------------------");

    Serial.println("System started.");
    Serial.println();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
    unsigned long now = millis();


    // ========================================================
    // SENSOR VARIABLES
    // ========================================================

    float Ic = 0.0;
    float Id = 0.0;
    float Vb = 0.0;
    float Vs = 0.0;


    // ========================================================
    // CHARGING MODE
    // ========================================================

    if (MODE == 1)
    {
        // Read ONLY charging-related sensors

        Ic = readChargingCurrent();

        Vb = readBatteryVoltage();

        Vs = readSupplyVoltage();

        // A1 / Id is completely ignored
    }


    // ========================================================
    // DISCHARGING MODE
    // ========================================================

    else if (MODE == 2)
    {
        // Read ONLY discharging-related sensors

        Id = readDischargingCurrent();

        Vb = readBatteryVoltage();

        // A0 / Ic is completely ignored
        // A3 / Vs is completely ignored
    }


    // ========================================================
    // PERMANENT ON STATE
    // ========================================================

    if (permanentOn)
    {
        relayON();
    }

    else
    {
        // ====================================================
        // CHECK IMMEDIATE CONDITIONS
        // ====================================================

        // ----------------------------------------------------
        // CHARGING MODE
        // ----------------------------------------------------

        if (MODE == 1)
        {
            // Charging current immediate condition

            if (Ic >= IC_IMMEDIATE)
            {
                permanentOn = true;

                Serial.println();
                Serial.println("************************************");
                Serial.println("IC IMMEDIATE CONDITION");
                Serial.print("Ic = ");
                Serial.print(Ic, 2);
                Serial.println(" A");
                Serial.println("Ic >= 7.5 A");
                Serial.println("RELAY = PERMANENT ON");
                Serial.println("************************************");
            }
        }


        // ----------------------------------------------------
        // DISCHARGING MODE
        // ----------------------------------------------------

        else if (MODE == 2)
        {
            // Discharging current immediate condition

            if (Id >= ID_IMMEDIATE)
            {
                permanentOn = true;

                Serial.println();
                Serial.println("************************************");
                Serial.println("ID IMMEDIATE CONDITION");
                Serial.print("Id = ");
                Serial.print(Id, 2);
                Serial.println(" A");
                Serial.println("Id >= 5.0 A");
                Serial.println("RELAY = PERMANENT ON");
                Serial.println("************************************");
            }
        }


        // ====================================================
        // BATTERY VOLTAGE IMMEDIATE CONDITION
        // ====================================================

        if (Vb >= VB_IMMEDIATE)
        {
            permanentOn = true;

            Serial.println();
            Serial.println("************************************");
            Serial.println("BATTERY VOLTAGE IMMEDIATE CONDITION");
            Serial.print("Vb = ");
            Serial.print(Vb, 2);
            Serial.println(" V");
            Serial.println("Vb >= 14.5 V");
            Serial.println("RELAY = PERMANENT ON");
            Serial.println("************************************");
        }


        // ====================================================
        // PERMANENT CONDITION ACTIVATED
        // ====================================================

        if (permanentOn)
        {
            relayON();
        }


        // ====================================================
        // NORMAL TEMPORARY CONDITION PROCESSING
        // ====================================================

        else
        {
            processTemporaryConditions(Ic, Id, Vb, now);
        }
    }


    // ========================================================
    // SERIAL MONITOR
    // ========================================================

    Serial.print("Ic: ");
    Serial.print(Ic, 2);
    Serial.print(" A | ");

    Serial.print("Id: ");
    Serial.print(Id, 2);
    Serial.print(" A | ");

    Serial.print("Vb: ");
    Serial.print(Vb, 2);
    Serial.print(" V | ");

    Serial.print("Vs: ");
    Serial.print(Vs, 2);
    Serial.print(" V | ");

    Serial.print("Relay: ");

    if (relayOn)
    {
        Serial.print("ON");
    }
    else
    {
        Serial.print("OFF");
    }

    if (permanentOn)
    {
        Serial.print(" [PERMANENT]");
    }

    Serial.println();


    // ========================================================
    // LOOP DELAY
    // ========================================================

    delay(100);
}


// ============================================================
// TEMPORARY CONDITION PROCESSING
// ============================================================

void processTemporaryConditions(
    float Ic,
    float Id,
    float Vb,
    unsigned long now
)
{
    // ========================================================
    // RELAY CURRENTLY OFF
    // ========================================================

    if (!relayOn)
    {
        bool triggerRelay = false;


        // ====================================================
        // CHARGING MODE
        // ====================================================

        if (MODE == 1)
        {
            // ------------------------------------------------
            // CHARGING CURRENT >= 7.0 A
            // ------------------------------------------------

            if (Ic >= IC_THRESHOLD)
            {
                if (icTriggerStart == 0)
                {
                    icTriggerStart = now;

                    Serial.println(
                        "Ic >= 7.0 A : 5 sec timer started"
                    );
                }

                if (now - icTriggerStart >= STABLE_TIME)
                {
                    triggerRelay = true;

                    Serial.println(
                        "Ic >= 7.0 A for 5 sec"
                    );

                    Serial.println(
                        "RELAY ON for 2 minutes"
                    );

                    icTriggerStart = 0;
                }
            }

            else
            {
                // Ic dropped below threshold
                icTriggerStart = 0;
            }
        }


        // ====================================================
        // DISCHARGING MODE
        // ====================================================

        else if (MODE == 2)
        {
            // ------------------------------------------------
            // DISCHARGING CURRENT >= 7.0 A
            // ------------------------------------------------

            if (Id >= ID_THRESHOLD)
            {
                if (idTriggerStart == 0)
                {
                    idTriggerStart = now;

                    Serial.println(
                        "Id >= 7.0 A : 5 sec timer started"
                    );
                }

                if (now - idTriggerStart >= STABLE_TIME)
                {
                    triggerRelay = true;

                    Serial.println(
                        "Id >= 7.0 A for 5 sec"
                    );

                    Serial.println(
                        "RELAY ON for 2 minutes"
                    );

                    idTriggerStart = 0;
                }
            }

            else
            {
                // Id dropped below threshold
                idTriggerStart = 0;
            }
        }


        // ====================================================
        // BATTERY VOLTAGE > 14.0 V
        // ====================================================

        if (Vb > VB_THRESHOLD)
        {
            if (vbTriggerStart == 0)
            {
                vbTriggerStart = now;

                Serial.println(
                    "Vb > 14.0 V : 5 sec timer started"
                );
            }

            if (now - vbTriggerStart >= STABLE_TIME)
            {
                triggerRelay = true;

                Serial.println(
                    "Vb > 14.0 V for 5 sec"
                );

                Serial.println(
                    "RELAY ON for 2 minutes"
                );

                vbTriggerStart = 0;
            }
        }

        else
        {
            // Battery voltage dropped below threshold
            vbTriggerStart = 0;
        }


        // ====================================================
        // START 2-MINUTE RELAY PERIOD
        // ====================================================

        if (triggerRelay)
        {
            relayON();

            relayOnStart = now;

            // Clear all 5-second timers
            icTriggerStart = 0;
            idTriggerStart = 0;
            vbTriggerStart = 0;
        }
    }


    // ========================================================
    // RELAY CURRENTLY ON
    // ========================================================

    else
    {
        // ----------------------------------------------------
        // Check whether 2 minutes are completed
        // ----------------------------------------------------

        if (now - relayOnStart >= RELAY_ON_TIME)
        {
            Serial.println();
            Serial.println("========================================");
            Serial.println("2 MINUTES COMPLETED");
            Serial.println("Checking conditions again...");
            Serial.println("========================================");


            bool conditionStillActive = false;


            // =================================================
            // CHARGING MODE
            // =================================================

            if (MODE == 1)
            {
                if (Ic >= IC_THRESHOLD)
                {
                    conditionStillActive = true;

                    Serial.print("Ic still >= 7.0 A : ");
                    Serial.print(Ic, 2);
                    Serial.println(" A");
                }
            }


            // =================================================
            // DISCHARGING MODE
            // =================================================

            else if (MODE == 2)
            {
                if (Id >= ID_THRESHOLD)
                {
                    conditionStillActive = true;

                    Serial.print("Id still >= 7.0 A : ");
                    Serial.print(Id, 2);
                    Serial.println(" A");
                }
            }


            // =================================================
            // BATTERY VOLTAGE
            // =================================================

            if (Vb > VB_THRESHOLD)
            {
                conditionStillActive = true;

                Serial.print("Vb still > 14.0 V : ");
                Serial.print(Vb, 2);
                Serial.println(" V");
            }


            // =================================================
            // CONDITION STILL ACTIVE
            // =================================================

            if (conditionStillActive)
            {
                Serial.println(
                    "Condition STILL ACTIVE."
                );

                Serial.println(
                    "RELAY remains ON."
                );

                Serial.println(
                    "Starting another 2-minute period."
                );

                relayOnStart = now;
            }


            // =================================================
            // ALL CONDITIONS CLEARED
            // =================================================

            else
            {
                Serial.println(
                    "All temporary conditions cleared."
                );

                Serial.println(
                    "RELAY OFF."
                );

                relayOFF();

                relayOnStart = 0;
            }
        }
    }
}


// ============================================================
// RELAY ON
// ============================================================

void relayON()
{
    digitalWrite(PIN_RELAY, LOW);

    relayOn = true;
}


// ============================================================
// RELAY OFF
// ============================================================

void relayOFF()
{
    digitalWrite(PIN_RELAY, HIGH);

    relayOn = false;
}


// ============================================================
// GENERIC ADC VOLTAGE READING
// ============================================================

float readADCVoltage(int pin)
{
    int raw = analogRead(pin);
    float voltage = ((float)raw * ADC_REF) / ADC_RES;
    return voltage;
}


// ============================================================
// CHARGING CURRENT - A0
// ============================================================

float readChargingCurrent()
{
    float voltage = readADCVoltage(PIN_IC);
    float current = voltage /  (0.1 * 8.5);
    return current;
}


// ============================================================
// DISCHARGING CURRENT - A1
// ============================================================

float readDischargingCurrent()
{
    float voltage = readADCVoltage(PIN_ID); 
    float current = voltage / (0.1 * 8.5);
    return current;
}


// ============================================================
// BATTERY VOLTAGE - A2
// ============================================================

float readBatteryVoltage()
{
    float voltage = readADCVoltage(PIN_VB);
    float batteryVoltage = voltage * BATTERY_VOLTAGE_DIVIDER;
    return batteryVoltage;
}


// ============================================================
// SUPPLY VOLTAGE - A3
// ============================================================

float readSupplyVoltage()
{
    float voltage = readADCVoltage(PIN_VS);
    float supplyVoltage = voltage * SUPPLY_VOLTAGE_DIVIDER;
    return supplyVoltage;
}