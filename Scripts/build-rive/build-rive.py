import os
import shutil
import argparse
import subprocess
import sys
import stat
from pathlib import Path
from enum import Enum

parser = argparse.ArgumentParser(description="""
Build and copy rive C++ runtime library and copy it and it headers into the correct location for unreal

The following steps are performed:
On Windows:
    * Build Rive Windows Binary debug and release
    * Copy binaries to Plugins/Rive/Source/ThirdParty/RiveLibrary/Libraries/Win64
    * Build Android Binary debug and release
    * Copy binaries to Plugins/Rive/Source/ThirdParty/RiveLibrary/Libraries/Android
On Mac:
    * Build Rive Mac Binary debug and release
    * Copy binaries to Plugins/Rive/Source/ThirdParty/RiveLibrary/Libraries/Mac
    * Build iOS Binary debug and release
    * Copy binaries to Plugins/Rive/Source/ThirdParty/RiveLibrary/Libraries/iOS
Finally on all platforms:
    * Copy includes from rive_runtime_path/include to Plugins/Rive/Source/ThirdParty/RiveLibrary/Includes
    * Copy includes from rive_runtime_path/renderer/include to Plugins/Rive/Source/ThirdParty/RiveLibrary/Includes
    * Copy generated exports from build_path/include/generated/shaders from to Plugins/Rive/Source/ThirdParty/RiveLibrary/Includes
    * Copy generated shaders build_path/include/generated/shaders from to Plugins/Rive/Shaders/Private/Rive/Generated
    * Copy unreal static shaders from rive_runtime_path/renderer/src/unreal to Plugins/Rive/Shaders/Private/Rive
""")

parser.add_argument("-p", "--rive_runtime_path", type=str, help="path to rive runtime root folder, if not specified default path is used")
parser.add_argument("-t", "--build_rive_tests", action='store_true',  default=False, help="If set, gms, goldens and player will be built and copied as well")
parser.add_argument("-r", "--raw_shaders", action='store_true', default=False, help="If set, --raw_shaders will be passed to the premake file for building")

PREFERED_MSVC = "14.38"

args = parser.parse_args()

script_directory = os.path.dirname(os.path.abspath(__file__))
gms_directory = os.path.abspath(os.path.join(script_directory, "../../../GM"))

targets = [
    'rive',
    'rive_decoders',
    'rive_harfbuzz',
    'rive_pls_renderer',
    'rive_sheenbidi',
    'rive_yoga',
    'libpng',
    'libjpeg',
    'libwebp',
    'zlib'
]

test_targets = [
    'gms',
    "goldens",
    'tools_common'
]

class CompileResult(Enum):
    SUCCESS=1 # compile finished succesfully
    FAILED=2 # compile failed because of some error
    NEEDS_CLEAN=3 # compile failed because premake arguments did not match, we should try again with clean
    NOT_STARTED=4 # no compile attempted

