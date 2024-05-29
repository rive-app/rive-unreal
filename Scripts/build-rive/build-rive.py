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
    'rive_pls_shaders',
    'rive_sheenbidi',
    'libpng',
    'zlib'
]

def get_base_command(rive_renderer_path, release):
    return (
        f"premake5 --scripts=\"{rive_renderer_path}/submodules/rive-cpp/build\" "
        f"--with_rive_text --with_rive_audio=external {'--config=release' if release else '--config=debug'}"
    )

@click.command()
@click.argument('rive_renderer_path')
def main(rive_renderer_path):
    if sys.platform.startswith('darwin'):
        os.environ["MACOSX_DEPLOYMENT_TARGET"] = '11.0'
        # determine a sane build environment
        output = subprocess.check_output(["xcrun", "--sdk", "macosx", "--show-sdk-path"], universal_newlines=True)
        sdk_path = output.strip()
        if "MacOSX12" not in sdk_path:
            print_red(f"SDK Path {sdk_path} didn't point to an SDK matching version 12, exiting..")
            return
        else:
            print_green(f"Using SDK at: {output}")
    
        if not do_mac(rive_renderer_path, True) or not do_mac(rive_renderer_path, False):
            return
        
        if not do_ios(rive_renderer_path, True) or not do_ios(rive_renderer_path, False):
            return
    elif sys.platform.startswith('win32'):
        if not do_windows(rive_renderer_path, True) or not do_windows(rive_renderer_path, False):
            return
        
        #if not do_android(rive_renderer_path, True) or not do_android(rive_renderer_path, False):
        #    return
    else:
        print_red("Unsupported platform")
        return

    copy_includes(rive_renderer_path)

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

def do_android(rive_renderer_path, release):
    try:
        out_dir = os.path.join('out', 'android', 'release' if release else 'debug')
        os.chdir(rive_renderer_path)

        if 'NDK_ROOT' in os.environ and 'NDK_PATH' not in os.environ:
            os.environ['NDK_PATH'] = os.environ['NDK_ROOT']

        command = f'{get_base_command(rive_renderer_path, release)} --os=android --arch=arm64 --out="{out_dir}" vs2022'
        execute_command(command)

        msbuild_path = get_msbuild()
        
        os.chdir(out_dir)
        execute_command(f'"{msbuild_path}" ./rive.sln /t:{";".join(targets)}')

        print_green(f'Built in {os.getcwd()}')

        rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

        print_green('Copying Android libs')
        copy_files(os.getcwd(), os.path.join(rive_libraries_path, 'Android'), ".a", release)
    except Exception as e:
        print_red(f"Exiting due to errors... {e}")
        return False
    
    return True


def do_windows(rive_renderer_path, release):
    try:
        out_dir = os.path.join('out', 'windows', 'release' if release else 'debug')
        os.chdir(rive_renderer_path)
        command = f'{get_base_command(rive_renderer_path, release)} --windows_runtime=dynamic --os=windows --out="{out_dir}" vs2022'
        execute_command(command)

        msbuild_path = get_msbuild()
            
        os.chdir(out_dir)
        execute_command(f'"{msbuild_path}" ./rive.sln /t:{";".join(targets)}')
        
        print_green(f'Built in {os.getcwd()}')

        rive_libraries_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Libraries')

        print_green('Copying Windows')
        copy_files(os.getcwd(), os.path.join(rive_libraries_path, 'Win64'), ".lib", release)
    except Exception as e:
        print_red("Exiting due to errors...")
        print_red(e)
        return False
    
    return True


def do_ios(rive_renderer_path, release):
    try:
        os.chdir(rive_renderer_path)
        command = f'{get_base_command(rive_renderer_path, release)} gmake2 --os=ios'
        build_dirs = {}

        print_green('Building iOS')
        out_dir = os.path.join('out', 'ios', 'release' if release else 'debug')
        execute_command(f'{command} --variant=system --out="{out_dir}"')
        os.chdir(out_dir)
        build_dirs['ios'] = os.getcwd()
        for target in targets:
            execute_command(f'make {target}')

        print_green('Building iOS Simulator')
        out_dir = os.path.join('out', 'ios_sim', 'release' if release else 'debug')
        os.chdir(rive_renderer_path)
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

        os.chdir(rive_renderer_path)
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


def do_mac(rive_renderer_path, release):
    try:
        command = f'{get_base_command(rive_renderer_path, release)} --os=macosx gmake2'
        build_dirs = {}

        # we currently don't use fat libs, otherwise we could use this for macOS
        # build_fat = False
        # if build_fat:
        #     execute_command(f'{command} --arch=universal --out=out/universal')
        #     os.chdir(os.path.join(rive_renderer_path, 'out', 'universal'))
        #     build_dirs['universal'] = os.getcwd()
        #     execute_command('make')
        # else:


        print_green('Building macOS x64')
        out_dir = os.path.join('out', 'mac', 'x64', 'release' if release else 'debug')
        os.chdir(rive_renderer_path)
        execute_command(f'{command} --arch=x64 --out="{out_dir}"')
        os.chdir(out_dir)
        build_dirs['mac_x64'] = os.getcwd()
        for target in targets:
            execute_command(f'make {target}')

        print_green('Building macOS arm64')
        out_dir = os.path.join('out', 'mac', 'arm64', 'release' if release else 'debug')
        os.chdir(rive_renderer_path)
        execute_command(f'{command} --arch=arm64 --out="{out_dir}')
        os.chdir(out_dir)
        build_dirs['mac_arm64'] = os.getcwd()
        for target in targets:
            execute_command(f'make {target}')

        os.chdir(rive_renderer_path)
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

            # ensure all libs are prefixed with "rive_"
            if not file_name.startswith("rive"):
                file_name = f'rive_{file_name}'

            relative_path = os.path.relpath(root, src)
            dest_path = os.path.join(dst, relative_path, file_name if is_release else file_name.replace(extension, f'_d{extension}'))
            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            shutil.copy2(src_path, dest_path)


def copy_includes(rive_renderer_path):
    print_green('Copying rive includes...')
    rive_includes_path = os.path.join(rive_renderer_path, 'submodules', 'rive-cpp', 'include')
    rive_pls_includes_path = os.path.join(rive_renderer_path, 'include')
    rive_decoders_includes_path = os.path.join(rive_renderer_path, 'submodules', 'rive-cpp', 'decoders', 'include')
    target_path = os.path.join(script_directory, '..', '..', 'Source', 'ThirdParty', 'RiveLibrary', 'Includes')
    if os.path.exists(target_path):
        shutil.rmtree(target_path)

    shutil.copytree(rive_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_pls_includes_path, target_path, dirs_exist_ok=True)
    shutil.copytree(rive_decoders_includes_path, target_path, dirs_exist_ok=True)


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
