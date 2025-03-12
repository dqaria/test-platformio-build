Import('env')
import os
import shutil

# TODO: add merge
# https://github.com/eurofurence/ef28-badge/blob/main/merge-bin.py

OUTPUT_DIR = "build_output{}".format(os.path.sep)
APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"

def _get_cpp_define_value(env, define):
    define_list = [item[-1] for item in env["CPPDEFINES"] if item[0] == define]

    if define_list:
        return define_list[0]

    return None

def _create_dirs(dirs=["map", "release", "firmware"]):
    for d in dirs:
        os.makedirs(os.path.join(OUTPUT_DIR, d), exist_ok=True)

def copy_merge_bins(firmware_src, env):
    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]
    name = firmware_src.split(os.path.sep)[2]
    # flash_size = env.GetProjectOption("board_upload.flash_size")
    board = env.BoardConfig()
    f_flash = board.get("build.f_flash", "40000000L")
    flash_freq = '40m'
    if (f_flash == '80000000L'):
        flash_freq = '80m'
    mcu = board.get("build.mcu", "esp32")
    firmware_dst = "{}{}_{}_0x0.bin".format(OUTPUT_DIR, mcu, name)
    if os.path.isfile(firmware_dst):
        os.remove(firmware_dst)
    cmd = " ".join(
        ["$PYTHONEXE", "$OBJCOPY", '--chip', mcu, 'merge_bin', '--output', firmware_dst, '--flash_mode', 'dio',
        '--flash_freq', flash_freq] + flash_images)
    env.Execute(cmd)
    return firmware_dst

def create_release(source):
    release_name = _get_cpp_define_value(env, "RELEASE_NAME")
    version = _get_cpp_define_value(env, "EGLANCE_VERSION")

    firmware_dst = copy_merge_bins(source, env)

    release_file = os.path.join(OUTPUT_DIR, "release", f"eglance_{version}_{release_name}.bin")
    print(f"Copying {firmware_dst} to {release_file}")
    shutil.copy(firmware_dst, release_file)

def bin_rename_copy(source, target, env):
    _create_dirs()
    variant = env["PIOENV"]
    builddir = os.path.join(env["PROJECT_BUILD_DIR"],  variant)
    source_map = os.path.join(builddir, env["PROGNAME"] + ".map")

    # create string with location and file names based on variant
    map_file = "{}map{}{}.map".format(OUTPUT_DIR, os.path.sep, variant)

    create_release(str(target[0]))

    # copy firmware.map to map/<variant>.map
    if os.path.isfile("firmware.map"):
        print("Found linker mapfile firmware.map")
        shutil.copy("firmware.map", map_file)
    if os.path.isfile(source_map):
        print(f"Found linker mapfile {source_map}")
        shutil.copy(source_map, map_file)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", bin_rename_copy)