class CompilePass(object):
    """ 
    Represents something that needs to be compiled 
    @rive_runtime_path, the absolute path to the dir containing the rive-runtime repo
    @root_dir the dir to compile from, should contain a premake5.lua file
    @out_dir the directory we want the output to go
    @is_release if this is a release build
    @build_type this is how the projects get compiled, like ios, android etc..
    @kwargs and extra arguments to pass to build_rive.sh
    """
    def __init__(self, rive_runtime_path:str, root_dir:str, out_dir:str, is_release:bool, build_type:str, targets:list, **kwargs):
        # for debugging, basically adds a var for each key passed in
        self.__dict__.update(kwargs)
        self.out_dir = out_dir
        self.root_dir = root_dir
        # this does not need to be a member var, it is for ease of debugging
        self.targets = targets
        # used to check if this compile was succesful
        self.success = CompileResult.NOT_STARTED
        # extra args passed in to be passed to build_script
        aux_command_args = ["--"+str(key)  + "=" + str(value) for key, value in kwargs.items()]
        # build script path generation
        build_script = os.path.join(rive_runtime_path, "build", "build_rive")
        if sys.platform.startswith('win32'):
            build_script = f"powershell -File {build_script}.ps1"
        else:
            build_script = f"{build_script}.sh"
        # default command to use, build_rive.sh includes --with_rive_layout and --with_rive_text automatically
        if build_type != '' and build_type is not None:
            command_args = build_type.split() + ["\"--with_rive_audio=external\"", "\"--for_unreal\""]
        else:
            command_args = ["\"--with_rive_audio=external\"", "\"--for_unreal\""]
        if is_release:
            command_args.append("release")
        if args.raw_shaders:
            command_args.append("\"--raw_shaders\"")

        tools_version = self.get_tools_version()
        if tools_version is not None:
            command_args.extend([f"--toolsversion={tools_version}"])

        command_args.extend(aux_command_args)

        if self.targets is not None:
            # if we have targets, append them to the command. otherwise it just builds everything
            command_args.append("\"--\"")
            command_args.extend(self.targets)

        self.command = build_script.split(" ") + command_args
        # command used when passing clean to build_rive
        self.clean_command = build_script.split(" ") + ["clean"] + command_args

    def get_tools_version(self) -> str:
        if sys.platform.startswith('win32'):
            # MSVC Tools are in subfolders with names that match the version they are. this will make a list of those folder names
            msvc_root = os.environ['MSVC_ROOT'] if "MSVC_ROOT" in os.environ.keys() else "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC"
            versions = os.listdir(msvc_root)
            prefered_versions = [version for version in versions if PREFERED_MSVC in version]
            #  if we have versions that match our prefered major and minor version sort by revision and return highest
            if len(prefered_versions) > 0:
                sorted_versions = sorted(prefered_versions, key=lambda x:int(x.split('.')[-1]), reverse=True)
                return sorted_versions[0]
            # sort the folders to get the latest version first and return it
            versions.sort()
            return versions[0]
        
        return None

    def try_build(self) -> bool:
        """Attempt to build, if fails, tries to build again with clean. returns true if succesful, false otherwise"""
        # move to our root build dir
        os.chdir(self.root_dir)
        #set RIVE_OUT otherwise the ouput dir won't match
        os.environ["RIVE_OUT"]=self.out_dir
        command = self.command
        # do we have an out dir? if so make sure '.rive_premake_args' exsits. If not delete the out dir
        if os.path.exists(self.out_dir):
            if not os.path.exists(os.path.join(self.out_dir, ".rive_premake_args")):
                shutil.rmtree(self.out_dir)

        # try the command
        self.success = execute_command(command)

        if self.success == CompileResult.NEEDS_CLEAN:
            # try again with clean
            self.success = execute_command(self.clean_command)

        return self.success == CompileResult.SUCCESS

def execute_command(cmd) -> bool:
    """attempt to run cmd in another process, piping output to this process stdout. returns a CompileResult"""
    if sys.platform.startswith('darwin'):
        cmd = " ".join(cmd)
    print_green(f'Executing {cmd}')
    process = subprocess.Popen(cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        universal_newlines=True
    )

    ret = CompileResult.SUCCESS

    for output in iter(process.stdout.readline, ""):
        if output:
            if 'error: premake5 arguments for current build do not match previous arguments' in output or "If you wish to overwrite the existing build, please use 'clean'" in output:
                ret = CompileResult.NEEDS_CLEAN
            elif ('error:' in output.lower() or 'error :' in output.lower()) and 'Structured output' not in output and '0 Error' not in output:
                print_red(f'{output.strip()}')
                if ret == CompileResult.SUCCESS:
                    # always prefer clean, so only set failed if we have not set clean
                    ret = CompileResult.FAILED
            else:
                print(output.strip())

    process.wait()
    return ret


def print_green(text):
    print(f'\033[32m{text}\033[0m')


def print_red(text):
    print(f'\033[91m{text}\033[0m')
        

