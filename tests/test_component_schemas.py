"""Schema structure tests for pip8048 ESPHome component modules."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent.parent))

from esphome.const import CONF_BATTERY_VOLTAGE, CONF_BUS_VOLTAGE  # noqa: E402

from components.pip8048 import binary_sensor, sensor  # noqa: E402
from components.pip8048.const import CONF_OUTPUT_SOURCE_PRIORITY  # noqa: E402


class TestSensorTypes:
    def test_qpiri_sensors_present(self):
        assert sensor.CONF_GRID_RATING_VOLTAGE in sensor.TYPES
        assert sensor.CONF_GRID_RATING_CURRENT in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_RATING_VOLTAGE in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_RATING_FREQUENCY in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_RATING_CURRENT in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_RATING_APPARENT_POWER in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_RATING_ACTIVE_POWER in sensor.TYPES
        assert sensor.CONF_BATTERY_RATING_VOLTAGE in sensor.TYPES
        assert sensor.CONF_BATTERY_RECHARGE_VOLTAGE in sensor.TYPES
        assert sensor.CONF_BATTERY_UNDER_VOLTAGE in sensor.TYPES
        assert sensor.CONF_BATTERY_BULK_VOLTAGE in sensor.TYPES
        assert sensor.CONF_BATTERY_FLOAT_VOLTAGE in sensor.TYPES
        assert sensor.CONF_BATTERY_TYPE in sensor.TYPES
        assert sensor.CONF_CURRENT_MAX_AC_CHARGING_CURRENT in sensor.TYPES
        assert sensor.CONF_CURRENT_MAX_CHARGING_CURRENT in sensor.TYPES
        assert sensor.CONF_INPUT_VOLTAGE_RANGE in sensor.TYPES
        assert CONF_OUTPUT_SOURCE_PRIORITY in sensor.TYPES
        assert sensor.CONF_CHARGER_SOURCE_PRIORITY in sensor.TYPES
        assert sensor.CONF_PARALLEL_MAX_NUM in sensor.TYPES
        assert sensor.CONF_MACHINE_TYPE in sensor.TYPES
        assert sensor.CONF_TOPOLOGY in sensor.TYPES
        assert sensor.CONF_OUTPUT_MODE in sensor.TYPES
        assert sensor.CONF_BATTERY_REDISCHARGE_VOLTAGE in sensor.TYPES
        assert sensor.CONF_PV_OK_CONDITION_FOR_PARALLEL in sensor.TYPES
        assert sensor.CONF_PV_POWER_BALANCE in sensor.TYPES
        assert sensor.CONF_MAX_CHARGING_TIME_AT_CV_STAGE in sensor.TYPES
        assert sensor.CONF_MAX_DISCHARGING_CURRENT in sensor.TYPES

    def test_qpigs_sensors_present(self):
        assert sensor.CONF_GRID_VOLTAGE in sensor.TYPES
        assert sensor.CONF_GRID_FREQUENCY in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_VOLTAGE in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_FREQUENCY in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_APPARENT_POWER in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_ACTIVE_POWER in sensor.TYPES
        assert sensor.CONF_OUTPUT_LOAD_PERCENT in sensor.TYPES
        assert CONF_BUS_VOLTAGE in sensor.TYPES
        assert CONF_BATTERY_VOLTAGE in sensor.TYPES
        assert sensor.CONF_BATTERY_CHARGING_CURRENT in sensor.TYPES
        assert sensor.CONF_BATTERY_CAPACITY_PERCENT in sensor.TYPES
        assert sensor.CONF_INVERTER_HEAT_SINK_TEMPERATURE in sensor.TYPES
        assert sensor.CONF_PV1_INPUT_CURRENT in sensor.TYPES
        assert sensor.CONF_PV1_INPUT_VOLTAGE in sensor.TYPES
        assert sensor.CONF_BATTERY_VOLTAGE_SCC in sensor.TYPES
        assert sensor.CONF_BATTERY_DISCHARGE_CURRENT in sensor.TYPES
        assert sensor.CONF_BATTERY_VOLTAGE_OFFSET_FOR_FANS_ON in sensor.TYPES
        assert sensor.CONF_EEPROM_VERSION in sensor.TYPES
        assert sensor.CONF_PV1_CHARGING_POWER in sensor.TYPES
        assert sensor.CONF_SOLAR_FEED_TO_GRID_POWER in sensor.TYPES
        assert sensor.CONF_COUNTRY_CUSTOMIZED_REGULATION in sensor.TYPES

    def test_qpigs2_sensors_present(self):
        assert sensor.CONF_PV2_INPUT_CURRENT in sensor.TYPES
        assert sensor.CONF_PV2_INPUT_VOLTAGE in sensor.TYPES
        assert sensor.CONF_PV2_CHARGING_POWER in sensor.TYPES

    def test_qet_qlt_sensors_present(self):
        assert sensor.CONF_TOTAL_PV_GENERATED_ENERGY in sensor.TYPES
        assert sensor.CONF_TOTAL_OUTPUT_LOAD_ENERGY in sensor.TYPES

    def test_qpgs0_sensors_present(self):
        assert sensor.CONF_SERIAL_NUMBER_0 in sensor.TYPES
        assert sensor.CONF_FAULT_CODE_0 in sensor.TYPES
        assert sensor.CONF_GRID_VOLTAGE_0 in sensor.TYPES
        assert sensor.CONF_GRID_FREQUENCY_0 in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_VOLTAGE_0 in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_FREQUENCY_0 in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_APPARENT_POWER_0 in sensor.TYPES
        assert sensor.CONF_AC_OUTPUT_ACTIVE_POWER_0 in sensor.TYPES
        assert sensor.CONF_LOAD_PERCENT_0 in sensor.TYPES
        assert sensor.CONF_BATTERY_VOLTAGE_0 in sensor.TYPES
        assert sensor.CONF_BATTERY_CHARGING_CURRENT_0 in sensor.TYPES
        assert sensor.CONF_BATTERY_CAPACITY_0 in sensor.TYPES
        assert sensor.CONF_PV1_INPUT_VOLTAGE_0 in sensor.TYPES
        assert sensor.CONF_TOTAL_CHARGING_CURRENT_0 in sensor.TYPES
        assert sensor.CONF_TOTAL_AC_OUTPUT_APPARENT_POWER_0 in sensor.TYPES
        assert sensor.CONF_TOTAL_OUTPUT_ACTIVE_POWER_0 in sensor.TYPES
        assert sensor.CONF_TOTAL_AC_OUTPUT_PERCENTAGE_0 in sensor.TYPES
        assert sensor.CONF_INVERTER_STATUS_BATTERY_0 in sensor.TYPES
        assert sensor.CONF_OUTPUT_MODE_0 in sensor.TYPES
        assert sensor.CONF_CHARGER_SOURCE_PRIORITY_0 in sensor.TYPES
        assert sensor.CONF_MAX_CHARGER_CURRENT_0 in sensor.TYPES
        assert sensor.CONF_MAX_CHARGER_RANGE_0 in sensor.TYPES
        assert sensor.CONF_MAX_AC_CHARGER_CURRENT_0 in sensor.TYPES
        assert sensor.CONF_PV1_INPUT_CURRENT_0 in sensor.TYPES
        assert sensor.CONF_BATTERY_DISCHARGE_CURRENT_0 in sensor.TYPES
        assert sensor.CONF_PV2_INPUT_VOLTAGE_0 in sensor.TYPES
        assert sensor.CONF_PV2_INPUT_CURRENT_0 in sensor.TYPES

    def test_q1_sensors_present(self):
        assert sensor.CONF_TIME_UNTIL_ABSORB_CHARGING in sensor.TYPES
        assert sensor.CONF_TIME_UNTIL_FLOAT_CHARGING in sensor.TYPES
        assert sensor.CONF_CHARGE_AVERAGE_CURRENT in sensor.TYPES
        assert sensor.CONF_SCC_PWM_TEMPERATURE in sensor.TYPES
        assert sensor.CONF_INVERTER_TEMPERATURE in sensor.TYPES
        assert sensor.CONF_BATTERY_TEMPERATURE in sensor.TYPES
        assert sensor.CONF_TRANSFORMER_TEMPERATURE in sensor.TYPES
        assert sensor.CONF_FAN_PWM_SPEED in sensor.TYPES
        assert sensor.CONF_SCC_CHARGE_POWER in sensor.TYPES
        assert sensor.CONF_SYNC_FREQUENCY in sensor.TYPES

    def test_sensor_types_count(self):
        assert len(sensor.TYPES) == 90


class TestBinarySensorTypes:
    def test_qpigs_binary_sensors_present(self):
        assert binary_sensor.CONF_ADD_SBU_PRIORITY_VERSION in binary_sensor.TYPES
        assert binary_sensor.CONF_CONFIGURATION_STATUS in binary_sensor.TYPES
        assert binary_sensor.CONF_SCC_FIRMWARE_VERSION in binary_sensor.TYPES
        assert binary_sensor.CONF_LOAD_STATUS in binary_sensor.TYPES
        assert (
            binary_sensor.CONF_BATTERY_VOLTAGE_TO_STEADY_WHILE_CHARGING
            in binary_sensor.TYPES
        )
        assert binary_sensor.CONF_CHARGING_STATUS in binary_sensor.TYPES
        assert binary_sensor.CONF_SCC_CHARGING_STATUS in binary_sensor.TYPES
        assert binary_sensor.CONF_AC_CHARGING_STATUS in binary_sensor.TYPES
        assert binary_sensor.CONF_CHARGING_TO_FLOATING_MODE in binary_sensor.TYPES
        assert binary_sensor.CONF_SWITCH_ON in binary_sensor.TYPES
        assert binary_sensor.CONF_DUSTPROOF_INSTALLED in binary_sensor.TYPES
        assert binary_sensor.CONF_SOLAR_FEED_TO_GRID_STATUS in binary_sensor.TYPES

    def test_qflag_binary_sensors_present(self):
        assert binary_sensor.CONF_SILENCE_BUZZER_OPEN_BUZZER in binary_sensor.TYPES
        assert binary_sensor.CONF_OVERLOAD_BYPASS_FUNCTION in binary_sensor.TYPES
        assert binary_sensor.CONF_LCD_ESCAPE_TO_DEFAULT in binary_sensor.TYPES
        assert binary_sensor.CONF_OVERLOAD_RESTART_FUNCTION in binary_sensor.TYPES
        assert (
            binary_sensor.CONF_OVER_TEMPERATURE_RESTART_FUNCTION in binary_sensor.TYPES
        )
        assert binary_sensor.CONF_BACKLIGHT_ON in binary_sensor.TYPES
        assert (
            binary_sensor.CONF_ALARM_ON_WHEN_PRIMARY_SOURCE_INTERRUPT
            in binary_sensor.TYPES
        )
        assert binary_sensor.CONF_FAULT_CODE_RECORD in binary_sensor.TYPES
        assert binary_sensor.CONF_POWER_SAVING in binary_sensor.TYPES

    def test_qpiws_binary_sensors_present(self):
        assert binary_sensor.CONF_WARNINGS_PRESENT in binary_sensor.TYPES
        assert binary_sensor.CONF_FAULTS_PRESENT in binary_sensor.TYPES
        assert binary_sensor.CONF_WARNING_BATTERY_WEAK in binary_sensor.TYPES

    def test_qbatcd_binary_sensors_present(self):
        assert binary_sensor.CONF_DISCHARGE_ONOFF in binary_sensor.TYPES
        assert binary_sensor.CONF_DISCHARGE_WITH_STANDBY_ONOFF in binary_sensor.TYPES
        assert binary_sensor.CONF_CHARGE_ONOFF in binary_sensor.TYPES

    def test_qpgs0_binary_sensors_present(self):
        assert binary_sensor.CONF_PARALLEL_NUM_0 in binary_sensor.TYPES
        assert binary_sensor.CONF_INVERTER_STATUS_SCC_0 in binary_sensor.TYPES
        assert binary_sensor.CONF_INVERTER_STATUS_AC_CHARGING_0 in binary_sensor.TYPES
        assert binary_sensor.CONF_INVERTER_STATUS_SCC_CHARGING_0 in binary_sensor.TYPES
        assert binary_sensor.CONF_INVERTER_STATUS_LINE_0 in binary_sensor.TYPES
        assert binary_sensor.CONF_INVERTER_STATUS_LOAD_0 in binary_sensor.TYPES
        assert binary_sensor.CONF_INVERTER_STATUS_CONFIGURATION_0 in binary_sensor.TYPES

    def test_q1_binary_sensors_present(self):
        assert binary_sensor.CONF_SCC_FLAG in binary_sensor.TYPES
        assert binary_sensor.CONF_ALLOW_SCC_ON in binary_sensor.TYPES
        assert binary_sensor.CONF_FAN_LOCK_STATUS in binary_sensor.TYPES
        assert binary_sensor.CONF_PARALLEL_WARNING in binary_sensor.TYPES

    def test_binary_sensor_types_count(self):
        assert len(binary_sensor.TYPES) == 71

    def test_binary_sensor_values_are_strings(self):
        for key in binary_sensor.TYPES:
            assert isinstance(key, str)

    def test_binary_sensor_values_unique(self):
        assert len(binary_sensor.TYPES) == len(set(binary_sensor.TYPES))


# ── pi18 component ────────────────────────────────────────────────────────────

from components.pi18 import (  # noqa: E402
    binary_sensor as pi18_bs,
    sensor as pi18_sensor,
    text_sensor as pi18_ts,  # noqa: E402
)


class TestPi18SensorTypes:
    def test_p005gs_sensors_present(self):
        assert pi18_sensor.CONF_GRID_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_GRID_FREQUENCY in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_FREQUENCY in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_APPARENT_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_ACTIVE_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_OUTPUT_LOAD_PERCENT in pi18_sensor.TYPES
        assert "battery_voltage" in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_VOLTAGE_SCC in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_VOLTAGE_SCC2 in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_DISCHARGE_CURRENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_CHARGING_CURRENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_CAPACITY_PERCENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_INVERTER_HEAT_SINK_TEMPERATURE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_MPPT1_CHARGER_TEMPERATURE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_MPPT2_CHARGER_TEMPERATURE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_PV1_INPUT_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_PV2_INPUT_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_PV1_INPUT_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_PV2_INPUT_VOLTAGE in pi18_sensor.TYPES

    def test_p007piri_sensors_present(self):
        assert pi18_sensor.CONF_GRID_RATING_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_GRID_RATING_CURRENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_RATING_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_RATING_FREQUENCY in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_RATING_CURRENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_RATING_APPARENT_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_AC_OUTPUT_RATING_ACTIVE_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_RATING_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_RECHARGE_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_REDISCHARGE_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_UNDER_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_BULK_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_FLOAT_VOLTAGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_BATTERY_TYPE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_CURRENT_MAX_AC_CHARGING_CURRENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_CURRENT_MAX_CHARGING_CURRENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_INPUT_VOLTAGE_RANGE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_OUTPUT_SOURCE_PRIORITY in pi18_sensor.TYPES
        assert pi18_sensor.CONF_CHARGER_SOURCE_PRIORITY in pi18_sensor.TYPES
        assert pi18_sensor.CONF_PARALLEL_MAX_NUM in pi18_sensor.TYPES
        assert pi18_sensor.CONF_MACHINE_TYPE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_TOPOLOGY in pi18_sensor.TYPES
        assert pi18_sensor.CONF_OUTPUT_MODE in pi18_sensor.TYPES
        assert pi18_sensor.CONF_SOLAR_POWER_PRIORITY in pi18_sensor.TYPES
        assert pi18_sensor.CONF_MPPT_STRING in pi18_sensor.TYPES

    def test_p006fws_sensor_present(self):
        assert pi18_sensor.CONF_FAULT_CODE in pi18_sensor.TYPES

    def test_p005et_sensor_present(self):
        assert pi18_sensor.CONF_TOTAL_GENERATED_ENERGY in pi18_sensor.TYPES

    def test_p006mod_sensor_present(self):
        assert pi18_sensor.CONF_DEVICE_MODE_ID in pi18_sensor.TYPES

    def test_p007pgs0_sensors_present(self):
        assert pi18_sensor.CONF_TOTAL_AC_OUTPUT_APPARENT_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_TOTAL_AC_OUTPUT_ACTIVE_POWER in pi18_sensor.TYPES
        assert pi18_sensor.CONF_TOTAL_OUTPUT_LOAD_PERCENT in pi18_sensor.TYPES
        assert pi18_sensor.CONF_TOTAL_BATTERY_CHARGING_CURRENT in pi18_sensor.TYPES

    def test_sensor_types_count(self):
        assert len(pi18_sensor.TYPES) == 52

    def test_sensor_keys_unique(self):
        assert len(pi18_sensor.TYPES) == len(set(pi18_sensor.TYPES))


class TestPi18BinarySensorTypes:
    def test_p005gs_binary_sensors_present(self):
        assert pi18_bs.CONF_SETTING_VALUE_CONFIGURATION_STATE in pi18_bs.TYPES

    def test_p007flag_binary_sensors_present(self):
        assert pi18_bs.CONF_SILENCE_BUZZER_OPEN_BUZZER in pi18_bs.TYPES
        assert pi18_bs.CONF_OVERLOAD_BYPASS_FUNCTION in pi18_bs.TYPES
        assert pi18_bs.CONF_LCD_ESCAPE_TO_DEFAULT in pi18_bs.TYPES
        assert pi18_bs.CONF_OVERLOAD_RESTART_FUNCTION in pi18_bs.TYPES
        assert pi18_bs.CONF_OVER_TEMPERATURE_RESTART_FUNCTION in pi18_bs.TYPES
        assert pi18_bs.CONF_BACKLIGHT_ON in pi18_bs.TYPES
        assert pi18_bs.CONF_ALARM_ON_WHEN_PRIMARY_SOURCE_INTERRUPT in pi18_bs.TYPES
        assert pi18_bs.CONF_FAULT_CODE_RECORD in pi18_bs.TYPES
        assert pi18_bs.CONF_POWER_SAVING in pi18_bs.TYPES

    def test_p006fws_binary_sensors_present(self):
        assert pi18_bs.CONF_WARNING_LINE_FAIL in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_OUTPUT_CIRCUIT_SHORT in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_OVER_TEMPERATURE in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_FAN_LOCK in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_BATTERY_VOLTAGE_HIGH in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_BATTERY_LOW_ALARM in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_BATTERY_UNDER_SHUTDOWN in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_OVER_LOAD in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_EEPROM_FAILED in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_POWER_LIMIT in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_PV1_VOLTAGE_HIGH in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_PV2_VOLTAGE_HIGH in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_MPPT1_OVERLOAD in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_MPPT2_OVERLOAD in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_SCC1_BATTERY_TOO_LOW_TO_CHARGE in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_SCC2_BATTERY_TOO_LOW_TO_CHARGE in pi18_bs.TYPES
        assert pi18_bs.CONF_WARNING_BATTERY_OPEN in pi18_bs.TYPES

    def test_binary_sensor_types_count(self):
        assert len(pi18_bs.TYPES) == 27

    def test_binary_sensor_values_unique(self):
        assert len(pi18_bs.TYPES) == len(set(pi18_bs.TYPES))


class TestPi18TextSensorTypes:
    def test_p005gs_text_sensors_present(self):
        assert pi18_ts.CONF_MPPT1_CHARGER_STATUS in pi18_ts.TYPES
        assert pi18_ts.CONF_MPPT2_CHARGER_STATUS in pi18_ts.TYPES
        assert pi18_ts.CONF_LOAD_CONNECTION in pi18_ts.TYPES
        assert pi18_ts.CONF_BATTERY_POWER_DIRECTION in pi18_ts.TYPES
        assert pi18_ts.CONF_DC_AC_POWER_DIRECTION in pi18_ts.TYPES
        assert pi18_ts.CONF_LINE_POWER_DIRECTION in pi18_ts.TYPES

    def test_p006mod_text_sensor_present(self):
        assert pi18_ts.CONF_DEVICE_MODE in pi18_ts.TYPES

    def test_p006fws_text_sensor_present(self):
        assert pi18_ts.CONF_FAULT_DESCRIPTION in pi18_ts.TYPES

    def test_raw_response_text_sensors_present(self):
        assert pi18_ts.CONF_LAST_P005GS in pi18_ts.TYPES
        assert pi18_ts.CONF_LAST_P007PIRI in pi18_ts.TYPES
        assert pi18_ts.CONF_LAST_P006MOD in pi18_ts.TYPES
        assert pi18_ts.CONF_LAST_P007FLAG in pi18_ts.TYPES
        assert pi18_ts.CONF_LAST_P005FWS in pi18_ts.TYPES
        assert pi18_ts.CONF_LAST_P005ET in pi18_ts.TYPES

    def test_text_sensor_types_count(self):
        assert len(pi18_ts.TYPES) == 14

    def test_text_sensor_values_unique(self):
        assert len(pi18_ts.TYPES) == len(set(pi18_ts.TYPES))


# ── pi18 switches and outputs ─────────────────────────────────────────────────

import re  # noqa: E402

import esphome.config_validation as cv  # noqa: E402
import pytest  # noqa: E402

from components.pi18 import (  # noqa: E402
    datetime as pi18_datetime,
    number as pi18_number,
    output as pi18_output,
    select as pi18_select,
    switch as pi18_switch,
)
from components.pi18.const import (  # noqa: E402
    CONF_BATTERY_BULK_VOLTAGE,
    CONF_BATTERY_FLOAT_VOLTAGE,
    CONF_POWER_SAVING,
)


def _schema_validator(schema, key):
    return next(value for marker, value in schema.schema.items() if str(marker) == key)


class TestPi18SwitchTypes:
    def test_power_saving_switch_removed(self):
        # ^S006PEI/PDI is the machine type setting (Hybrid/Off-grid) in SolarPower
        assert CONF_POWER_SAVING not in pi18_switch.TYPES

    def test_power_saving_switch_config_explains_why(self):
        validator = _schema_validator(pi18_switch.CONFIG_SCHEMA, CONF_POWER_SAVING)
        with pytest.raises(cv.Invalid, match="machine type"):
            validator({})


class TestPi18OutputTypes:
    def test_command_templates_format_the_float_level(self):
        # The level is a float; a %d conversion would read the wrong argument
        for values, command in pi18_output.TYPES.values():
            if command is None:
                continue
            assert re.findall(r"%[-+ 0#]*\d*(?:\.\d+)?([a-zA-Z])", command) == ["f"]
            sent = command % values[-1]
            assert int(sent[2:5]) == len(sent) - 5 + 3

    def test_charge_voltages_are_sent_by_the_component(self):
        # ^S015MCHGV always carries both voltages, so it has no single-value template
        assert set(pi18_output.CHARGE_VOLTAGES) == {
            CONF_BATTERY_BULK_VOLTAGE,
            CONF_BATTERY_FLOAT_VOLTAGE,
        }
        for key in pi18_output.CHARGE_VOLTAGES:
            assert pi18_output.TYPES[key][1] is None


class TestPi18OutputVoltageDefaults:
    def test_output_voltage_defaults_cover_24v_and_48v(self):
        steps = pi18_output._VOLTAGE_STEPS
        for volts in (24.0, 29.2, 48.0, 58.4):
            assert volts in steps
        assert 30.0 not in steps
        assert 29.3 not in steps


class TestPi18NumberTypes:
    def _validate(self, **keys):
        return pi18_number.CONFIG_SCHEMA({"pipsolar_id": "inverter0", **keys})

    def test_voltage_numbers_require_battery_rated_voltage(self):
        with pytest.raises(cv.Invalid, match="battery_rated_voltage"):
            self._validate(battery_float_voltage={"name": "float without rated"})

    def test_current_numbers_do_not_need_battery_rated_voltage(self):
        self._validate(current_max_charging_current={"name": "max charging"})

    def test_battery_rated_voltage_must_be_24_or_48(self):
        with pytest.raises(cv.Invalid):
            self._validate(battery_rated_voltage=12)
        self._validate(
            battery_rated_voltage=24, battery_float_voltage={"name": "float 24 V"}
        )

    def test_every_setting_has_a_key(self):
        assert list(pi18_number.TYPES) == [
            "battery_under_voltage",
            "battery_bulk_voltage",
            "battery_float_voltage",
            "battery_recharge_voltage",
            "battery_redischarge_voltage",
            "current_max_charging_current",
            "current_max_ac_charging_current",
        ]

    def test_24v_defaults_match_the_spec(self):
        assert pi18_number.VOLTAGE_RANGES[24] == {
            "battery_under_voltage": (20.4, 24.0, 0.1),
            "battery_bulk_voltage": (24.0, 29.2, 0.1),
            "battery_float_voltage": (24.0, 29.2, 0.1),
            "battery_recharge_voltage": (22.0, 25.5, 0.5),
            "battery_redischarge_voltage": (24.0, 29.0, 0.5),
        }

    def test_48v_defaults_match_the_spec(self):
        assert pi18_number.VOLTAGE_RANGES[48] == {
            "battery_under_voltage": (40.8, 48.0, 0.1),
            "battery_bulk_voltage": (48.0, 58.4, 0.1),
            "battery_float_voltage": (48.0, 58.4, 0.1),
            "battery_recharge_voltage": (44.0, 51.0, 1.0),
            "battery_redischarge_voltage": (48.0, 58.0, 1.0),
        }

    def test_current_defaults_match_the_spec(self):
        assert pi18_number.CURRENT_RANGES == {
            "current_max_charging_current": (10.0, 90.0, 10.0),
            "current_max_ac_charging_current": (2.0, 90.0, 1.0),
        }


class TestPi18DatetimeTypes:
    def test_datetime_keys_present(self):
        assert list(pi18_datetime.TYPES) == [
            "ac_charge_start_time",
            "ac_charge_end_time",
            "ac_output_start_time",
            "ac_output_end_time",
        ]

    def test_time_entity_validates(self):
        config = pi18_datetime.CONFIG_SCHEMA(
            {"pipsolar_id": "inverter0", "ac_charge_start_time": {"name": "start"}}
        )
        assert config["ac_charge_start_time"]["type"] == "TIME"


class TestPi18SelectTypes:
    def _validate(self, **keys):
        return pi18_select.CONFIG_SCHEMA({"pipsolar_id": "inverter0", **keys})

    def test_new_selects_have_built_in_maps(self):
        config = self._validate(
            battery_type={"name": "battery type"},
            lcd_sleep_time={"name": "LCD sleep"},
        )
        assert "optionsmap" not in config["battery_type"]
        battery, battery_status = pi18_select.DEFAULT_MAPS["battery_type"]
        assert battery == {
            "AGM": "^S007PBT0",
            "Flooded": "^S007PBT1",
            "User": "^S007PBT2",
        }
        assert battery_status == {"0": "AGM", "1": "Flooded", "2": "User"}
        lcd, lcd_status = pi18_select.DEFAULT_MAPS["lcd_sleep_time"]
        assert lcd == {
            "None": "^S008LST00",
            "30 s": "^S008LST01",
            "60 s": "^S008LST02",
            "5 min": "^S008LST10",
            "10 min": "^S008LST20",
        }
        assert lcd_status == {
            "0": "None",
            "1": "30 s",
            "2": "60 s",
            "10": "5 min",
            "20": "10 min",
        }

    def test_built_in_maps_can_be_replaced(self):
        config = self._validate(
            battery_type={"name": "type", "optionsmap": {"Mine": "^S007PBT2"}}
        )
        assert config["battery_type"]["optionsmap"] == {"Mine": "^S007PBT2"}

    def test_existing_selects_still_need_an_optionsmap(self):
        with pytest.raises(cv.Invalid, match="optionsmap"):
            self._validate(output_source_priority={"name": "priority"})
