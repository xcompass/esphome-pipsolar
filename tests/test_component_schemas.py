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
        assert len(pi18_ts.TYPES) == 15

    def test_text_sensor_values_unique(self):
        assert len(pi18_ts.TYPES) == len(set(pi18_ts.TYPES))


# ── pi18 switches and outputs ─────────────────────────────────────────────────

import asyncio  # noqa: E402
import itertools  # noqa: E402
import re  # noqa: E402
from unittest.mock import MagicMock  # noqa: E402

import esphome.config_validation as cv  # noqa: E402
import pytest  # noqa: E402

from components.pi18 import (  # noqa: E402
    button as pi18_button,
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

_entity_names = itertools.count()


def _unique_name(prefix):
    """ESPHome rejects a repeated entity name, even across separate validations."""
    return f"{prefix} {next(_entity_names)}"


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


class TestPi18NumberRanges:
    def _validate(self, **keys):
        return pi18_number.CONFIG_SCHEMA({"pipsolar_id": "inverter0", **keys})

    def _range(self, key, rated=48, **options):
        config = self._validate(
            battery_rated_voltage=rated, **{key: {"name": _unique_name(key), **options}}
        )
        return pi18_number._range(config, key)

    def test_default_range_follows_the_battery_voltage(self):
        assert self._range("battery_bulk_voltage", rated=24) == (24.0, 29.2, 0.1)
        assert self._range("battery_bulk_voltage", rated=48) == (48.0, 58.4, 0.1)
        assert self._range("battery_recharge_voltage", rated=24) == (22.0, 25.5, 0.5)
        assert self._range("battery_recharge_voltage", rated=48) == (44.0, 51.0, 1.0)

    def test_currents_ignore_the_battery_voltage(self):
        assert self._range("current_max_charging_current", rated=24) == (
            10.0,
            90.0,
            10.0,
        )
        assert self._range("current_max_ac_charging_current", rated=48) == (
            2.0,
            90.0,
            1.0,
        )

    def test_user_values_override_the_defaults_one_by_one(self):
        assert self._range("battery_float_voltage", min_value=50.0) == (50.0, 58.4, 0.1)
        assert self._range("battery_float_voltage", max_value=56.0) == (48.0, 56.0, 0.1)
        assert self._range("battery_float_voltage", step=0.2) == (48.0, 58.4, 0.2)
        assert self._range(
            "battery_float_voltage", min_value=50.0, max_value=56.0, step=0.5
        ) == (50.0, 56.0, 0.5)

    def test_min_value_must_be_below_max_value(self):
        for options in ({"min_value": 58.4}, {"max_value": 48.0}, {"min_value": 60.0}):
            with pytest.raises(cv.Invalid, match="min_value") as error:
                self._validate(
                    battery_rated_voltage=48,
                    battery_bulk_voltage={"name": _unique_name("bulk"), **options},
                )
            assert error.value.path == ["battery_bulk_voltage"]

    def test_step_must_fit_in_the_range(self):
        with pytest.raises(cv.Invalid, match="step") as error:
            self._validate(
                current_max_charging_current={
                    "name": _unique_name("current"),
                    "min_value": 10.0,
                    "max_value": 20.0,
                    "step": 20.0,
                }
            )
        assert error.value.path == ["current_max_charging_current"]
        self._validate(
            current_max_charging_current={
                "name": _unique_name("current"),
                "min_value": 10.0,
                "max_value": 20.0,
                "step": 10.0,
            }
        )

    def test_voltage_step_must_be_a_multiple_of_a_tenth_of_a_volt(self):
        with pytest.raises(cv.Invalid, match="resolution"):
            self._validate(
                battery_rated_voltage=48,
                battery_float_voltage={"name": _unique_name("float"), "step": 0.05},
            )
        for step in (0.1, 0.3, 0.5, 1.0):  # float error must not matter
            self._validate(
                battery_rated_voltage=48,
                battery_float_voltage={"name": _unique_name("float"), "step": step},
            )

    def test_current_step_must_be_whole_amperes(self):
        with pytest.raises(cv.Invalid, match="resolution") as error:
            self._validate(
                current_max_ac_charging_current={
                    "name": _unique_name("ac"),
                    "step": 0.5,
                }
            )
        assert error.value.path == ["current_max_ac_charging_current"]

    def test_back_to_battery_limits_apply_to_the_non_zero_range(self):
        with pytest.raises(cv.Invalid, match="min_value"):
            self._validate(
                battery_rated_voltage=48,
                battery_redischarge_voltage={
                    "name": _unique_name("b"),
                    "min_value": 58.0,
                },
            )
        with pytest.raises(cv.Invalid, match="step"):
            self._validate(
                battery_rated_voltage=48,
                battery_redischarge_voltage={
                    "name": _unique_name("b"),
                    "min_value": 50.0,
                    "max_value": 52.0,
                    "step": 5.0,
                },
            )

    def test_back_to_battery_is_a_box_by_default(self):
        config = self._validate(
            battery_rated_voltage=48,
            battery_redischarge_voltage={"name": _unique_name("back to battery")},
            battery_recharge_voltage={"name": _unique_name("back to utility")},
        )
        assert config["battery_redischarge_voltage"]["mode"] == "BOX"
        assert config["battery_recharge_voltage"]["mode"] == "AUTO"
        config = self._validate(
            battery_rated_voltage=48,
            battery_redischarge_voltage={"name": _unique_name("b"), "mode": "slider"},
        )
        assert config["battery_redischarge_voltage"]["mode"] == "SLIDER"

    def _generate(self, monkeypatch, key, **options):
        config = self._validate(
            battery_rated_voltage=24, **{key: {"name": _unique_name(key), **options}}
        )
        created = []

        async def new_number(conf, **kwargs):
            var = MagicMock()
            created.append((var, kwargs))
            return var

        async def get_variable(_):
            return MagicMock()

        monkeypatch.setattr(pi18_number.number, "new_number", new_number)
        monkeypatch.setattr(pi18_number.cg, "get_variable", get_variable)
        monkeypatch.setattr(pi18_number.cg, "add", lambda _: None)
        asyncio.run(pi18_number.to_code(config))
        assert len(created) == 1
        return created[0]

    def test_back_to_battery_accepts_zero_below_the_minimum(self, monkeypatch):
        var, kwargs = self._generate(monkeypatch, "battery_redischarge_voltage")
        assert kwargs == {"min_value": 0.0, "max_value": 29.0, "step": 0.5}
        var.set_min_nonzero_value.assert_called_once_with(24.0)

    def test_back_to_battery_user_limits_become_the_non_zero_range(self, monkeypatch):
        var, kwargs = self._generate(
            monkeypatch, "battery_redischarge_voltage", min_value=25.0, max_value=28.0
        )
        assert kwargs["min_value"] == 0.0
        assert kwargs["max_value"] == 28.0
        var.set_min_nonzero_value.assert_called_once_with(25.0)

    def test_other_numbers_use_their_range_directly(self, monkeypatch):
        var, kwargs = self._generate(monkeypatch, "battery_recharge_voltage")
        assert kwargs == {"min_value": 22.0, "max_value": 25.5, "step": 0.5}
        var.set_min_nonzero_value.assert_not_called()


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
        config = self._validate(battery_type={"name": "battery type"})
        assert "optionsmap" not in config["battery_type"]
        battery, battery_status = pi18_select.DEFAULT_MAPS["battery_type"]
        assert battery == {
            "AGM": "^S007PBT0",
            "Flooded": "^S007PBT1",
            "User": "^S007PBT2",
        }
        assert battery_status == {"0": "AGM", "1": "Flooded", "2": "User"}

    def test_built_in_maps_can_be_replaced(self):
        config = self._validate(
            battery_type={
                "name": "type",
                "optionsmap": {"Mine": "^S007PBT2"},
                "statusmap": {"2": "Mine"},
            }
        )
        assert config["battery_type"]["optionsmap"] == {"Mine": "^S007PBT2"}

    def test_optionsmap_and_statusmap_come_together_on_the_new_selects(self):
        key = "battery_type"
        for only in (
            {"optionsmap": {"Mine": "^S007PBT2"}},
            {"statusmap": {"2": "Mine"}},
        ):
            with pytest.raises(cv.Invalid, match="together") as error:
                self._validate(**{key: {"name": _unique_name("select"), **only}})
            assert error.value.path == [key]

    def test_existing_selects_still_need_an_optionsmap(self):
        with pytest.raises(cv.Invalid, match="optionsmap"):
            self._validate(output_source_priority={"name": "priority"})


class TestPi18Clock:
    def test_inverter_time_text_sensor_present(self):
        assert "inverter_time" in pi18_ts.TYPES
        config = pi18_ts.CONFIG_SCHEMA(
            {"pipsolar_id": "inverter0", "inverter_time": {"name": "clock"}}
        )
        assert config["inverter_time"]["name"] == "clock"

    def test_sync_clock_requires_time_id(self):
        with pytest.raises(cv.Invalid, match="time_id"):
            pi18_button.CONFIG_SCHEMA(
                {"pipsolar_id": "inverter0", "sync_clock": {"name": "sync"}}
            )

    def test_sync_clock_validates_with_time_id(self):
        config = pi18_button.CONFIG_SCHEMA(
            {
                "pipsolar_id": "inverter0",
                "sync_clock": {"name": "sync", "time_id": "ha_time"},
            }
        )
        assert config["sync_clock"]["time_id"].id == "ha_time"
