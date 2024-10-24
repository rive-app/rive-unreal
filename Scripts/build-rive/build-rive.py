import os
import shutil
import click
import subprocess
import sys

# install requirements via pip install -r requirements.txt
# running: python3 build-rive.py <path-to-rive-renderer-root>

script_directory = os.path.dirname(os.path.abspath(__file__))

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

should_build_test = False

def get_base_command(rive_runtime_path, release):
    return (
        f"premake5 --scripts=\"{os.path.join(rive_runtime_path, 'build')}\" "
        f"--with_rive_text --raw_shaders --with_rive_audio=external --for_unreal {'--config=release' if release else '--config=debug'}"
    )

@click.command()
@click.argument('rive_runtime_path')
@click.option('--build_tests', is_flag=True)
def main(rive_runtime_path, build_tests):
    global should_build_test
    should_build_test= build_tests
    rive_runtime_path = os.path.abspath(rive_runtime_path)
    
    if sys.platform.startswith('darwin'):
        os.environ["MACOSX_DEPLOYMENT_TARGET"] = '11.0'
        # determine a sane build environment
        output = subprocess.check_output(["xcrun", "--sdk", "macosx", "--show-sdk-path"], universal_newlines=True)
        sdk_path = output.strip()
        if "MacOSX13" not in sdk_path:
            print_red(f"SDK Path {sdk_path} didn't point to an SDK matching version 12, exiting..")
            return
        else:
            print_green(f"Using SDK at: {output}")
    
        if not do_mac(rive_runtime_path, True) or not do_mac(rive_runtime_path, False):
            return
        
        if not do_ios(rive_runtime_path, True) or not do_ios(rive_runtime_path, False):
            return
    elif sys.platform.startswith('win32'):
        if not do_windows(rive_runtime_path, True) or not do_windows(rive_runtime_path, False):
            return
        
        # apply android patch before android
        os.chdir(rive_runtime_path)
        #patch_output = subprocess.check_output(['git', 'apply', f'{os.path.join(script_directory, "patches", "android.patch")}'], universal_newlines=True)
        #print(patch_output)
        android_succeeded = True
        if not do_android(rive_runtime_path, True) or not do_android(rive_runtime_path, False):
           android_succeeded = False

        # unapply android patch after
        os.chdir(rive_runtime_path)
        #patch_output = subprocess.check_output(['git', 'apply', '-R', f'{os.path.join(script_directory, "patches", "android.patch")}'], universal_newlines=True)
        #print(patch_output)

        if not android_succeeded:
            return
    else:
        print_red("Unsupported platform")
        return

    copy_includes(rive_runtime_path)

def get_msbuild():
    msbuild_path = ''
    import vswhere

    msbuilds = vswhere.find('MSBuild')
    if len(msbuilds) == 0:
        raise Exception('Could not find msbuild')
    
    msbuild_path = os.path.join(msbuilds[-1:][0], 'Current', 'Bin', 'MSBuild.exe')
    if not os.path.exists(msbuild_path):
        raise Exception(f'Invalid MSBuild path {msbuild_path}')
    return msbuild_path

def do_android(rive_runtime_path, release):
    try:
        out_dir = os.path.join('..', 'out', 'android', 'release' if release else 'debug')
        os.chdir(os.path.join(rive_runtime_path, 'renderer'))

        if 'NDK_ROOT' in os.environ and 'NDK_PATH' not in os.environ:
            os.environ['NDK_PATH'] = os.environ['NDK_ROOT']

        command = f'{get_base_command(rive_runtime_path, release)} --os=android --arch=arm64 --out="{out_dir}" vs2022'
        execute_command(command)

        msbuild_path = get_msbuild()
        
        os.chdir(out_dir)
        execute_command(f'"{msbuild_path}" ./rive.sln /t:{";".join(targets)}')

        print_green(f'Built in {os.getcwd()}')

        os.chdir(os.path.join('ARM64', 'default'))
        rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

        print_green('Copying Android libs')
        copy_files(os.getcwd(), os.path.join(rive_libraries_path, 'Android'), ".a", release)
    except Exception as e:
        print_red(f"Exiting due to errors... {e}")
        return False
    
    return True


