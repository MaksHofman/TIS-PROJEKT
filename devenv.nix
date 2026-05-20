{ pkgs, lib, config, inputs, ... }:

{
    # PlatformIO
    languages.cplusplus.enable = true;

    packages = with pkgs; [
        # general
        git

        # sensor
        tio
        platformio
    ];

    env = {
        # TODO: use $PWD
        PLATFORMIO_CORE_DIR = ".platformio";
    };
}
