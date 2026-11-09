<<<<<<< HEAD
# Battery Charge / Discharge Relay Control — Arduino UNO

An Arduino-based safety relay controller for **12 V battery charge and discharge testing**. The system monitors charging current, discharging current, and battery voltage in real time and activates a relay to disconnect the circuit when overcurrent or overvoltage conditions are detected — protecting the battery, equipment, and operator during lab testing.

Built as part of the **Energy Storage and Conversion Systems (ED5361)** coursework at **IIT Madras**, aligned with battery safety standards such as **AIS-156** and **ECE-R100**.

---

## System Block Diagram

![System Block Diagram](images/system_block_diagram.png)

## State Machine

![State Machine](images/state_machine.png)

## Pin Wiring Diagram

![Wiring Diagram](images/wiring_diagram.png)

---

## Features

- **Dual-mode operation** — switch between Charging (Mode 1) and Discharging (Mode 2) via a single constant
- **Two-tier overcurrent protection**
  - *Temporary*: threshold held for 5 seconds → relay ON for 2 minutes → re-check
  - *Immediate*: hard limit exceeded → relay latched ON permanently (requires Arduino reset)
- **Overvoltage protection** — monitors battery terminal voltage with the same two-tier logic
- **Active-low relay control** — `LOW = ON (disconnect)`, `HIGH = OFF (normal)`
- **Real-time serial monitor** — live readout of Ic, Id, Vb, Vs, relay state, and permanent-latch flag at 9600 baud
- **Configurable thresholds** — all limits are `const` variables at the top of the sketch

---

## Hardware

| Component | Purpose |
|---|---|
| Arduino UNO | Microcontroller (10-bit ADC, digital I/O) |
| 5 V Relay Module | Disconnect relay on pin D6 (active-low) |
| Current Shunt + Amplifier (×2) | Sense charging current (A0) and discharging current (A1) |
| Resistor Voltage Divider (Vb) | Scale battery voltage to 0–5 V on A2 (ratio 4.26) |
| Resistor Voltage Divider (Vs) | Scale supply voltage to 0–5 V on A3 (ratio 5.54) |
| 12 V Lead-Acid / Li-ion Battery | Device under test |
| Regulated Power Supply (BPS) | Charging source |
| Electronic Load | Discharge sink |
| Fuse | Inline protection |

---

## Pin Map

| Arduino Pin | Signal | Mode | Description |
|---|---|---|---|
| `A0` | Ic | Charging (Mode 1) | Charging current via shunt + amplifier |
| `A1` | Id | Discharging (Mode 2) | Discharging current via shunt + amplifier |
| `A2` | Vb | Both | Battery terminal voltage via voltage divider |
| `A3` | Vs | Charging (Mode 1) | Supply voltage via voltage divider |
| `D6` | Relay | Both | Relay control (LOW = ON, HIGH = OFF) |

---

## Protection Thresholds

| Parameter | Temporary Threshold | Immediate (Permanent) |
|---|---|---|
| Charging Current (Ic) | ≥ 1.0 A for 5 sec → ON 2 min | ≥ 1.5 A → Permanent ON |
| Discharging Current (Id) | ≥ 4.0 A for 5 sec → ON 2 min | ≥ 5.0 A → Permanent ON |
| Battery Voltage (Vb) | > 14.0 V for 5 sec → ON 2 min | ≥ 14.5 V → Permanent ON |

> **Note:** Thresholds are configurable — edit the `const float` values at the top of the `.ino` file. The values shown in the header comments (7.0 A / 7.5 A) are the original design values; the code uses the updated calibrated values above.

---

## Sensing Equations

```
ADC voltage:     V_adc = (raw × 5.0) / 1023

Charging current:    Ic = V_adc / (R_sh × Gain) = V_adc / (0.1 × 8.5) = V_adc / 0.85
Discharging current: Id = V_adc / (R_sh × Gain) = V_adc / (0.1 × 8.5) = V_adc / 0.85

Battery voltage:  Vb = V_adc × 4.26
Supply voltage:   Vs = V_adc × 5.54
```

---

## Relay Logic

```
IDLE
 │
 ├── threshold exceeded ──► 5-SEC TIMER
 │                              │
 │    cleared before 5s ◄───────┘
 │                              │
 │                         held 5 sec
 │                              │
 │                              ▼
 │                        RELAY ON (2 min)
 │                          │         │
 │   all clear after 2 min ◄┘         └► still active → restart 2 min
 │
 └── immediate threshold ──► PERMANENT ON (reset Arduino to clear)
```

**Temporary relay cycle:**
1. Sensor exceeds the temporary threshold
2. If held continuously for **5 seconds** → relay turns ON
3. Relay stays ON for **2 minutes**, then re-checks
4. If the condition is still active → another 2-minute period
5. If all conditions are clear → relay OFF, return to monitoring

**Permanent latch:**
- If any sensor hits the immediate threshold even once → relay latches ON
- Only way to clear: **reset the Arduino**

---

## How to Use

1. **Set the mode** — edit `const int MODE` at the top of the sketch:
   - `1` = Charging mode (monitors Ic, Vb, Vs)
   - `2` = Discharging mode (monitors Id, Vb)

2. **Adjust thresholds** — edit the `const float` values to match your battery and test setup

3. **Upload** to Arduino UNO via Arduino IDE

4. **Open Serial Monitor** at 9600 baud to see live sensor readings:
   ```
   Ic: 0.82 A | Id: 0.00 A | Vb: 12.34 V | Vs: 15.02 V | Relay: OFF
   ```

5. **Connect hardware** per the wiring diagram above

---

## Circuit Schematics

The protection circuit uses:
- **Shunt resistor + differential amplifier** for current sensing
- **Resistor voltage divider** for battery and supply voltage scaling
- **5 V relay module** with NO (normally open) contact in the power path
- **Fuse** for hard-fault backup protection

Refer to the [project report PDF](docs/) for detailed hand-drawn schematics of the overcharge and over-discharge protection circuits.

---

## Safety Standards Reference

This project aligns with battery testing safety practices from:

| Standard | Scope |
|---|---|
| **AIS-156** | Indian automotive standard for EV traction battery safety (ARAI / CMVR) |
| **ECE-R100** | UN regulation for EV energy storage system approval |
| **IEC 62133** | Safety requirements for portable Li-ion batteries |
| **UN38.3** | Transport testing for lithium batteries |

Key safety practices implemented:
- Overcurrent and overvoltage detection with configurable thresholds
- Two-tier protection (temporary + permanent latch)
- Active-low relay for fail-safe disconnection
- Real-time serial monitoring for operator awareness

---

## File Structure

```
.
├── battery_relay_control.ino    # Arduino sketch
├── README.md
├── images/
│   ├── system_block_diagram.png # System-level block diagram
│   ├── state_machine.png        # Relay control state machine
│   └── wiring_diagram.png       # Arduino pin wiring diagram
└── docs/                        # (optional) scanned schematics / report
```

---

## Built With

- **Arduino UNO** (ATmega328P)
- **Arduino IDE**
- C/C++ (Arduino framework)

---

## Author

**Ramavath Mohan Sankar**
MTech, Applied Mechanics | IIT Madras
- [LinkedIn](https://linkedin.com/in/mohan-sankar-naik-1a7928287/)
- [GitHub](https://github.com/sankarnaik725)

---

## License

This project is open-source under the [MIT License](LICENSE).
=======
# Arduino-Based-Battery-Charge-Discharge-Protection-System-Arduino-UNO-
>>>>>>> 407fb5b84934eb7c5c5e870b70009af0dea3b034