def do_windows(rive_runtime_path, release):
    try:
        out_dir = os.path.join('..', 'out', 'windows', 'release' if release else 'debug')

        os.chdir(os.path.join(rive_runtime_path, 'renderer'))
        
        command = f'{get_base_command(rive_runtime_path, release)} --os=windows --out="{out_dir}" vs2022'
        execute_command(command)

        # build
        msbuild_path = get_msbuild()
            
        os.chdir(out_dir)
        execute_command(f'"{msbuild_path}" ./rive.sln /t:{";".join(targets)}')
        
        print_green(f'Built in {os.getcwd()}')
        # end build

        rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

        print_green('Copying Windows')
        copy_files(os.getcwd(), os.path.join(rive_libraries_path, 'Win64'), ".lib", release)
    except Exception as e:
        print_red("Exiting due to errors...")
        print_red(e)
        return False
    
    return True


def do_ios(rive_runtime_path, release):
    try:
        os.chdir(os.path.join(rive_runtime_path, 'renderer'))
        command = f'{get_base_command(rive_runtime_path, release)} gmake2 --os=ios'
        build_dirs = {}

        print_green('Building iOS')
        out_dir = os.path.join('..', 'out', 'ios', 'release' if release else 'debug')
        execute_command(f'{command} --variant=system --out="{out_dir}"')
        os.chdir(out_dir)
        build_dirs['ios'] = os.getcwd()
        for target in targets:
            execute_command(f'make {target}')

        print_green('Building iOS Simulator')
        out_dir = os.path.join('..', 'out', 'ios_sim', 'release' if release else 'debug')
        os.chdir(os.path.join(rive_runtime_path, 'renderer'))
        execute_command(f'{command} --variant=emulator --out="{out_dir}"')
        os.chdir(out_dir)
        build_dirs['ios_sim'] = os.getcwd()
        for target in targets:
            execute_command(f'make {target}')
        
        # clear old .sim.a files
        for root, dirs, files in os.walk(build_dirs['ios_sim']):
            for file in files:
                if file.endswith('.sim.a'):
                    os.unlink(os.path.join(root, file))

        # we need to rename all ios simulator to .sim.a
        for root, dirs, files in os.walk(build_dirs['ios_sim']):
            for file in files:
                if file.endswith('.a'):
                    old_file = os.path.join(root, file)
                    new_file = f'{file[:-2]}.sim.a'
                    new_file = os.path.join(root, new_file)
                    try:
                        os.unlink(new_file) # just in case an existing .sim.a exists
                    except:
                        pass
                    os.rename(old_file, new_file)

        os.chdir(rive_runtime_path)
        print_green(f'Built in {build_dirs}')

        rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

        print_green('Copying iOS and iOS simulator')
        copy_files(build_dirs['ios'], os.path.join(rive_libraries_path, 'IOS'), ".a", release)
        copy_files(build_dirs['ios_sim'], os.path.join(rive_libraries_path, 'IOS'), ".sim.a", release)
    except Exception as e:
        print_red("Exiting due to errors...")
        print_red(e)
        return False
    
    return True


def do_mac(rive_runtime_path, release):
    try:
        command = f'{get_base_command(rive_runtime_path, release)} --os=macosx gmake2'
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
        os.chdir(os.path.join(rive_runtime_path, 'renderer'))
        execute_command(f'{command} --arch=x64 --out="{out_dir}"')
        os.chdir(out_dir)
        build_dirs['mac_x64'] = os.getcwd()
        for target in targets:
            execute_command(f'make {target}')

        print_green('Building macOS arm64')
        out_dir = os.path.join('..', 'out', 'mac', 'arm64', 'release' if release else 'debug')
        os.chdir(os.path.join(rive_runtime_path, 'renderer'))
        execute_command(f'{command} --arch=arm64 --out="{out_dir}"')
        os.chdir(out_dir)
        build_dirs['mac_arm64'] = os.getcwd()
        for target in targets:
            execute_command(f'make {target}')

        os.chdir(rive_runtime_path)
        print_green(f'Built in {build_dirs}')

        rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

        print_green('Copying macOS x64')
        copy_files(build_dirs['mac_x64'], os.path.join(rive_libraries_path, 'Mac', 'Intel'), ".a", release)
        copy_files(build_dirs['mac_arm64'], os.path.join(rive_libraries_path, 'Mac', 'Mac'), ".a", release)
    except Exception as e:
        print_red("Exiting due to errors...")
        print_red(e)
        return False
    
    return True