def main(rive_runtime_path):
    print_green(f"using runtime location {rive_runtime_path}")
    
    if sys.platform.startswith('darwin'):
        os.environ["MACOSX_DEPLOYMENT_TARGET"] = '11.0'
    
        if not do_mac(rive_runtime_path, True) or not do_mac(rive_runtime_path, False):
            return
        
        if not do_ios(rive_runtime_path, True) or not do_ios(rive_runtime_path, False):
            return
        
    elif sys.platform.startswith('win32'):
        # add rive build dir to path if it's not already there
        build_path = os.path.join(rive_runtime_path, "build")
        if build_path not in os.environ["PATH"]:
            os.environ["PATH"] = os.environ["PATH"] + ";" + build_path
        if not do_windows(rive_runtime_path, True) or not do_windows(rive_runtime_path, False):
            return
        
        os.chdir(rive_runtime_path)

        if not do_android(rive_runtime_path, True) or not do_android(rive_runtime_path, False):
           return
        
        os.chdir(rive_runtime_path)

        
    else:
        print_red("Unsupported platform")
        return

    copy_includes(rive_runtime_path)

def do_android(rive_runtime_path, release):
    should_build_tests = args.build_rive_tests and release

    out_dir = os.path.join('..', 'out', 'android', 'release' if release else 'debug')
    root_dir = os.path.join(rive_runtime_path, 'tests' if should_build_tests else 'renderer' )

    # arch=arm64 is chosen for you in build_rive.sh because type is android
    android_pass = CompilePass(rive_runtime_path, root_dir, out_dir, release, "android", targets + test_targets if should_build_tests else targets, android_api="31")
    if not android_pass.try_build():
        print_red("Exiting due to errors...")
        return False
    
    os.chdir(out_dir)
    print_green(f'Built in {os.getcwd()}')
        
    rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

    print_green('Copying Android libs')
    copy_files(os.getcwd(), os.path.join(rive_libraries_path, 'Android'), ".a", release)
    if should_build_tests:
        gm_libraries_path = os.path.join(gms_directory, 'Source', 'ThirdParty', 'GMLibrary', 'Libraries', "Android")
        copy_files(os.getcwd(), gm_libraries_path, ".a", True, False, ['libgms', 'libgoldens', 'libtools_common'])
    return True


def do_windows(rive_runtime_path, release):

    should_build_tests = args.build_rive_tests and release

    out_dir = os.path.join('..', 'out', 'windows', 'release' if release else 'debug')

    root_dir = os.path.join(rive_runtime_path, 'tests' if should_build_tests else 'renderer' )
    
    windows_pass = CompilePass(rive_runtime_path, root_dir, out_dir, release, "", targets + test_targets if should_build_tests else targets, os="windows")
    if not windows_pass.try_build():
        print_red("Exiting due to errors...")
        return False
    os.chdir(out_dir)
    print_green(f'Built in {os.getcwd()}')
    # end build

    rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

    print_green('Copying Windows')
    copy_files(os.getcwd(), os.path.join(rive_libraries_path, 'Win64'), ".lib", release)
    if should_build_tests:
        gm_libraries_path = os.path.join(gms_directory, 'Source', 'ThirdParty', 'GMLibrary', 'Libraries', "x64", "Release")
        copy_files(os.getcwd(), gm_libraries_path, ".lib", True, False, ['gms', 'goldens', 'tools_common'])
    
    return True

