# ----------------------------------------------------------------------------
#  Both TFT_eSPI (SPIClass spi) and SmartRC-CC1101 (bool spi) define a global
#  symbol named `spi`, which breaks linking. This pre-script renames the
#  SmartRC one at compile time - only for that library's sources - so neither
#  library has to be patched by hand.
# ----------------------------------------------------------------------------
Import("env")


def rename_smartrc_spi(env, node):
    if "SmartRC-CC1101-Driver-Lib" in node.get_path():
        return env.Object(
            node,
            CCFLAGS=list(env.get("CCFLAGS", [])) + ["-Dspi=cc1101_spi_custom_pins"],
        )
    return node


env.AddBuildMiddleware(rename_smartrc_spi)