def copy_files(src, dst, extension, is_release):
     # Ensure the source directory exists
    if not os.path.exists(src):
        print_red(f"The source directory {src} does not exist.")
        return

    os.makedirs(dst, exist_ok=True)

    for root, dirs, files in os.walk(src):
        files_to_copy = [f for f in files if f.endswith(extension)]
        for file_name in files_to_copy:
            src_path = os.path.join(root, file_name)

            # ensure all libs are prefixed with "rive_" on non-darwin / apple platforms
            if not sys.platform.startswith("darwin") and not file_name.startswith("rive") and not 'Android' in dst:
                file_name = f'rive_{file_name}'

            relative_path = os.path.relpath(root, src)
            dest_path = os.path.join(dst, file_name if is_release else file_name.replace(extension, f'_d{extension}'))
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            shutil.copy2(src_path, dest_path)


def copy_includes(rive_runtime_path):
    print_green('Copying rive includes...')
    rive_includes_path = os.path.join(rive_runtime_path, 'include')
    rive_pls_includes_path = os.path.join(rive_runtime_path, 'renderer', 'include')
    rive_shaders_includes_path = os.path.join(rive_runtime_path, 'out', 'windows', 'release', "include")
    rive_shader_source_path = os.path.join(rive_runtime_path, 'renderer', 'src', 'shaders', "unreal")
    rive_decoders_includes_path = os.path.join(rive_runtime_path, 'decoders', 'include')
    target_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Includes')
    shaders_target_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Includes', "rive")
    rive_shader_source_target_path = os.path.join(script_directory, '..', '..', 'Shaders', 'Private', 'Rive')
    generated_shader_path = os.path.join(script_directory, '..', '..', 'Shaders', 'Private', 'Rive', 'Generated')
    generated_shader_ush_path = os.path.join(rive_shaders_includes_path, "generated", "shaders")

    if os.path.exists(target_path):
        shutil.rmtree(target_path)

    if os.path.exists(generated_shader_path):
        shutil.rmtree(generated_shader_path)

    # delete and re create generated shader path to clear it
    os.mkdir(generated_shader_path)

    shutil.copytree(rive_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_pls_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_decoders_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_shader_source_path, rive_shader_source_target_path, dirs_exist_ok=True)

    shutil.copytree(rive_shaders_includes_path, shaders_target_path, ignore=shutil.ignore_patterns('*.ush'),dirs_exist_ok=True)
    for basename in os.listdir(generated_shader_ush_path):
        if basename.endswith('.ush'):
            pathname = os.path.join(generated_shader_ush_path, basename)
            if os.path.isfile(pathname):
                shutil.copy(pathname, generated_shader_path)
    


def execute_command(cmd):
    had_errors = False
    print_green(f'Executing {cmd}')
    process = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        universal_newlines=True
    )

    while True:
        output = process.stdout.readline()
        if output == '' and process.poll() is not None:
            break
        if output:
            if ' error' in output.lower() and 'Structured output' not in output and '0 Error' not in output:
                print_red(f'{output.strip()}')
                had_errors = True
            else:
                print(output.strip())

    if sys.platform.startswith('darwin'):
        stdout, stderr = process.communicate()
        if stderr:
            if not "pnglibconf.h: Permission denied" in stderr and not "has no symbols" in stderr:
                print_red(stderr.strip())
                had_errors = True

    process.wait()

    if had_errors:
        raise Exception('execute_command had errors')


def print_green(text):
    print(f'\033[32m{text}\033[0m')


def print_red(text):
    print(f'\033[91m{text}\033[0m')


if __name__ == '__main__':
    main()