def do_ios(rive_runtime_path, release):
    should_build_tests = args.build_rive_tests and release

    root_dir = os.path.join(rive_runtime_path, 'tests' if should_build_tests else 'renderer' )
    build_dirs = {}

    print_green('Building iOS')
    out_dir = os.path.join('..', 'out', 'ios', 'release' if release else 'debug')
    system_pass = CompilePass(rive_runtime_path, root_dir, out_dir, release, "ios", targets + test_targets if should_build_tests else targets, variant="system")
    if not system_pass.try_build():
        print_red("Exiting due to errors...")
        return False

    os.chdir(out_dir)
    build_dirs['ios'] = os.getcwd()

    print_green('Building iOS Simulator')
    out_dir = os.path.join('..', 'out', 'ios_sim', 'release' if release else 'debug')
    os.chdir(os.path.join(rive_runtime_path, 'tests' if should_build_tests else 'renderer' ))
    emulator_pass = CompilePass(rive_runtime_path, root_dir, out_dir, release, "ios", targets + test_targets if should_build_tests else targets, variant="emulator")
    if not emulator_pass.try_build():
        print_red("Exiting due to errors...")
        return False

    os.chdir(out_dir)
    build_dirs['ios_sim'] = os.getcwd()

    # we need to rename all ios simulator to .sim.a
    for root, _, files in os.walk(build_dirs['ios_sim']):
        for file in files:
            if file.endswith('.a') and not '.sim.' in file:
                old_file = os.path.join(root, file)
                new_file = f'{file[:-2]}.sim.a'
                new_file = os.path.join(root, new_file)
                if os.path.exists(new_file):
                    os.replace(old_file, new_file)
                else:
                    os.rename(old_file, new_file)

    os.chdir(rive_runtime_path)
    print_green(f'Built in {build_dirs}')

    rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

    print_green('Copying iOS and iOS simulator')
    copy_files(build_dirs['ios'], os.path.join(rive_libraries_path, 'IOS'), ".a", release)
    copy_files(build_dirs['ios_sim'], os.path.join(rive_libraries_path, 'IOS'), ".sim.a", release)
    if should_build_tests:
        gm_libraries_path = os.path.join(gms_directory, 'Source', 'ThirdParty', 'GMLibrary', 'Libraries', "iOS")
        copy_files(build_dirs['ios'], gm_libraries_path, ".a", True, False, ['libgms', 'libgoldens', 'libtools_common'])
        copy_files(build_dirs['ios_sim'], gm_libraries_path, ".sim.a", True, False, ['libgms', 'libgoldens', 'libtools_common'])
    
    return True


def do_mac(rive_runtime_path, release):
    should_build_tests = args.build_rive_tests and release

    build_dirs = {}

    # we currently don't use fat libs, otherwise we could use this for macOS
    # build_fat = False
    # if build_fat:
    #     execute_command(f'{command} --arch=universal --out=out/universal')
    #     os.chdir(os.path.join('..', 'out', 'universal'))
    #     build_dirs['universal'] = os.getcwd()
    #     execute_command('make')
    # else:


    print_green('Building macOS x64')
    out_dir = os.path.join('..', 'out', 'mac', 'x64', 'release' if release else 'debug')
    root_dir = os.path.join(rive_runtime_path, 'tests' if should_build_tests else 'renderer' )
    x64_pass = CompilePass(rive_runtime_path, root_dir, out_dir, release, "", targets + test_targets if should_build_tests else targets, os="macosx", arch="x64")
    if not x64_pass.try_build():
        print_red("Exiting due to errors...")
        return False
    os.chdir(out_dir)
    build_dirs['mac_x64'] = os.getcwd()
    print_green('Building macOS arm64')

    out_dir = os.path.join('..', 'out', 'mac', 'arm64', 'release' if release else 'debug')
    os.chdir(os.path.join(rive_runtime_path, 'tests' if should_build_tests else 'renderer' ))
    arm64_pass = CompilePass(rive_runtime_path, root_dir, out_dir, release, "", targets + test_targets if should_build_tests else targets, os="macosx", arch="arm64")
    if not arm64_pass.try_build():
        print_red("Exiting due to errors...")
        return False
    os.chdir(out_dir)
    build_dirs['mac_arm64'] = os.getcwd()

    os.chdir(rive_runtime_path)
    print_green(f'Built in {build_dirs}')

    rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

    print_green('Copying macOS x64')
    copy_files(build_dirs['mac_x64'], os.path.join(rive_libraries_path, 'Mac', 'Intel'), ".a", release)
    copy_files(build_dirs['mac_arm64'], os.path.join(rive_libraries_path, 'Mac', 'Mac'), ".a", release)
    if should_build_tests:
        gm_libraries_path = os.path.join(gms_directory, 'Source', 'ThirdParty', 'GMLibrary', 'Libraries', "Mac")
        copy_files(build_dirs['mac_arm64'], os.path.join(gm_libraries_path, "arm"), ".a", True, False, ['libgms', 'libgoldens', 'libtools_common'])
        copy_files(build_dirs['mac_x64'], os.path.join(gm_libraries_path, "x64"), ".a", True, False, ['libgms', 'libgoldens', 'libtools_common'])
    
    return True


