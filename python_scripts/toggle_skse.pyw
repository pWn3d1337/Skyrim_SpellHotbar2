import os
import time
from pathlib import Path
import shutil

# this script switches between debug and release skse build when run from game root directory
version = "1_6_640"
release_folder = "SKSE_release"
debug_folder = "SKSE_debug"


def folders_ok():
    return Path(f"./{debug_folder}").exists() and \
        Path(f"./{release_folder}").exists()


def is_debug_skse():
    return Path(f"./skse64_{version}.pdb").exists() and \
        Path(f"./skse64_{version}.dll").exists() and \
        Path(f"./skse64_loader.exe").exists() and \
        Path(f"./skse64_loader.pdb").exists()


def is_release_skse():
    return not Path(f"./skse64_{version}.pdb").exists() and \
        Path(f"./skse64_{version}.dll").exists() and \
        Path(f"./skse64_loader.exe").exists() and \
        not Path(f"./skse64_loader.pdb").exists()


if __name__ == "__main__":
    # check current skse type
    os.chdir(Path(__file__).parent)
    if folders_ok():
        if is_debug_skse():
            print("Debug: switching to Release")
            shutil.move(f"./skse64_{version}.dll", f"./{debug_folder}/skse64_{version}.dll")
            shutil.move(f"./skse64_{version}.pdb", f"./{debug_folder}/skse64_{version}.pdb")
            shutil.move(f"./skse64_loader.exe", f"./{debug_folder}/skse64_loader.exe")
            shutil.move(f"./skse64_loader.pdb", f"./{debug_folder}/skse64_loader.pdb")
            time.sleep(0.1)
            shutil.move(f"./{release_folder}/skse64_loader.exe", f"./skse64_loader.exe")
            shutil.move(f"./{release_folder}/skse64_{version}.dll", f"./skse64_{version}.dll")

        elif is_release_skse():
            print("Release: switching to Debug")
            shutil.move(f"./skse64_{version}.dll", f"./{release_folder}/skse64_{version}.dll")
            shutil.move(f"./skse64_loader.exe", f"./{release_folder}/skse64_loader.exe")
            time.sleep(0.1)
            shutil.move(f"./{debug_folder}/skse64_loader.exe", f"./skse64_loader.exe")
            shutil.move(f"./{debug_folder}/skse64_loader.pdb", f"./skse64_loader.pdb")
            shutil.move(f"./{debug_folder}/skse64_{version}.dll", f"./skse64_{version}.dll")
            shutil.move(f"./{debug_folder}/skse64_{version}.pdb", f"./skse64_{version}.pdb")
    else:
        print("Folders NOT ok!, Waiting 10s")
        print(os.getcwd())
        time.sleep(10)
