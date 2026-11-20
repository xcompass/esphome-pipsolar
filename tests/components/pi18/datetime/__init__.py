import esphome.codegen as cg


def override_manifest(manifest):
    # USE_DATETIME_TIME is only defined by a real build that configures a time entity.
    # The unit tests have none, so this platform's replacement to_code defines it, and
    # pipsolar_time.cpp and the datetime time sources get compiled.
    async def to_code_testing(config):
        cg.add_define("USE_DATETIME_TIME")
        # Application keeps a registry of the time entities, sized by this
        cg.add_define("ESPHOME_ENTITY_TIME_COUNT", 1)

    manifest.to_code = to_code_testing