def copy_files(src, dst, extension, is_release, should_rename = True, files_to_copy = None):
     # Ensure the source directory exists
    if not os.path.exists(src):
        print_red(f"The source directory {src} does not exist.")
        return
    
    dst = os.path.abspath(dst)
    src = os.path.abspath(src)

    os.makedirs(dst, exist_ok=True)

    files = os.listdir(src)

    if files_to_copy is not None:
        files_to_copy = [f + extension for f in files_to_copy]
    else:
        files_to_copy = [f for f in files if f.endswith(extension)]
    for file_name in files_to_copy:
        src_path = os.path.join(src, file_name)
        # ensure all libs are prefixed with "rive_" on non-darwin / apple platforms
        if not sys.platform.startswith("darwin") and not file_name.startswith("rive") and not 'Android' in dst and should_rename:
            file_name = f'rive_{file_name}'

        dest_path = os.path.join(dst, file_name if is_release else file_name.replace(extension, f'_d{extension}'))
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        shutil.copy2(src_path, dest_path)

def remove_readonly(func, path, exc_info):
    os.chmod(path, stat.S_IWRITE)
    func(path)

def copy_includes(rive_runtime_path):
    print_green('Copying rive includes...')
    rive_constant_include_src = os.path.join(rive_runtime_path, "renderer", "src", "shaders", "constants.glsl")
    rive_includes_path = os.path.join(rive_runtime_path, 'include')
    rive_pls_includes_path = os.path.join(rive_runtime_path, 'renderer', 'include')
    rive_shaders_includes_path = os.path.join(rive_runtime_path, 'out', 'windows' if sys.platform.startswith('win32') else 'mac/x64', 'release', "include")
    rive_shader_source_path = os.path.join(rive_runtime_path, 'renderer', 'src', 'shaders', "unreal")
    rive_decoders_includes_path = os.path.join(rive_runtime_path, 'decoders', 'include')
    target_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Includes')
    shaders_target_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Includes', "rive")
    rive_shader_source_target_path = os.path.join(script_directory, '..', '..', 'Shaders', 'Private', 'Rive')
    rive_constants_include_target_path = os.path.join(shaders_target_path, "shaders", "constants.glsl")
    generated_shader_path = os.path.join(script_directory, '..', '..', 'Shaders', 'Private', 'Rive', 'Generated')
    generated_shader_ush_path = os.path.join(rive_shaders_includes_path, "generated", "shaders")

    if os.path.exists(target_path):
        shutil.rmtree(target_path, onerror=remove_readonly)

    if os.path.exists(generated_shader_path):
        shutil.rmtree(generated_shader_path, onerror=remove_readonly)

    # delete and re create generated shader path to clear it
    os.makedirs(generated_shader_path)
    os.makedirs(os.path.dirname(rive_constants_include_target_path), exist_ok=True)
    shutil.copyfile(rive_constant_include_src, rive_constants_include_target_path)

    shutil.copytree(rive_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_pls_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_decoders_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_shader_source_path, rive_shader_source_target_path, dirs_exist_ok=True)

    shutil.copytree(rive_shaders_includes_path, shaders_target_path, ignore=shutil.ignore_patterns('*.minified.*'),dirs_exist_ok=True)
    for basename in os.listdir(generated_shader_ush_path):
        if 'minified' in basename:
            pathname = os.path.join(generated_shader_ush_path, basename)
            if os.path.isfile(pathname):
                ush_basename = os.path.splitext(basename)[0] + '.ush'
                ush_path = os.path.join(generated_shader_path, ush_basename)
                shutil.copyfile(pathname, ush_path)
    

if __name__ == '__main__':
    if args.rive_runtime_path:
        main(Path(args.rive_runtime_path).absolute().resolve())
    else:
        mono_runtime_directory = Path(__file__)
        mono_runtime_directory = mono_runtime_directory.joinpath("../../../../../../runtime").absolute().resolve()
        main(str(mono_runtime_directory))
