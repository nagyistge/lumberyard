#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
#  Android builder
"""
Usage (in wscript):

def options(opt):
    opt.load('android')

def configure(conf):
    conf.load('android')
"""
import os, sys, random, time, re, stat, string, imghdr, multiprocessing
import atexit, shutil, threading, collections, hashlib, subprocess
import xml.etree.ElementTree as ET

from contextlib import contextmanager
from subprocess import call, check_output

from cry_utils import append_to_unique_list, get_command_line_limit
from third_party import is_third_party_uselib_configured

from waflib import Context, TaskGen, Build, Utils, Node, Logs, Options, Errors
from waflib.Build import POST_LAZY, POST_AT_ONCE
from waflib.Configure import conf
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waflib.TaskGen import feature, extension, before, before_method, after, after_method, taskgen_method

from waflib.Tools import ccroot
ccroot.USELIB_VARS['android'] = set([ 'AAPT', 'AAPT_RESOURCES', 'AAPT_INCLUDES', 'AAPT_PACKAGE_FLAGS' ])


################################################################
#                     Defaults                                 #
BUILDER_DIR = 'Code/Launcher/AndroidLauncher/ProjectBuilder'
BUILDER_FILES = 'android_builder.json'
ANDROID_LIBRARY_FILES = 'android_libraries.json'

RESOLUTION_MESSAGE = 'Please re-run Setup Assistant with "Compile For Android" enabled and run the configure command again.'

RESOLUTION_SETTINGS = ( 'mdpi', 'hdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi' )

LATEST_KEYWORD = 'latest'

ANDROID_CACHE_FOLDER = 'AndroidCache'

APK_WITH_ASSETS_SUFFIX = '_w_assets'

# these are the default names for application icons and splash images
APP_ICON_NAME = 'app_icon.png'
APP_SPLASH_NAME = 'app_splash.png'

# supported api versions
SUPPORTED_APIS = [
    'android-19',
    'android-21',
    'android-22',
    'android-23',
    'android-24'
]

# while some earlier versions may work, it's probably best to enforce a min version of the build tools
MIN_BUILD_TOOLS_VERSION = '19.1.0'

# known build tools versions with stablity issues
UNSUPPORTED_BUILD_TOOLS_VERSIONS = {
    'win32' : [
        '24.0.0'        # works fine on win 7 machines but consistantly crashes on win 10 machines
    ]
}

# build tools versions marked as obsolete by the Android SDK manager
OBSOLETE_BUILD_TOOLS_VERSIONS = [
    '21.0.0',
    '21.0.1',
    '21.0.2',
    '21.1.0',
    '21.1.1',
    '22.0.0',
    '23.0.0'
]

# 'defines' for the different asset deployment modes
ASSET_DEPLOY_LOOSE = 'loose'
ASSET_DEPLOY_PAKS = 'paks'
ASSET_DEPLOY_PROJECT_SETTINGS = 'project_settings'

ASSET_DEPLOY_MODES = [
    ASSET_DEPLOY_LOOSE,
    ASSET_DEPLOY_PAKS,
    ASSET_DEPLOY_PROJECT_SETTINGS
]

# root types
ACCESS_NORMAL = 0 # the device is not rooted, we do not have access to any elevated permissions
ACCESS_ROOT_ADBD = 1 # the device is rooted, we have elevated permissions at the adb level
ACCESS_SHELL_SU = 2 # the device is rooted, we only have elevated permissions using 'adb shell su -c'

# The default permissions for installed libriares on device.
LIB_FILE_PERMISSIONS = '755'

# The default owner:group for installed libraries on device.
LIB_OWNER_GROUP = 'system:system'

# the default file permissions after copies are made.  511 => 'chmod 777 <file>'
FILE_PERMISSIONS = 511

AUTO_GEN_HEADER_PYTHON = r'''
################################################################
# This file was automatically created by WAF
# WARNING! All modifications will be lost!
################################################################

'''
#                                                              #
################################################################


################################################################
"""
Parts of the Android build process require the ability to create directory junctions/symlinks to make sure some assets are properly
included in the build while maintaining as small of a memory footprint as possible (don't want to make copies).  Since we only care
about directories, we don't need the full power of os.symlink (doesn't work on windows anyway and writing one would require either
admin privileges or running something such as a VB script to create a shortcut to bypass the admin issue; neither of those options
are desirable).  The following functions are to make it explicitly clear we only care about directory links.
"""
def junction_directory(source, link_name):
    if not os.path.isdir(source):
        Logs.error("[ERROR] Attempting to make a junction to a file, which is not supported.  Unexpected behaviour may result.")
        return

    if Utils.unversioned_sys_platform() == "win32":
        cleaned_source_name = '"' + source.replace('/', '\\') + '"'
        cleaned_link_name = '"' + link_name.replace('/', '\\') + '"'

        # mklink generaully requires admin privileges, however directory junctions don't.
        # subprocess.check_call will auto raise.
        subprocess.check_call('mklink /D /J %s %s' % (cleaned_link_name, cleaned_source_name), shell=True)
    else:
        os.symlink(source, link_name)


def remove_junction(junction_path):
    """
    Wrapper for correctly deleting a symlink/junction regardless of host platform
    """
    if Utils.unversioned_sys_platform() == "win32":
        os.rmdir(junction_path)
    else:
        os.unlink(junction_path)


@contextmanager
def push_dir(directory):
    """
    Temporarily changes the current working directory.  By decorating it with the contexmanager, makes this function only 
    useable in "with" statements, otherwise its a no-op.  When the "with" statement is executed, this function will run
    till the yeild, then run what's inside the "with" statement and finally run what's after the yeild.
    """
    previous_dir = os.getcwd()
    os.chdir(directory)
    yield
    os.chdir(previous_dir)


################################################################
@feature('cshlib', 'cxxshlib')
@after_method('apply_link')
def apply_so_name(self):
    """
    Adds the linker flag to set the DT_SONAME in ELF shared objects. The
    name used here will be used instead of the file name when the dynamic
    linker attempts to load the shared object
    """

    if 'android' in self.bld.env['PLATFORM'] and self.env.SONAME_ST:
        flag = self.env.SONAME_ST % self.link_task.outputs[0]
        self.env.append_value('LINKFLAGS', flag.split())


@feature('find_android_api_libs')
@before_method('propagate_uselib_vars')
def find_api_lib(self):
    """
    Goes through the list of libraries included by the 'uselib' keyword and search if there's a better version of the library for the
    API level being used. If one is found, the new version is used instead.
    The api specific version must have the same name as the original library + "_android_XX".
    For example, if we are including a library called "foo", and we also have defined "foo_android_21", when compiling with API 23,
    "foo" will be replaced with "foo_android_21". We do this for supporting multiple versions of a library depending on which API
    level the user is using.
    """

    def find_api_helper(lib_list, api_list):
        if not lib_list or not api_list:
            return

        for idx, lib in enumerate(lib_list):
            for api_level in api_list:
                lib_name = (lib + '_' + api_level).upper()
                if is_third_party_uselib_configured(self.bld, lib_name):
                    lib_list[idx] = lib_name
                    break

    if 'android' not in self.bld.env['PLATFORM']:
        return

    uselib_keys = getattr(self, 'uselib', None)
    use_keys = getattr(self, 'use', None)
    if uselib_keys or use_keys:
        # sort the list of supported apis and search if there's a better version depending on the Android NDK platform version being used.
        api_list = sorted(SUPPORTED_APIS)
        ndk_platform = self.bld.get_android_ndk_platform()
        try:
            index = api_list.index(ndk_platform)
        except:
            self.bld.fatal('[ERROR] Unsupported Android NDK platform version %s' % ndk_platform)
            return

        # we can only use libs built with api levels lower or equal to the one being used.
        api_list = api_list[:index + 1] # end index is exclusive, so we add 1
        # reverse the list so we use the lib with the higher api (if we find one).
        api_list.reverse()
        api_list = [ api.replace('-', '_') for api in api_list ]

        find_api_helper(uselib_keys, api_list)
        find_api_helper(use_keys, api_list)


################################################################
def remove_file_and_empty_directory(directory, file_name):
    """
    Helper function for deleting a file and directory, if empty
    """

    file_path = os.path.join(directory, file_name)

    # first delete the file, if it exists
    if os.path.exists(file_path):
        os.remove(file_path)

    # then remove the directory, if it exists and is empty
    if os.path.exists(directory) and not os.listdir(directory):
        os.rmdir(directory)


################################################################
def remove_readonly(func, path, _):
    '''Clear the readonly bit and reattempt the removal'''
    os.chmod(path, stat.S_IWRITE)
    func(path)


################################################################
def construct_source_path(conf, project, source_path):
    """
    Helper to construct the source path to an asset override such as
    application icons or splash screen images
    """
    if os.path.isabs(source_path):
        path_node = conf.root.make_node(source_path)
    else:
        relative_path = os.path.join('Code', project, 'Resources', source_path)
        path_node = conf.path.make_node(relative_path)
    return path_node.abspath()


################################################################
def clear_splash_assets(project_node, path_prefix):

    target_directory = project_node.make_node(path_prefix)
    remove_file_and_empty_directory(target_directory.abspath(), APP_SPLASH_NAME)

    for resolution in RESOLUTION_SETTINGS:
        # The xxxhdpi resolution is only for application icons, its overkill to include them for drawables... for now
        if resolution == 'xxxhdpi':
            continue

        target_directory = project_node.make_node(path_prefix + '-' + resolution)
        remove_file_and_empty_directory(target_directory.abspath(), APP_SPLASH_NAME)


################################################################
def options(opt):
    group = opt.add_option_group('android-specific config')

    group.add_option('--android-toolchain', dest = 'android_toolchain', action = 'store', default = '', help = 'DEPRECATED: Android toolchain to use for building, valid options are gcc or clang')
    group.add_option('--use-incredibuild-android', dest = 'use_incredibuild_android', action = 'store', default = 'False', help = 'DEPRECATED: Use --use-incredibuild instead to enable Incredibuild for Android.')

    group.add_option('--android-sdk-version-override', dest = 'android_sdk_version_override', action = 'store', default = '', help = 'Override the Android SDK version used in the Java compilation.  Only works during configure.')
    group.add_option('--android-ndk-platform-override', dest = 'android_ndk_platform_override', action = 'store', default = '', help = 'Override the Android NDK platform version used in the native compilation.  Only works during configure.')

    group.add_option('--dev-store-pass', dest = 'dev_store_pass', action = 'store', default = 'Lumberyard', help = 'The store password for the development keystore')
    group.add_option('--dev-key-pass', dest = 'dev_key_pass', action = 'store', default = 'Lumberyard', help = 'The key password for the development keystore')

    group.add_option('--distro-store-pass', dest = 'distro_store_pass', action = 'store', default = '', help = 'The store password for the distribution keystore')
    group.add_option('--distro-key-pass', dest = 'distro_key_pass', action = 'store', default = '', help = 'The key password for the distribution keystore')

    group.add_option('--android-apk-path', dest = 'apk_path', action = 'store', default = '', help = 'Path to apk to deploy. If not specified the default build path will be used')

    group.add_option('--from-editor-deploy', dest = 'from_editor_deploy', action = 'store_true', default = False, help = 'Signals that the build is coming from the editor deployment tool')

    group.add_option('--deploy-android-attempt-libs-only', dest = 'deploy_android_attempt_libs_only', action = 'store_true', default = False, 
        help = 'Will only push the changed native libraries.  If "deploy_android_executable" is enabled, it will take precedent if modified.  Option ignored if "deploy_android_clean_device" is enabled.  This feature is only available for "unlocked" devices.')


################################################################
def configure(conf):

    env = conf.env

    # validate the stored sdk and ndk paths from SetupAssistant
    sdk_root = conf.get_env_file_var('LY_ANDROID_SDK', required = True)
    ndk_root = conf.get_env_file_var('LY_ANDROID_NDK', required = True)

    if not (sdk_root and ndk_root):
        missing_paths = []
        missing_paths += ['Android SDK'] if not sdk_root else []
        missing_paths += ['Android NDK'] if not ndk_root else []

        conf.fatal('[ERROR] Missing paths from Setup Assistant detected for: {}.  {}'.format(', '.join(missing_paths), RESOLUTION_MESSAGE))

    env['ANDROID_SDK_HOME'] = sdk_root
    env['ANDROID_NDK_HOME'] = ndk_root

    # get the revision of the NDK
    with open(os.path.join(ndk_root, 'source.properties')) as ndk_props_file:
        for line in ndk_props_file.readlines():
            tokens = line.split('=')
            trimed_tokens = [token.strip() for token in tokens]

            if 'Pkg.Revision' in trimed_tokens:
                ndk_rev = trimed_tokens[1]

    env['ANDROID_NDK_REV_FULL'] = ndk_rev

    ndk_rev_tokens = ndk_rev.split('.')
    env['ANDROID_NDK_REV_MAJOR'] = int(ndk_rev_tokens[0])
    env['ANDROID_NDK_REV_MINOR'] = int(ndk_rev_tokens[1])

    headers_type = 'unified platform' if conf.is_using_android_unified_headers() else 'platform specific'
    Logs.debug('android: Using {} headers with Android NDK revision {}.'.format(headers_type, ndk_rev))

    # validate the desired SDK version
    installed_sdk_versions = os.listdir(os.path.join(sdk_root, 'platforms'))
    valid_sdk_versions = [platorm for platorm in installed_sdk_versions if platorm in SUPPORTED_APIS]
    Logs.debug('android: Valid installed SDK versions are: {}'.format(valid_sdk_versions))

    sdk_version = Options.options.android_sdk_version_override
    if not sdk_version:
        sdk_version = conf.get_android_sdk_version()

    if sdk_version.lower() == LATEST_KEYWORD:
        if not valid_sdk_versions:
            conf.fatal('[ERROR] Unable to detect a valid Android SDK version installed in path {}.  '
                        'Please use the Android SDK Manager to download an appropriate SDK version and run the configure command again.\n'
                        '\t-> Supported APIs installed are: {}'.format(sdk_root, ', '.join(SUPPORTED_APIS)))

        valid_sdk_versions = sorted(valid_sdk_versions)
        sdk_version = valid_sdk_versions[-1]
        Logs.debug('android: Using the latest installed Android SDK version {}'.format(sdk_version))

    else:
        if sdk_version not in SUPPORTED_APIS:
            conf.fatal('[ERROR] Android SDK version - {} - is unsupported.  Please change SDK_VERSION in _WAF_/android/android_setting.json to a supported API and run the configure command again.\n'
                        '\t-> Supported APIs are: {}'.format(sdk_version, ', '.join(SUPPORTED_APIS)))

        if sdk_version not in valid_sdk_versions:
            conf.fatal('[ERROR] Failed to find Android SDK version - {} - installed in path {}.  '
                        'Please use the Android SDK Manager to download the appropriate SDK version or change SDK_VERSION in _WAF_/android/android_settings.json to a supported version installed and run the configure command again.\n'
                        '\t-> Supported APIs installed are: {}'.format(sdk_version, sdk_root, ', '.join(valid_sdk_versions)))

    env['ANDROID_SDK_VERSION'] = sdk_version
    env['ANDROID_SDK_VERSION_NUMBER'] = int(sdk_version.split('-')[1])

    # validate the desired NDK platform version
    ndk_platform = Options.options.android_ndk_platform_override
    if not ndk_platform:
        ndk_platform = conf.get_android_ndk_platform()

    ndk_sdk_match = False
    if not ndk_platform:
        Logs.debug('android: The Android NDK platform version has not been specified.  Auto-detecting from specified Android SDK version {}.'.format(sdk_version))
        ndk_platform = sdk_version
        ndk_sdk_match = True

    ndk_platforms = os.listdir(os.path.join(ndk_root, 'platforms'))
    valid_ndk_platforms = [platorm for platorm in ndk_platforms if platorm in SUPPORTED_APIS]
    Logs.debug('android: Valid NDK platforms for revision {} are: {}'.format(ndk_rev, valid_ndk_platforms))

    if ndk_platform not in valid_ndk_platforms:
        if ndk_sdk_match:
            # search the valid ndk platforms for one that is closest, but lower, than the desired sdk version
            sorted_valid_platforms = sorted(valid_ndk_platforms)
            for platform in sorted_valid_platforms:
                if platform <= sdk_version:
                    ndk_platform = platform
            Logs.debug('android: Closest Android NDK platform version detected from Android SDK version {} is {}'.format(sdk_version, ndk_platform))
        else:
            platform_list = ', '.join(valid_ndk_platforms)
            conf.fatal("[ERROR] Attempting to use a NDK platform - {} - that is either unsupported or doesn't have platform specific headers.  "
                        "Please set NDK_PLATFORM in _WAF_/android/android_settings.json to a valid platform or remove to auto-detect from SDK_VERSION and run the configure command again.\n"
                        "\t-> Valid platforms for NDK {} include: {}".format(ndk_platform, ndk_rev, platform_list))

    env['ANDROID_NDK_PLATFORM'] = ndk_platform
    env['ANDROID_NDK_PLATFORM_NUMBER'] = int(ndk_platform.split('-')[1])

    # final check is to make sure the ndk platform <= sdk version to ensure compatibilty
    if not (ndk_platform <= sdk_version):
        conf.fatal('[ERROR] The Android API specified in NDK_PLATFORM - {} - is newer than the API specified in SDK_VERSION - {}; this can lead to compatibilty issues.\n'
                    'Please update your _WAF_/android/android_settings.json to make sure NDK_PLATFORM <= SDK_VERSION and run the configure command again.'.format(ndk_platform, sdk_version))

    # validate the desired SDK build-tools version
    build_tools_version = conf.get_android_build_tools_version()

    build_tools_dir = os.path.join(sdk_root, 'build-tools')
    build_tools_dir_contents = os.listdir(build_tools_dir)

    host_platform = Utils.unversioned_sys_platform()
    host_unsupported_build_tools_versions = UNSUPPORTED_BUILD_TOOLS_VERSIONS.get(host_platform, [])

    unusable_build_tools_versions = host_unsupported_build_tools_versions + OBSOLETE_BUILD_TOOLS_VERSIONS

    installed_build_tools_versions = [ entry for entry in build_tools_dir_contents if entry.split('.')[0].isdigit() ]

    valid_build_tools_versions = [ entry for entry in installed_build_tools_versions if entry >= MIN_BUILD_TOOLS_VERSION and entry not in unusable_build_tools_versions ]
    Logs.debug('android: Valid installed build-tools versions are: {}'.format(valid_build_tools_versions))

    if build_tools_version.lower() == LATEST_KEYWORD:
        if not valid_build_tools_versions:
            conf.fatal('[ERROR] Unable to detect a valid Android SDK build-tools version installed in path {}.  Please use the Android SDK Manager to download build-tools '
                        'version {} or higher and run the configure command again.  Also note the following versions are unsupported:\n'
                        '\t-> {}'.format(sdk_root, MIN_BUILD_TOOLS_VERSION, ', '.join(host_unsupported_build_tools_versions)))

        valid_build_tools_versions = sorted(valid_build_tools_versions)
        build_tools_version = valid_build_tools_versions[-1]
        Logs.debug('android: Using the latest installed Android SDK build tools version {}'.format(build_tools_version))

    elif build_tools_version not in valid_build_tools_versions:
        if build_tools_version in OBSOLETE_BUILD_TOOLS_VERSIONS:
            Logs.warn('[WARN] The build-tools versions selected - {} - has been marked as obsolete by Google.  Consider using a different version of the build tools by '
                        'changing BUILD_TOOLS_VER in _WAF_/android/android_settings.json to "latest" or a version mentioned below and run the configure command again.\n'
                        '\t-> Valid installed build-tools version detected: {}'.format(build_tools_version, ', '.join(valid_build_tools_versions)))
        else:
            conf.fatal('[ERROR] The build-tools versions selected - {} - was either not found in path {} or unsupported.  Please use Android SDK Manager to download the appropriate build-tools version '
                        'or change BUILD_TOOLS_VER in _WAF_/android/android_settings.json to either a valid version installed or "latest" and run the configure command again.\n'
                        '\t-> Valid installed build-tools version detected: {}'.format(build_tools_version, build_tools_dir, ', '.join(valid_build_tools_versions)))

    conf.env['ANDROID_BUILD_TOOLS_VER'] = build_tools_version


################################################################
@conf
def is_using_android_unified_headers(conf):
    """
    NDK r14 introduced unified headers which is to replace the old platform specific set
    of headers in previous versions of the NDK.  There is a bug in one of the headers
    (strerror_r isn't proper defined) in this version but fixed in NDK r14b.  Because of
    this we will make NDK r14b our min spec for using the unified headers.
    """
    env = conf.env

    ndk_rev_major = env['ANDROID_NDK_REV_MAJOR']
    ndk_rev_minor = env['ANDROID_NDK_REV_MINOR']

    return ((ndk_rev_major == 14 and ndk_rev_minor >= 1) or (ndk_rev_major >= 15))


################################################################
@conf
def load_android_toolchains(conf, search_paths, CC, CXX, AR, STRIP, **addition_toolchains):
    """
    Loads the native toolchains from the Android NDK
    """
    try:
        conf.find_program(CC, var = 'CC', path_list = search_paths, silent_output = True)
        conf.find_program(CXX, var = 'CXX', path_list = search_paths, silent_output = True)
        conf.find_program(AR, var = 'AR', path_list = search_paths, silent_output = True)

        # for debug symbol stripping
        conf.find_program(STRIP, var = 'STRIP', path_list = search_paths, silent_output = True)

        # optional linker override
        if 'LINK_CC' in addition_toolchains and 'LINK_CXX' in addition_toolchains:
            conf.find_program(addition_toolchains['LINK_CC'], var = 'LINK_CC', path_list = search_paths, silent_output = True)
            conf.find_program(addition_toolchains['LINK_CXX'], var = 'LINK_CXX', path_list = search_paths, silent_output = True)
        else:
            conf.env['LINK_CC'] = conf.env['CC']
            conf.env['LINK_CXX'] = conf.env['CXX']

        conf.env['LINK'] = conf.env['LINK_CC']

        # common cc settings
        conf.cc_load_tools()
        conf.cc_add_flags()

        # common cxx settings
        conf.cxx_load_tools()
        conf.cxx_add_flags()

        # common link settings
        conf.link_add_flags()
    except:
        Logs.error('[ERROR] Failed to find the Android NDK standalone toolchain(s) in search path %s' % search_paths)
        return False

    return True


################################################################
@conf
def load_android_tools(conf):
    """
    Loads the necessary build tools from the Android SDK
    """
    android_sdk_home = conf.env['ANDROID_SDK_HOME']

    build_tools_version = conf.get_android_build_tools_version()
    build_tools_dir = os.path.join(android_sdk_home, 'build-tools', build_tools_version)

    try:
        conf.find_program('aapt', var = 'AAPT', path_list = [ build_tools_dir ], silent_output = True)
        conf.find_program('aidl', var = 'AIDL', path_list = [ build_tools_dir ], silent_output = True)
        conf.find_program('dx', var = 'DX', path_list = [ build_tools_dir ], silent_output = True)
        conf.find_program('zipalign', var = 'ZIPALIGN', path_list = [ build_tools_dir ], silent_output = True)
    except:
        Logs.error('[ERROR] The desired Android SDK build-tools version - {} - appears to be incomplete.  Please use Android SDK Manager to validate the build-tools version installation '
                         'or change BUILD_TOOLS_VER in _WAF_/android/android_settings.json to either a version installed or "latest" and run the configure command again.'.format(build_tools_version))
        return False

    return True


################################################################
@conf
def get_android_cache_node(conf):
    return conf.get_bintemp_folder_node().make_node(ANDROID_CACHE_FOLDER)


################################################################
@conf
def add_to_android_cache(conf, path_to_resource):
    """
    Adds resource files from outside the engine folder into a local cache directory so they can be used by WAF tasks.
    Returns the path of the new resource file relative the cache root.
    """

    cache_node = get_android_cache_node(conf)
    cache_node.mkdir()

    dest_node = cache_node
    if conf.env['ANDROID_ARCH']:
        dest_node = cache_node.make_node(conf.env['ANDROID_ARCH'])
        dest_node.mkdir()

    file_name = os.path.basename(path_to_resource)

    files_node = dest_node.make_node(file_name)
    files_node.delete()

    shutil.copy2(path_to_resource, files_node.abspath())
    files_node.chmod(FILE_PERMISSIONS)

    rel_path = files_node.path_from(cache_node)
    Logs.debug('android: Adding resource - {} - to Android cache'.format(rel_path))
    return rel_path


################################################################
def process_json(conf, json_data, curr_node, root_node, template, copied_files):
    for elem in json_data:

        if elem == 'NO_OP':
            continue

        if os.path.isabs(elem):
            source_curr = conf.root.make_node(elem)
        else:
            source_curr = root_node.make_node(elem)

        target_curr = curr_node.make_node(elem)

        if isinstance(json_data, dict):
            # resolve name overrides for the copy, if specified
            if isinstance(json_data[elem], unicode) or isinstance(json_data[elem], str):
                target_curr = curr_node.make_node(json_data[elem])

            # otherwise continue processing the tree
            else:
                target_curr.mkdir()
                process_json(conf, json_data[elem], target_curr, root_node, template, copied_files)
                continue

        # leaf handing
        if imghdr.what(source_curr.abspath()) in ( 'rgb', 'gif', 'pbm', 'ppm', 'tiff', 'rast', 'xbm', 'jpeg', 'bmp', 'png' ):
            shutil.copyfile(source_curr.abspath(), target_curr.abspath())
        else:
            transformed_text = string.Template(source_curr.read()).substitute(template)
            target_curr.write(transformed_text)

        target_curr.chmod(FILE_PERMISSIONS)
        copied_files.append(target_curr.abspath())


################################################################
def copy_and_patch_android_libraries(conf, source_node, android_root):
    """
    Copy the libraries that need to be patched and do the patching of the files.
    """

    class _Library:
        def __init__(self, name, path):
            self.name = name
            self.path = path
            self.patch_files = []

        def add_file_to_patch(self, file):
            self.patch_files.append(file)

    class _File:
        def __init__(self, path):
            self.path = path
            self.changes = []

        def add_change(self, change):
            self.changes.append(change)

    class _Change:
        def __init__(self, line, old, new):
            self.line = line
            self.old = old
            self.new = new

    lib_src_file = source_node.make_node(ANDROID_LIBRARY_FILES)

    json_data = conf.parse_json_file(lib_src_file)
    if not json_data:
        conf.fatal('[ERROR] Android library settings (%s) not found or invalid.' % ANDROID_LIBRARY_FILES)
        return False

    # Collect the libraries that need to be patched
    libs_to_patch = []
    for libName, value in json_data.iteritems():
        # The library is in different places depending on the revision, so we must check multiple paths.
        srcDir = None
        for path in value['srcDir']:
            path = string.Template(path).substitute(conf.env)
            if os.path.exists(path):
                srcDir = path
                break

        if not srcDir:
            conf.fatal('[ERROR] Failed to find library - %s - in path(s) [%s]. Please download the library from the Android SDK Manager and run the configure command again.'
                        % (libName, ", ".join(string.Template(path).substitute(conf.env) for path in value['srcDir'])))
            return False

        if 'patches' in value:
            lib_to_patch = _Library(libName, srcDir)
            for patch in value['patches']:
                file_to_patch =  _File(patch['path'])
                for change in patch['changes']:
                    lineNum = change['line']
                    oldLines = change['old']
                    newLines = change['new']
                    for oldLine in oldLines[:-1]:
                        change = _Change(lineNum, oldLine, (newLines.pop() if newLines else None))
                        file_to_patch.add_change(change)
                        lineNum += 1
                    else:
                        change = _Change(lineNum, oldLines[-1], ('\n'.join(newLines) if newLines else None))
                        file_to_patch.add_change(change)

                lib_to_patch.add_file_to_patch(file_to_patch)
            libs_to_patch.append(lib_to_patch)

    # Patch the libraries
    for lib in libs_to_patch:
        dest_path = os.path.join(conf.Path(conf.get_android_patched_libraries_relative_path()), lib.name)
        shutil.rmtree(dest_path, ignore_errors=True, onerror=remove_readonly)
        shutil.copytree(lib.path, dest_path)
        for file in lib.patch_files:
            inputFilePath = os.path.join(lib.path, file.path)
            outputFilePath = os.path.join(dest_path, file.path)
            with open(inputFilePath) as inputFile:
                lines = inputFile.readlines()

            with open(outputFilePath, 'w') as outFile:
                for replace in file.changes:
                    lines[replace.line] = string.replace(lines[replace.line], replace.old, (replace.new if replace.new else ""), 1)

                outFile.write(''.join([line for line in lines if line]))

    return True


################################################################
@conf
def create_and_add_android_launchers_to_build(conf):
    """
    This function will generate the bare minimum android project
    and include the new android launcher(s) in the build path.
    So no Android Studio gradle files will be generated.
    """
    android_root = conf.path.make_node(conf.get_android_project_relative_path())
    android_root.mkdir()

    if conf.is_engine_local():
        source_node = conf.path.make_node(BUILDER_DIR)
    else:
        source_node = conf.root.make_node(os.path.abspath(os.path.join(conf.engine_path,BUILDER_DIR)))
    builder_file_src = source_node.make_node(BUILDER_FILES)
    builder_file_dest = conf.path.get_bld().make_node(BUILDER_DIR)

    if not os.path.exists(builder_file_src.abspath()):
        conf.fatal('[ERROR] Failed to find the Android project builder - %s - in path %s.  Verify file exists and run the configure command again.' % (BUILDER_FILES, BUILDER_DIR))
        return False

    created_directories = []
    for project in conf.get_enabled_game_project_list():
        # make sure the project has android options specified
        if conf.get_android_settings(project) == None:
            Logs.warn('[WARN] Android settings not found in %s/project.json, skipping.' % project)
            continue

        proj_root = android_root.make_node(conf.get_executable_name(project))
        proj_root.mkdir()

        created_directories.append(proj_root.path_from(android_root))

        proj_src_path = os.path.join(proj_root.abspath(), 'src')
        if os.path.exists(proj_src_path):
            shutil.rmtree(proj_src_path, ignore_errors=True, onerror=remove_readonly)

        # setup the macro replacement map for the builder files
        activity_name = '%sActivity' % project
        transformed_package = conf.get_android_package_name(project).replace('.', '/')

        template = {
            'ANDROID_PACKAGE' : conf.get_android_package_name(project),
            'ANDROID_PACKAGE_PATH' : transformed_package,

            'ANDROID_APP_NAME' : conf.get_launcher_product_name(project),    # external facing name
            'ANDROID_PROJECT_NAME' : project,                                # internal facing name

            'ANDROID_PROJECT_ACTIVITY' : activity_name,
            'ANDROID_LAUNCHER_NAME' : conf.get_executable_name(project),     # first native library to load from java

            'ANDROID_VERSION_NUMBER' : conf.get_android_version_number(project),
            'ANDROID_VERSION_NAME' : conf.get_android_version_name(project),

            'ANDROID_SCREEN_ORIENTATION' : conf.get_android_orientation(project),

            'ANDROID_APP_PUBLIC_KEY' : conf.get_android_app_public_key(project),
            'ANDROID_APP_OBFUSCATOR_SALT' : conf.get_android_app_obfuscator_salt(project),

            'ANDROID_USE_MAIN_OBB' : conf.get_android_use_main_obb(project),
            'ANDROID_USE_PATCH_OBB' : conf.get_android_use_patch_obb(project),
            'ANDROID_ENABLE_KEEP_SCREEN_ON' : conf.get_android_enable_keep_screen_on(project),

            'ANDROID_MIN_SDK_VERSION' : conf.env['ANDROID_NDK_PLATFORM_NUMBER'],
            'ANDROID_TARGET_SDK_VERSION' : conf.env['ANDROID_SDK_VERSION_NUMBER'],
        }

        # update the builder file with the correct package name
        transformed_node = builder_file_dest.find_or_declare('%s_builder.json' % project)

        transformed_text = string.Template(builder_file_src.read()).substitute(template)
        transformed_node.write(transformed_text)

        # process the builder file and create project
        copied_files = []
        json_data = conf.parse_json_file(transformed_node)
        process_json(conf, json_data, proj_root, source_node, template, copied_files)

        # resolve the application icon overrides
        resource_node = proj_root.make_node(['src', 'main', 'res'])

        icon_overrides = conf.get_android_app_icons(project)
        if icon_overrides is not None:
            mipmap_path_prefix = 'mipmap'

            # if a default icon is specified, then copy it into the generic mipmap folder
            default_icon = icon_overrides.get('default', None)

            if default_icon is not None:
                default_icon_source_node = construct_source_path(conf, project, default_icon)

                default_icon_target_dir = resource_node.make_node(mipmap_path_prefix)
                default_icon_target_dir.mkdir()

                dest_file = os.path.join(default_icon_target_dir.abspath(), APP_ICON_NAME)
                shutil.copyfile(default_icon_source_node, dest_file)
                os.chmod(dest_file, FILE_PERMISSIONS)
                copied_files.append(dest_file)

            else:
                Logs.debug('android: No default icon override specified for %s' % project)

            # process each of the resolution overrides
            for resolution in RESOLUTION_SETTINGS:
                target_directory = resource_node.make_node(mipmap_path_prefix + '-' + resolution)

                # get the current resolution icon override
                icon_source = icon_overrides.get(resolution, default_icon)
                if icon_source is default_icon:

                    # if both the resolution and the default are unspecified, warn the user but do nothing
                    if icon_source is None:
                        Logs.warn('[WARN] No icon override found for "%s".  Either supply one for "%s" or a "default" in the android_settings "icon" section of the project.json file for %s' % (resolution, resolution, project))

                    # if only the resoultion is unspecified, remove the resolution specific version from the project
                    else:
                        Logs.debug('android: Default icon being used for "%s" in %s' % (resolution, project))
                        remove_file_and_empty_directory(target_directory.abspath(), APP_ICON_NAME)

                    continue

                icon_source_node = construct_source_path(conf, project, icon_source)
                icon_target_node = target_directory.make_node(APP_ICON_NAME)

                shutil.copyfile(icon_source_node, icon_target_node.abspath())
                icon_target_node.chmod(FILE_PERMISSIONS)
                copied_files.append(icon_target_node.abspath())

        # resolve the application splash screen overrides
        splash_overrides = conf.get_android_app_splash_screens(project)
        if splash_overrides is not None:
            drawable_path_prefix = 'drawable-'

            for orientation in ('land', 'port'):
                orientation_path_prefix = drawable_path_prefix + orientation

                oriented_splashes = splash_overrides.get(orientation, {})

                # if a default splash image is specified for this orientation, then copy it into the generic drawable-<orientation> folder
                default_splash_img = oriented_splashes.get('default', None)

                if default_splash_img is not None:
                    default_splash_img_source_node = construct_source_path(conf, project, default_splash_img)

                    default_splash_img_target_dir = resource_node.make_node(orientation_path_prefix)
                    default_splash_img_target_dir.mkdir()

                    dest_file = os.path.join(default_splash_img_target_dir.abspath(), APP_SPLASH_NAME)
                    shutil.copyfile(default_splash_img_source_node, dest_file)
                    os.chmod(dest_file, FILE_PERMISSIONS)
                    copied_files.append(dest_file)

                else:
                    Logs.debug('android: No default splash screen override specified for "%s" orientation in %s' % (orientation, project))

                # process each of the resolution overrides
                for resolution in RESOLUTION_SETTINGS:
                    # The xxxhdpi resolution is only for application icons, its overkill to include them for drawables... for now
                    if resolution == 'xxxhdpi':
                        continue

                    target_directory = resource_node.make_node(orientation_path_prefix + '-' + resolution)

                    # get the current resolution splash image override
                    splash_img_source = oriented_splashes.get(resolution, default_splash_img)
                    if splash_img_source is default_splash_img:

                        # if both the resolution and the default are unspecified, warn the user but do nothing
                        if splash_img_source is None:
                            section = "%s-%s" % (orientation, resolution)
                            Logs.warn('[WARN] No splash screen override found for "%s".  Either supply one for "%s" or a "default" in the android_settings "splash_screen-%s" section of the project.json file for %s' % (section, resolution, orientation, project))

                        # if only the resoultion is unspecified, remove the resolution specific version from the project
                        else:
                            Logs.debug('android: Default icon being used for "%s-%s" in %s' % (orientation, resolution, project))
                            remove_file_and_empty_directory(target_directory.abspath(), APP_SPLASH_NAME)

                        continue

                    splash_img_source_node = construct_source_path(conf, project, splash_img_source)
                    splash_img_target_node = target_directory.make_node(APP_SPLASH_NAME)

                    shutil.copyfile(splash_img_source_node, splash_img_target_node.abspath())
                    splash_img_target_node.chmod(FILE_PERMISSIONS)
                    copied_files.append(splash_img_target_node.abspath())

        # additional optimization to only include the splash screens for the avaiable orientations allowed by the manifest
        requested_orientation = conf.get_android_orientation(project)

        if requested_orientation in ('landscape', 'reverseLandscape', 'sensorLandscape', 'userLandscape'):
            Logs.debug('android: Clearing the portrait assets from %s' % project)
            clear_splash_assets(resource_node, 'drawable-port')

        elif requested_orientation in ('portrait', 'reversePortrait', 'sensorPortrait', 'userPortrait'):
            Logs.debug('android: Clearing the landscape assets from %s' % project)
            clear_splash_assets(resource_node, 'drawable-land')

        # delete all files from the destination folder that were not copied by the script
        all_files = proj_root.ant_glob("**", excl=['wscript', 'build.gradle', 'assets_for_apk/*'])
        files_to_delete = [path for path in all_files if path.abspath() not in copied_files]

        for file in files_to_delete:
            file.delete()

    # add all the projects to the root wscript
    android_wscript = android_root.make_node('wscript')
    with open(android_wscript.abspath(), 'w') as wscript_file:
        w = wscript_file.write

        w(AUTO_GEN_HEADER_PYTHON)

        w('SUBFOLDERS = [\n')
        w('\t\'%s\'\n]\n\n' % '\',\n\t\''.join(created_directories))

        w('def build(bld):\n')
        w('\tvalid_subdirs = [x for x in SUBFOLDERS if bld.path.find_node("%s/wscript" % x)]\n')
        w('\tbld.recurse(valid_subdirs)\n')

    # Some Android SDK libraries have bugs, so we need to copy them locally and patch them.
    if not copy_and_patch_android_libraries(conf, source_node, android_root):
        return False

    return True


################################################################
@conf
def is_module_for_game_project(self, module_name, game_project, project_name):
    """
    Check to see if the task generator is part of the build for a particular game project.
    The following rules apply:
        1. It is a gem requested by the game project
        2. It is the game project / project's launcher
        3. It is part of the general modules list
    """
    enabled_game_projects = self.get_enabled_game_project_list()

    if self.is_gem(module_name):
        gem_name_list = [gem.name for gem in self.get_game_gems(game_project)]
        return (True if module_name in gem_name_list else False)

    elif module_name == game_project or game_project == project_name:
        return True

    elif module_name not in enabled_game_projects and project_name is None:
        return True

    return False


################################################################
def collect_source_paths(android_task, src_path_tag):

    game_project = android_task.game_project
    bld = android_task.bld

    platform = bld.env['PLATFORM']
    config = bld.env['CONFIGURATION']

    search_tags = [
        'android_{}'.format(src_path_tag),
        'android_{}_{}'.format(config, src_path_tag),

        '{}_{}'.format(platform, src_path_tag),
        '{}_{}_{}'.format(platform, config, src_path_tag),
    ]

    source_paths = []
    for group in bld.groups:
        for task_generator in group:
            if not isinstance(task_generator, TaskGen.task_gen):
                continue

            Logs.debug('android: Processing task %s' % task_generator.name)

            if not (getattr(task_generator, 'posted', None) and getattr(task_generator, 'link_task', None)):
                Logs.debug('android:  -> Task is NOT posted, Skipping...')
                continue

            project_name = getattr(task_generator, 'project_name', None)
            if not bld.is_module_for_game_project(task_generator.name, game_project, project_name):
                Logs.debug('android:  -> Task is NOT part of the game project, Skipping...')
                continue

            raw_paths = []
            for tag in search_tags:
                raw_paths += getattr(task_generator, tag, [])

            Logs.debug('android:  -> Raw Source Paths %s' % raw_paths)

            for path in raw_paths:
                if os.path.isabs(path):
                    path = bld.root.make_node(path)
                else:
                    path = task_generator.path.make_node(path)
                source_paths.append(path)

    return source_paths


################################################################
def get_resource_compiler_path(ctx):
    if Utils.unversioned_sys_platform() == "win32":
        paths = ['Bin64vc140', 'Bin64vc120', 'Bin64']
    else:
        paths = ['BinMac64', 'Bin64']
    rc_search_paths = [os.path.join(ctx.path.abspath(), path, 'rc') for path in paths]
    try:
        return ctx.find_program('rc', path_list = rc_search_paths, silent_output = True)
    except:
        ctx.fatal('[ERROR] Failed to find the Resource Compiler in paths: {}'.format(paths))


def get_python_path(ctx):
    python_cmd = 'python'

    if Utils.unversioned_sys_platform() == "win32":
        python_cmd = '"{}"'.format(ctx.path.find_resource('Tools/Python/python.cmd').abspath())

    return python_cmd


def set_key_and_store_pass(ctx):
    if ctx.get_android_build_environment() == 'Distribution':
        key_pass = ctx.options.distro_key_pass
        store_pass = ctx.options.distro_store_pass

        if not (key_pass and store_pass):
            ctx.fatal('[ERROR] Build environment is set to Distribution but --distro-key-pass or --distro-store-pass arguments were not specified or blank')

    else:
        key_pass = ctx.options.dev_key_pass
        store_pass = ctx.options.dev_store_pass

    ctx.env['KEYPASS'] = key_pass
    ctx.env['STOREPASS'] = store_pass


################################################################
################################################################
class strip_debug_symbols(Task):
    """
    Strips the debug symbols from a shared library
    """
    color = 'CYAN'
    run_str = "${STRIP} --strip-debug -o ${TGT} ${SRC}"
    vars = [ 'STRIP' ]

    def runnable_status(self):
        if super(strip_debug_symbols, self).runnable_status() == ASK_LATER:
            return ASK_LATER

        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # If the target file is missing, we need to run
        try:
            stat_tgt = os.stat(tgt)
        except OSError:
            return RUN_ME

        # Now compare both file stats
        try:
            stat_src = os.stat(src)
        except OSError:
            pass
        else:
            CREATION_TIME_PADDING = 10

            # only check timestamps
            if stat_src.st_mtime >= (stat_tgt.st_mtime + CREATION_TIME_PADDING):
                return RUN_ME

        # Everything fine, we can skip this task
        return SKIP_ME


################################################################
class aapt_package_base(Task):
    """
    General purpose 'package' variant Android Asset Packaging Tool task
    """
    color = 'PINK'
    vars = [ 'AAPT', 'AAPT_RESOURCES', 'AAPT_INCLUDES', 'AAPT_PACKAGE_FLAGS' ]


    def runnable_status(self):

        def _to_list(value):
            if isinstance(value, list):
                return value
            else:
                return [ value ]

        if not self.inputs:
            self.inputs  = []

            aapt_resources = getattr(self.generator, 'aapt_resources', [])
            assets = getattr(self, 'assets', [])
            apk_layout = getattr(self, 'srcdir', [])

            input_paths = _to_list(aapt_resources) + _to_list(assets) + _to_list(apk_layout)

            for path in input_paths:
                files = path.ant_glob('**/*')
                self.inputs.extend(files)

            android_manifest = getattr(self.generator, 'main_android_manifest', None)
            if android_manifest:
                self.inputs.append(android_manifest)

        result = super(aapt_package_base, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
class android_code_gen(aapt_package_base):
    """
    Generates the R.java files from the Android resources
    """
    run_str = '${AAPT} package -f -M ${ANDROID_MANIFEST} ${AAPT_RESOURCE_ST:AAPT_RESOURCES} ${AAPT_INLC_ST:AAPT_INCLUDES} ${AAPT_PACKAGE_FLAGS} -m -J ${OUTDIR}'


################################################################
class package_resources(aapt_package_base):
    """
    Packages all the native resources from the Android project
    """
    run_str = '${AAPT} package -f ${ANDROID_DEBUG_MODE} -M ${ANDROID_MANIFEST} ${AAPT_RESOURCE_ST:AAPT_RESOURCES} ${AAPT_INLC_ST:AAPT_INCLUDES} ${AAPT_PACKAGE_FLAGS} -F ${TGT}'


################################################################
class build_apk(aapt_package_base):
    """
    Generates an unsigned, unaligned Android APK
    """
    run_str = '${AAPT} package -f ${ANDROID_DEBUG_MODE} -M ${ANDROID_MANIFEST} ${AAPT_RESOURCE_ST:AAPT_RESOURCES} ${AAPT_INLC_ST:AAPT_INCLUDES} ${AAPT_ASSETS_ST:AAPT_ASSETS} ${AAPT_PACKAGE_FLAGS} -F ${TGT} ${SRCDIR}'


################################################################
class aapt_crunch(Task):
    """
    Processes the PNG resources from the Android project
    """
    color = 'PINK'
    run_str = '${AAPT} crunch ${AAPT_RESOURCE_ST:AAPT_RESOURCES} -C ${TGT}'
    vars = [ 'AAPT', 'AAPT_RESOURCES' ]


    def runnable_status(self):
        if not self.inputs:
            self.inputs  = []

            for resource in self.generator.aapt_resources:
                res = resource.ant_glob('**/*')
                self.inputs.extend(res)

        return super(aapt_crunch, self).runnable_status()


################################################################
class aidl(Task):
    """
    Processes the Android interface files
    """
    color = 'PINK'
    run_str = '${AIDL} ${AIDL_PREPROC_ST:AIDL_PREPROCESSES} ${SRC} ${TGT}'


    def runnable_status(self):
        result = super(aidl, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
class dex(Task):
    """
    Compiles the .class files into the dalvik executable
    """
    color = 'PINK'
    run_str = '${DX} --dex --output ${TGT} ${JAR_INCLUDES} ${SRCDIR}'


    def runnable_status(self):

        for tsk in self.run_after:
            if not tsk.hasrun:
                return ASK_LATER

        if not self.inputs:
            self.inputs  = []

            srcdir = self.srcdir
            if not isinstance(srcdir, list):
                srcdir = [ srcdir ]

            for src_node in srcdir:
                self.inputs.extend(src_node.ant_glob('**/*.class', remove = False, quiet = True))

        result = super(dex, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
class zipalign(Task):
    """
    Performs a specified byte alignment on the source file
    """
    color = 'PINK'
    run_str = '${ZIPALIGN} -f ${ZIPALIGN_SIZE} ${SRC} ${TGT}'

    def runnable_status(self):
        result = super(zipalign, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
################################################################
@taskgen_method
def create_debug_strip_task(self, source_file, dest_location):

    lib_name = os.path.basename(source_file.abspath())
    output_node = dest_location.make_node(lib_name)

    # For Android Studio we should just copy the libs because stripping is part of the build process.
    # But we have issues with long path names that makes the stripping process to fail in Android Studio.
    self.create_task('strip_debug_symbols', source_file, output_node)


################################################################
@feature('c', 'cxx')
@after_method('apply_link')
def add_android_natives_processing(self):
    if 'android' not in self.env['PLATFORM']:
        return

    if not getattr(self, 'link_task', None):
        return

    if self._type == 'stlib': # Do not copy static libs
        return

    output_node = self.bld.get_output_folders(self.bld.env['PLATFORM'], self.bld.env['CONFIGURATION'])[0]

    project_name = getattr(self, 'project_name', None)
    game_projects = self.bld.get_enabled_game_project_list()

    for src in self.link_task.outputs:
        src_lib = output_node.make_node(src.name)

        for game in game_projects:

            game_build_native_key = '%s_BUILDER_NATIVES' % game

            # If the game is a valid android project, a specific build native value will have been created during
            # the project configuration.  Only process games with valid android project settings
            if not game_build_native_key in self.env:
                continue
            game_build_native_node = self.bld.root.find_dir(self.env[game_build_native_key])

            if self.bld.is_module_for_game_project(self.name, game, project_name):
                self.create_debug_strip_task(src_lib, game_build_native_node)


################################################################
@feature('c', 'cxx', 'copy_3rd_party_binaries')
@after_method('apply_link')
def add_3rd_party_library_stripping(self):
    """
    Strip and copy 3rd party shared libraries so they are included into the APK.
    """
    if 'android' not in self.env['PLATFORM'] or self.bld.spec_monolithic_build():
        return

    third_party_artifacts = self.env['COPY_3RD_PARTY_ARTIFACTS']
    if third_party_artifacts:
        game_projects = self.bld.get_enabled_game_project_list()
        for source_node in third_party_artifacts:
            _, ext = os.path.splitext(source_node.abspath())
            # Only care about shared libraries
            if ext == '.so':
                for game in game_projects:
                    game_build_native_key = '%s_BUILDER_NATIVES' % game

                    # If the game is a valid android project, a specific build native value will have been created during
                    # the project configuration.  Only process games with valid android project settings
                    if not game_build_native_key in self.env:
                        continue

                    game_build_native_node = self.bld.root.find_dir(self.env[game_build_native_key])
                    self.create_debug_strip_task(source_node, game_build_native_node)


################################################################
################################################################
@feature('wrapped_copy_outputs')
@before_method('process_source')
def create_copy_outputs(self):
    self.meths.remove('process_source')
    self.create_task('copy_outputs', self.source, self.target)


@taskgen_method
def sign_and_align_apk(self, base_apk_name, raw_apk, intermediate_folder, final_output, suffix = ''):
    # first sign the apk with jarsigner
    apk_name = '{}_unaligned{}.apk'.format(base_apk_name, suffix)
    unaligned_apk = intermediate_folder.make_node(apk_name)

    self.jarsign_task = jarsign_task = self.create_task('jarsigner', raw_apk, unaligned_apk)

    # align the new apk with assets
    apk_name = '{}{}.apk'.format(base_apk_name, suffix)
    final_apk = final_output.make_node(apk_name)

    self.zipalign_task = zipalign_task = self.create_task('zipalign', unaligned_apk, final_apk)

    # chain the alignment to happen after signing
    zipalign_task.set_run_after(jarsign_task)


################################################################
################################################################
@conf
def AndroidAPK(ctx, *k, **kw):

    project_name = kw['project_name']

    if ctx.cmd in ('configure', 'generate_uber_files', 'generate_module_def_files', 'msvs'):
        return 
    if ctx.env['PLATFORM'] not in ('android_armv7_clang', 'android_armv7_gcc', 'project_generator'):
        return
    if project_name not in ctx.get_enabled_game_project_list():
        return

    Logs.debug('android: ******************************** ')
    Logs.debug('android: Processing {}...'.format(project_name))

    root_input = ctx.path.get_src().make_node('src')
    root_output = ctx.path.get_bld()

    env = ctx.env

    platform = env['PLATFORM']
    configuration = env['CONFIGURATION']

    apk_layout_dir = root_output.make_node('builder')

    # The variant name is constructed in the same fashion as how Gradle generates all it's build
    # variants.  After all the Gradle configurations and product flavors are evaluated, the variants
    # are generated in the following lower camel case format {product_flavor}{configuration}.
    # Our configuration and Gradle's configuration is a one to one mapping of what each describe,
    # while our platform is effectively Gradle's product flavor.
    gradle_variant = '{}{}'.format(platform, configuration.title())

    # copy over the required 3rd party libs that need to be bundled into the apk
    abi = env['ANDROID_ARCH']
    if not abi:
        abi = 'armeabi-v7a'

    if ctx.options.from_android_studio:
        local_native_libs_node = root_input.make_node([ gradle_variant, 'jniLibs', abi ])
    else:
        local_native_libs_node = apk_layout_dir.make_node([ 'lib', abi ])

    local_native_libs_node.mkdir()
    Logs.debug('android: APK builder path (native libs) -> {}'.format(local_native_libs_node.abspath()))
    env['{}_BUILDER_NATIVES'.format(project_name)] = local_native_libs_node.abspath()

    android_cache = get_android_cache_node(ctx)
    libs_to_copy = env['EXT_LIBS']
    for lib in libs_to_copy:

        src = android_cache.make_node(lib)

        lib_name = os.path.basename(lib)
        tgt = local_native_libs_node.make_node(lib_name)

        ctx(features = 'wrapped_copy_outputs', source = src, target = tgt)

    # since we are having android studio building the apk we can kick out early
    if ctx.options.from_android_studio:
        return

    # order of precedence from highest (primary) to lowest (inputs): full variant, build type, product flavor, main
    local_variant_dirs = [ gradle_variant, configuration, platform, 'main' ]

    java_source_nodes = []

    android_manifests = []
    resource_nodes = []

    for source_dir in local_variant_dirs:

        java_node = root_input.find_node([ source_dir, 'java' ])
        if java_node:
            java_source_nodes.append(java_node)

        res_node = root_input.find_node([ source_dir, 'res' ])
        if res_node:
            resource_nodes.append(res_node)

        manifest_node = root_input.find_node([ source_dir, 'AndroidManifest.xml' ])
        if manifest_node:
            android_manifests.append(manifest_node)

    if not android_manifests:
        ctx.fatal('[ERROR] Unable to find any AndroidManifest.xml files in project path {}.'.format(ctx.path.get_src().abspath()))

    Logs.debug('android: Found local Java source directories {}'.format(java_source_nodes))
    Logs.debug('android: Found local resouce directories {}'.format(resource_nodes))
    Logs.debug('android: Found local manifest file {}'.format(android_manifests))

    # get the keystore passwords
    set_key_and_store_pass(ctx)

    # Private function to add android libraries to the build
    def _add_library(folder, libName, source_paths, manifests, package_names, resources):
        '''
        Collect the resources and package names of the specified library.
        '''
        if not folder:
            Logs.error('[ERROR] Invalid folder for library {}. Please check the path in the {} file.'.format(libName, java_libs_json.abspath()))
            return False

        src = folder.find_dir('src')
        if not src:
            Logs.error("[ERROR] Could not find the 'src' folder for library {}. Please check that they are present at {}".format(libName, folder.abspath()))
            return False

        source_paths.append(src)

        manifest =  folder.find_node('AndroidManifest.xml')
        if not manifest:
            Logs.error("[ERROR] Could not find the AndroidManifest.xml folder for library {}. Please check that they are present at {}".format(libName, folder.abspath()))
            return False

        manifests.append(manifest)

        tree = ET.parse(manifest.abspath())
        root = tree.getroot()
        package = root.get('package')
        if not package:
            Logs.error("[ERROR] Could not find 'package' node in {}. Please check that the manifest is valid ".format(manifest.abspath()))
            return False

        package_names.append(package)

        res = folder.find_dir('res')
        if res:
            resources.append(res)

        return True

    library_packages = []
    library_jars = []

    java_libs_json = ctx.root.make_node(kw['android_java_libs'])
    json_data = ctx.parse_json_file(java_libs_json)
    if json_data:
        for libName, value in json_data.iteritems():
            if 'libs' in value:
                # Collect any java lib that is needed so we can add it to the classpath.
                for java_lib in value['libs']:
                    jar_path = string.Template(java_lib['path']).substitute(env)
                    if os.path.exists(jar_path):
                        library_jars.append(jar_path)
                    elif java_lib['required']:
                        ctx.fatal('[ERROR] Required java lib - {} - was not found'.format(jar_path))

            if 'patches' in value:
                lib_path = os.path.join(ctx.Path(ctx.get_android_patched_libraries_relative_path()), libName)
            else:
                # Search the multiple library paths where the library can be
                lib_path = None
                for path in value['srcDir']:
                    path = string.Template(path).substitute(env)
                    if os.path.exists(path):
                        lib_path = path
                        break

            if not _add_library(ctx.root.make_node(lib_path), libName, java_source_nodes, android_manifests, library_packages, resource_nodes):
                ctx.fatal('[ERROR] Could not add the android library - {}'.format(libName))

    r_java_out = root_output.make_node('r')
    aidl_out = root_output.make_node('aidl')

    java_out = root_output.make_node('classes')
    crunch_out = root_output.make_node('res')

    manifest_merger_out = root_output.make_node('manifest')

    game_package = ctx.get_android_package_name(project_name)
    executable_name = ctx.get_executable_name(project_name)

    Logs.debug('android: ****')
    Logs.debug('android: All Java source directories {}'.format(java_source_nodes))
    Logs.debug('android: All resouce directories {}'.format(resource_nodes))

    java_include_paths = java_source_nodes + [ r_java_out, aidl_out ]
    java_source_paths = java_source_nodes

    uses = kw.get('use', [])
    if not isinstance(uses, list):
        uses = [ uses ]

    ################################
    # Push all the Android apk packaging into their own build groups with
    # lazy posting to ensure they are processed at the end of the build
    ctx.post_mode = POST_LAZY
    build_group_name = '{}_android_group'.format(project_name)
    ctx.add_group(build_group_name)

    ctx(
        name = '{}_APK'.format(project_name),
        target = executable_name,

        features = [ 'android', 'android_apk', 'javac', 'use', 'uselib' ],
        use = uses,

        game_project = project_name,

        # java settings
        compat          = env['JAVA_VERSION'],  # java compatibility version number
        classpath       = library_jars,

        srcdir          = java_include_paths,   # folder containing the sources to compile
        outdir          = java_out,             # folder where to output the classes (in the build directory)

        # android settings
        android_manifests = android_manifests,
        android_package = game_package,

        aidl_outdir = aidl_out,
        r_java_outdir = r_java_out,

        manifest_merger_out = manifest_merger_out,

        apk_layout_dir = apk_layout_dir,
        apk_native_lib_dir = local_native_libs_node,
        apk_output_dir = 'apk',

        aapt_assets = [],
        aapt_resources = resource_nodes,

        aapt_extra_packages = library_packages,

        aapt_package_flags = [],
        aapt_package_resources_outdir = 'bin',

        aapt_crunch_outdir = crunch_out,
    )

    # reset the build group/mode back to default
    ctx.post_mode = POST_AT_ONCE
    ctx.set_group('regular_group')


################################################################
@feature('android')
@before('apply_java')
def apply_android_java(self):
    """
    Generates the AIDL tasks for all other tasks that may require it, and adds
    their Java source directories to the current projects Java source paths
    so they all get processed at the same time.  Also processes the direct
    Android Archive Resource uses.
    """

    Utils.def_attrs(
        self,

        srcdir = [],
        classpath = [],

        aidl_srcdir = [],

        aapt_assets = [],
        aapt_includes = [],
        aapt_resources = [],
        aapt_extra_packages = [],
    )

    # validate we have some required attributes
    apk_native_lib_dir = getattr(self, 'apk_native_lib_dir', None)
    if not apk_native_lib_dir:
        self.fatal('[ERROR] No "apk_native_lib_dir" specified in Android package task.')

    android_manifests = getattr(self, 'android_manifests', None)
    if not android_manifests:
        self.fatal('[ERROR] No "android_manifests" specified in Android package task.')

    manifest_nodes = []

    for manifest in android_manifests:
        if not isinstance(manifest, Node.Node):
            manifest_nodes.append(self.path.get_src().make_node(manifest))
        else:
            manifest_nodes.append(manifest)

    self.android_manifests = manifest_nodes
    self.main_android_manifest = manifest_nodes[0] # guaranteed to be the main; manifests are added in order of precedence highest to lowest

    # process the uses, only first level uses are supported at this time
    libs = self.to_list(getattr(self, 'use', []))
    Logs.debug('android: -> Processing Android libs used by APK {}'.format(libs))

    input_manifests = []
    use_libs_added = []

    for lib_name in libs:
        try:
            task_gen = self.bld.get_tgen_by_name(lib_name)
            task_gen.post()
        except Exception:
            continue
        else:
            if not hasattr(task_gen, 'aar_task'):
                continue

            use_libs_added.append(lib_name)

            # required entries from the library
            append_to_unique_list(self.aapt_extra_packages, task_gen.package)
            append_to_unique_list(self.aapt_includes, task_gen.jar_task.outputs[0].abspath())
            append_to_unique_list(self.aapt_resources, task_gen.aapt_resources)

            append_to_unique_list(input_manifests, task_gen.manifest)

            # optional entries from the library
            if task_gen.aapt_assets:
                append_to_unique_list(self.aapt_assets, task_gen.aapt_assets)

            # since classpath is propagated by the java tool, we just need to make sure the jars are propagated to the android specific tools using aapt_includes
            if task_gen.classpath:
                append_to_unique_list(self.aapt_includes, task_gen.classpath)

            if task_gen.native_libs:
                native_libs_root = task_gen.native_libs_root
                native_libs = task_gen.native_libs

                for lib in native_libs:
                    rel_path = lib.path_from(native_libs_root)

                    tgt = apk_native_lib_dir.make_node(rel_path)

                    strip_task = self.create_task('strip_debug_symbols', lib, tgt)
                    self.bld.add_to_group(strip_task, 'regular_group')

    Logs.debug('android: -> Android use libs added {}'.format(use_libs_added))

    # generate the task to merge the manifests
    manifest_nodes.extend(input_manifests)

    if len(manifest_nodes) >= 2:
        manifest_merger_out = getattr(self, 'manifest_merger_out', None)
        if manifest_merger_out:
            if not isinstance(manifest_merger_out, Node.Node):
                manifest_merger_out = self.path.get_bld().make_node(manifest_merger_out)
        else:
            manifest_merger_out = self.path.get_bld().make_node('manifest')
        manifest_merger_out.mkdir()

        merged_manifest = manifest_merger_out.make_node('AndroidManifest.xml')
        self.manifest_task = manifest_task = self.create_task('android_manifest_merger', manifest_nodes, merged_manifest)

        manifest_task.env['MAIN_MANIFEST'] = manifest_nodes[0].abspath()

        input_manifest_paths = [ manifest.abspath() for manifest in manifest_nodes[1:] ]
        if manifest_task.env['MANIFEST_MERGER_OLD_VERSION']:
            manifest_task.env['LIBRARY_MANIFESTS'] = input_manifest_paths
        else:
            manifest_task.env['LIBRARY_MANIFESTS'] = os.pathsep.join(input_manifest_paths)

        self.main_android_manifest = merged_manifest

    # generate all the aidl tasks
    aidl_outdir = getattr(self, 'aidl_outdir', None)
    if aidl_outdir:
        if not isinstance(aidl_outdir, Node.Node):
            aidl_outdir = self.path.get_bld().make_node(aidl_outdir)
    else:
        aidl_outdir = self.path.get_bld().make_node('aidl')
    aidl_outdir.mkdir()

    aidl_src_paths = collect_source_paths(self, 'aidl_src_path')
    self.aidl_tasks = []

    for srcdir in aidl_src_paths:
        for aidl_file in srcdir.ant_glob('**/*.aidl'):
            rel_path = aidl_file.path_from(srcdir)

            java_file = aidl_outdir.make_node('{}.java'.format(os.path.splitext(rel_path)[0]))

            aidl_task = self.create_task('aidl', aidl_file, java_file)
            self.aidl_tasks.append(aidl_task)

    java_src_paths = collect_source_paths(self, 'java_src_path')
    append_to_unique_list(self.srcdir, java_src_paths)

    jars = collect_source_paths(self, 'jars')
    append_to_unique_list(self.classpath, jars)

    Logs.debug('android: -> Additional Java source paths found {}'.format(java_src_paths))


################################################################
@feature('android')
@before_method('process_source')
@after_method('apply_java')
def apply_android(self):
    """
    Generates the code generation task (produces R.java) and setups the task chaining
    for AIDL, Java and the code gen task
    """

    Utils.def_attrs(
        self,

        classpath = [],

        aapt_resources = [],
        aapt_includes = [],
        aapt_extra_packages = [],
        aapt_package_flags = [],
    )

    main_package = getattr(self, 'android_package', None)
    if not main_package:
        self.fatal('[ERROR] No "android_package" specified in Android package task.')

    javac_task = getattr(self, 'javac_task', None)
    if not javac_task:
        self.fatal('[ERROR] It seems the "javac" task failed to be generated, unable to complete the Android build process.')

    self.code_gen_task = code_gen_task = self.create_task('android_code_gen')

    r_java_outdir = getattr(self, 'r_java_outdir', None)
    if r_java_outdir:
        if not isinstance(r_java_outdir, Node.Node):
            r_java_outdir = self.path.get_bld().make_node(r_java_outdir)
    else:
        r_java_outdir = self.path.get_bld().make_node('r')
    r_java_outdir.mkdir()

    code_gen_task.env['OUTDIR'] = r_java_outdir.abspath()

    android_manifest = self.main_android_manifest
    code_gen_task.env['ANDROID_MANIFEST'] = android_manifest.abspath()

    # resources

    aapt_resources = []
    for resource in self.aapt_resources:
        if isinstance(resource, Node.Node):
            aapt_resources.append(resource.abspath())
        else:
            aapt_resources.append(resource)

    self.aapt_resource_paths = aapt_resources
    code_gen_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    # included jars

    aapt_includes = self.aapt_includes + self.classpath

    aapt_include_paths = []

    for include_path in self.aapt_includes:
        if isinstance(include_path, Node.Node):
            aapt_include_paths.append(include_path.abspath())
        else:
            aapt_include_paths.append(include_path)

    self.aapt_include_paths = aapt_include_paths
    code_gen_task.env.append_value('AAPT_INCLUDES', aapt_include_paths)

    # additional flags

    aapt_package_flags = self.aapt_package_flags

    extra_packages = self.aapt_extra_packages
    if extra_packages:
        aapt_package_flags.extend([ '--extra-packages', ':'.join(extra_packages) ])

    code_gen_task.env.append_value('AAPT_PACKAGE_FLAGS', aapt_package_flags)

    # outputs (R.java files)

    included_packages = [ main_package ] + extra_packages

    output_nodes = []
    for package in included_packages:
        sub_dirs = package.split('.')
        dir_path = os.path.join(*sub_dirs)

        r_java_path = os.path.join(dir_path, 'R.java')
        r_java_node = r_java_outdir.make_node(r_java_path)

        output_nodes.append(r_java_node)

    code_gen_task.set_outputs(output_nodes)

    # task chaining

    manifest_task = getattr(self, 'manifest_task', None)
    if manifest_task:
        code_gen_task.set_run_after(manifest_task)

    aidl_tasks = getattr(self, 'aidl_tasks', [])
    for aidl_task in aidl_tasks:
        code_gen_task.set_run_after(aidl_task)

    javac_task.set_run_after(self.code_gen_task)


################################################################
@feature('android_apk')
@after_method('apply_android')
def apply_android_apk(self):
    """
    Generates the rest of the tasks necessary for building an APK (dex, crunch, package, build, sign, alignment).
    """

    Utils.def_attrs(
        self,

        aapt_assets = [],
        aapt_include_paths = [],
        aapt_resource_paths = [],

        aapt_package_flags = [],
    )

    root_input = self.path.get_src()
    root_output = self.path.get_bld()

    if not hasattr(self, 'target'):
        self.target = self.name
    executable_name = self.target

    aapt_resources = self.aapt_resource_paths
    aapt_includes = self.aapt_include_paths

    aapt_assets = []
    asset_nodes = []
    for asset_dir in self.aapt_assets:
        if isinstance(asset_dir, Node.Node):
            aapt_assets.append(asset_dir.abspath())
            asset_nodes.append(asset_dir)
        else:
            aapt_assets.append(asset_dir)
            asset_nodes.append(root_input.make_node(asset_dir))

    android_manifest = self.android_manifests[0]
    if hasattr(self, 'manifest_task'):
        android_manifest = self.manifest_task.outputs[0]

    # dex task

    apk_layout_dir = getattr(self, 'apk_layout_dir', None)
    if apk_layout_dir:
        if not isinstance(apk_layout_dir, Node.Node):
            apk_layout_dir = self.path.get_bld().make_node(apk_layout_dir)
    else:
        apk_layout_dir = root_output.make_node('builder')

    apk_layout_dir.mkdir()

    self.dex_task = dex_task = self.create_task('dex')
    self.dex_task.set_outputs(apk_layout_dir.make_node('classes.dex'))

    dex_task.env.append_value('JAR_INCLUDES', aapt_includes)

    dex_srcdir = self.outdir
    dex_task.env['SRCDIR'] = dex_srcdir.abspath()
    dex_task.srcdir = dex_srcdir

    # crunch task

    self.crunch_task = crunch_task = self.create_task('aapt_crunch')

    crunch_outdir = getattr(self, 'aapt_crunch_outdir', None)
    if crunch_outdir:
        if not isinstance(crunch_outdir, Node.Node):
            crunch_outdir = root_output.make_node(crunch_outdir)
    else:
        crunch_outdir = root_output.make_node('res')
    crunch_outdir.mkdir()

    crunch_task.set_outputs(crunch_outdir)

    crunch_task.env.append_value('AAPT_INCLUDES', aapt_includes)
    crunch_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    # package resources task

    self.package_resources_task = package_resources_task = self.create_task('package_resources')

    aapt_package_resources_outdir = getattr(self, 'aapt_package_resources_outdir', None)
    if aapt_package_resources_outdir:
        if not isinstance(aapt_package_resources_outdir, Node.Node):
            aapt_package_resources_outdir = root_output.make_node(aapt_package_resources_outdir)
    else:
        aapt_package_resources_outdir = root_output.make_node('bin')
    aapt_package_resources_outdir.mkdir()

    package_resources_task.set_outputs(aapt_package_resources_outdir.make_node('{}.ap_'.format(executable_name)))

    package_resources_task.env['ANDROID_MANIFEST'] = android_manifest.abspath()

    package_resources_task.env.append_value('AAPT_INCLUDES', aapt_includes)
    package_resources_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    ################################
    # generate the APK

    # Generating all the APK has to be in the right order.  This is important for Android store APK uploads,
    # if the alignment happens before the signing, then the signing will blow over the alignment and will
    # require a realignment before store upload.
    # 1. Generate the unsigned, unaligned APK
    # 2. Sign the APK
    # 3. Align the APK

    apk_output_dir = getattr(self, 'apk_output_dir', None)
    if apk_output_dir:
        if not isinstance(apk_output_dir, Node.Node):
            apk_output_dir = root_output.make_node(apk_output_dir)
    else:
        apk_output_dir = root_output.make_node('apk')
    apk_output_dir.mkdir()

    # 1. build_apk
    self.apk_task = apk_task = self.create_task('build_apk')

    apk_task.env['SRCDIR'] = apk_layout_dir.abspath()
    apk_task.srcdir = apk_layout_dir

    apk_name = '{}_unaligned_unsigned.apk'.format(executable_name)
    unsigned_unaligned_apk = apk_output_dir.make_node(apk_name)
    apk_task.set_outputs(unsigned_unaligned_apk)

    apk_task.env['ANDROID_MANIFEST'] = android_manifest.abspath()

    apk_task.assets = asset_nodes
    apk_task.env.append_value('AAPT_ASSETS', aapt_assets)

    apk_task.env.append_value('AAPT_INCLUDES', aapt_includes)
    apk_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    # 2. jarsign and 3. zipalign
    final_apk_out = self.bld.get_output_folders(self.bld.env['PLATFORM'], self.bld.env['CONFIGURATION'])[0]
    self.sign_and_align_apk(
        executable_name, # base_apk_name
        unsigned_unaligned_apk, # raw_apk
        apk_output_dir, # intermediate_folder
        final_apk_out # final_output
    )

    # task chaining
    dex_task.set_run_after(self.javac_task)

    crunch_task.set_run_after(dex_task)
    package_resources_task.set_run_after(crunch_task)

    apk_task.set_run_after(package_resources_task)
    self.jarsign_task.set_run_after(apk_task)


###############################################################################
###############################################################################
def adb_call(*cmdArgs, **keywords):
    '''
    Issue a adb command. Args are joined into a single string with spaces
    in between and keyword arguments supported is device=serial # of device
    reported by adb.
    Examples:
    adb_call('start-server') results in "adb start-server" being executed
    adb_call('push', local_path, remote_path, device='123456') results in "adb -s 123456 push <local_path> <remote_path>" being executed
    '''

    command = [ 'adb' ]

    if 'device' in keywords:
        command.extend([ '-s', keywords['device'] ])

    command.extend(cmdArgs)

    cmdline = ' '.join(command)
    Logs.debug('adb_call: running command \'%s\'', cmdline)

    try:
        output = check_output(cmdline, stderr = subprocess.STDOUT, shell = True)
        stripped_output = output.rstrip()

        # don't need to dump the output of 'push' or 'install' commands
        if not any(cmd for cmd in ('push', 'install') if cmd in cmdArgs):
            if '\n' in stripped_output:
                # the newline arg is because Logs.debug will replace newlines with spaces 
                # in the format string before passing it on to the logger
                Logs.debug('adb_call: output = %s%s', '\n', stripped_output)
            else:
                Logs.debug('adb_call: output = %s', stripped_output)

        return stripped_output

    except Exception as inst:
        Logs.debug('adb_call: exception was thrown: ' + str(inst))
        return None  # Return None so the caller can handle the failure gracefully'


def adb_ls(path, device_id, args = [], as_root = False):
    '''
    Special wrapper around calling "adb shell ls <args> <path>".  This uses
    adb_call under the hood but provides some additional error handling specific
    to the "ls" command.  Optionally, this command can be run as super user, or 
    'as_root', which is disabled by default.
    Returns:
        status - if the command failed or not
        output - the stripped output from the ls command
    '''
    error_messages = [
        'No such file or directory',
        'Permission denied'
    ]

    shell_command = [ 'shell' ]

    if as_root:
        shell_command.extend([ 'su', '-c' ])

    shell_command.append('ls')
    shell_command.extend(args)
    shell_command.append(path)

    Logs.debug('adb_ls: {}'.format(shell_command))
    raw_output = adb_call(*shell_command, device = device_id)

    if raw_output is None:
        Logs.debug('adb_ls: No output given')
        return False, None

    if raw_output is None or any([error for error in error_messages if error in raw_output]):
        Logs.debug('adb_ls: Error message found')
        status = False
    else:
        Logs.debug('adb_ls: Command was successful')
        status = True

    return status, raw_output


def get_list_of_android_devices():
    '''
    Gets the connected android devices using the adb command devices and
    returns a list of serial numbers of connected devices.
    '''
    devices = []
    devices_output = adb_call("devices")
    if devices_output is not None:

        devices_output = devices_output.split(os.linesep)
        for output in devices_output:

            if any(x in output for x in ['List', '*', 'emulator']):
                Logs.debug("android: skipping the following line as it has 'List', '*' or 'emulator' in it: %s" % output)
                continue

            device_serial = output.split()
            if device_serial:
                if 'unauthorized' in output.lower():
                    Logs.warn("[WARN] android: device %s is not authorized for development access. Please reconnect the device and check for a confirmation dialog." % device_serial[0])

                else:
                    devices.append(device_serial[0])
                    Logs.debug("android: found device with serial: " + device_serial[0])

    return devices


def get_device_access_type(device_id):
    '''
    Determines what kind of access level we have on the device
    '''
    adb_call('root', device = device_id) # this ends up being either a no-op or restarts adbd on device as root

    adbd_info = adb_call('shell', '"ps | grep adbd"', device = device_id)
    if adbd_info and ('root' in adbd_info):
        Logs.debug('adb_call: Device - {} - has adbd root access'.format(device_id))
        return ACCESS_ROOT_ADBD

    su_test = adb_call('shell', '"su -c echo test"', device = device_id)
    if su_test and ('test' in su_test):
        Logs.debug('adb_call: Device - {} - has shell su access'.format(device_id))
        return ACCESS_SHELL_SU

    Logs.debug('adb_call: Unable to verify root access for device {}.  Assuming default access mode.'.format(device_id))
    return ACCESS_NORMAL


def get_device_file_timestamp(remote_file_path, device_id, as_root = False):
    '''
    Get the integer timestamp value of a file from a given device.  Optionally, this 
    command can be run as super user, or 'as_root', which is disabled by default.
    '''
    timestamp_string = ''
    device_sdk_version = adb_call('shell', 'getprop', 'ro.build.version.sdk', device = device_id)

    # for devices running Android 5.1.1 or under, use the old 'ls' command for getting the file timestame
    if int(device_sdk_version) <= 22:
        ls_status, ls_output = adb_ls(args = [ '-l' ], path = remote_file_path, device_id = device_id, as_root = as_root)
        if ls_status:
            tgt_ls_fields = ls_output.split()
            timestamp_string = '{} {}'.format(tgt_ls_fields[4], tgt_ls_fields[5])
            Logs.debug('android_deploy: ls timestamp %s', timestamp_string)

    # otherwise for newer devices we can use the 'stat' command
    else:
        adb_command = [ 'shell' ]

        if as_root:
            adb_command.extend([ 'su', '-c' ])

        adb_command.extend([ 'stat', '-c', '%y', remote_file_path ])

        file_timestamp = adb_call(*adb_command, device = device_id)
        if file_timestamp and 'No such file or directory' not in file_timestamp:
            # strip the seconds and milliseconds from the time format from the stat command
            timestamp_string = file_timestamp[:file_timestamp.rfind(':')]
            Logs.debug('android_deploy: stat timestamp %s', timestamp_string)

    if timestamp_string:
        target_time = time.mktime(time.strptime(timestamp_string, "%Y-%m-%d %H:%M"))
        Logs.debug('android_deploy: {} time is {}'.format(remote_file_path, target_time))
        return target_time

    return 0


def auto_detect_device_storage_path(device_id, log_warnings = False):
    '''
    Uses the device's environment variable "EXTERNAL_STORAGE" to determine the correct
    path to public storage that has write permissions
    '''
    def _log_debug(message):
        Logs.debug('adb_call: {}'.format(message))

    def _log_warn(message):
        Logs.warn('[WARN] {}'.format(message))

    log_func = _log_warn if log_warnings else _log_debug

    external_storage = adb_call('shell', '"set | grep EXTERNAL_STORAGE"', device = device_id)
    if not external_storage:
        log_func('Call to get the EXTERNAL_STORAGE environment variable from device {} failed.'.format(device_id))
        return ''

    storage_path = external_storage.split('=')
    if len(storage_path) != 2:
        log_func('Received bad data while attempting extract the EXTERNAL_STORAGE environment variable from device {}.'.format(device_id))
        return ''

    var_path = storage_path[1].strip()
    status, _ = adb_ls(var_path, device_id)
    if status:
        return var_path
    else: 
        Logs.debug('adb_call: The path specified in EXTERNAL_STORAGE seems to have permission issues, attempting to resolve with realpath for device {}.'.format(device_id))

    real_path = adb_call('shell', 'realpath', var_path, device = device_id)
    if not real_path:
        log_func('Something happend while attempting to resolve the path from the EXTERNAL_STORAGE environment variable from device {}.'.format(device_id))
        return ''

    real_path = real_path.strip()
    status, _ = adb_ls(real_path, device_id)
    if status:
        return real_path
    else:
        log_func('Unable to validate the resolved EXTERNAL_STORAGE environment variable path from device {}.'.format(device_id))
        return ''


def construct_assets_path_for_game_project(ctx, game_project):
    '''
    Generates the relative path from the root of public storage to the application's specific data folder
    '''
    return 'Android/data/{}/files'.format(ctx.get_android_package_name(game_project))


def run_rc_job(ctx, job, source, target, game, assets_type, is_obb):
    rc_path = get_resource_compiler_path(ctx)

    command_args = [
        '"{}"'.format(rc_path),
        '/job={}'.format(os.path.join('Bin64', 'rc', job)),
        '/p={}'.format(assets_type),
        '/src="{}"'.format(source),
        '/trg="{}"'.format(target),
        '/threads={}'.format(ctx.options.max_cores),
        '/game={}'.format(game)
    ]

    if is_obb:
        pacakge_name = ctx.get_android_package_name(game)
        app_version_number = ctx.get_android_version_number(game)

        command_args.extend([
            '/obb_pak=main.{}.{}.obb'.format(app_version_number, pacakge_name),
            '/obb_patch_pak=patch.{}.{}.obb'.format(app_version_number, pacakge_name)
        ])

    command = ' '.join(command_args)
    Logs.debug('android_deploy: running RC command - {}'.format(command))
    call(command, shell = True)


def build_shader_paks(ctx, game, assets_type, layout_node, shaders_source_paths):
    pak_shaders_script = ctx.path.find_resource('Tools/PakShaders/pak_shaders.py')
    shaders_pak_dir = ctx.path.make_node("build/{}/{}".format(assets_type, game).lower())
    shaders_pak_dir.mkdir()

    command_args = [
        get_python_path(ctx), 
        '"{}"'.format(pak_shaders_script.abspath()),
        '"{}"'.format(shaders_pak_dir.abspath()),
        '-s'
    ]
    command_args.extend([ '{},"{}"'.format(key, path.abspath()) for key, path in shaders_source_paths.iteritems() ])

    command = ' '.join(command_args)
    Logs.debug('android_deploy: Running command - {}'.format(command))
    call(command, shell = True)

    shader_paks = shaders_pak_dir.ant_glob('*.pak')
    if not shader_paks:
        ctx.fatal('[ERROR] No shader pak files were found after running the pak_shaders command')

    # copy the shader paks into the layout directory
    shader_pak_dest = layout_node.make_node(game.lower())
    for pak in shader_paks:
        dest_node = shader_pak_dest.make_node(pak.name)
        dest_node.delete()

        Logs.debug('android_deploy: Copying {} => {}'.format(pak.relpath(), dest_node.relpath()))
        shutil.copy2(pak.abspath(), dest_node.abspath())


def pack_assets_in_apk(ctx, executable_name, layout_node):
    class command_buffer:
        def __init__(self, base_command_args):
            self._args_master = base_command_args
            self._base_len = len(' '.join(base_command_args))

            self.args = self._args_master[:]
            self.len = self._base_len

        def flush(self):
            if len(self.args) > len(self._args_master):
                command = ' '.join(self.args)
                Logs.debug('android_deploy: Running command - {}'.format(command))
                call(command, shell = True)

                self.args = self._args_master[:]
                self.len = self._base_len

    android_cache = get_android_cache_node(ctx)

    # create a copy of the existing barebones APK for the assets
    variant = getattr(ctx.__class__, 'variant', None)
    if not variant:
        (platform, configuration) = ctx.get_platform_and_configuration()
        variant = '{}_{}'.format(platform, configuration)

    raw_apk_path = os.path.join(ctx.get_bintemp_folder_node().name, variant, ctx.get_android_project_relative_path(), executable_name, 'apk')
    barebones_apk_path = '{}/{}_unaligned_unsigned.apk'.format(raw_apk_path, executable_name)

    if not os.path.exists(barebones_apk_path):
        ctx.fatal('[ERROR] Unable to find the barebones APK in path {}.  Run the build command for {} again to generate it.'.format(barebones_apk_path, variant))

    apk_cache_node = android_cache.make_node('apk')
    apk_cache_node.mkdir()

    raw_apk_with_asset_node = apk_cache_node.make_node('{}_unaligned_unsigned{}.apk'.format(executable_name, APK_WITH_ASSETS_SUFFIX))

    shutil.copy2(barebones_apk_path, raw_apk_with_asset_node.abspath())

    # We need to make the 'assets' junction in order to generate the correct pathing structure when adding 
    # files to an existing APK
    asset_dir = 'assets'
    asset_junction = android_cache.make_node(asset_dir)

    if os.path.exists(asset_junction.abspath()):
        remove_junction(asset_junction.abspath())

    try:
        Logs.debug('android_deploy: Creating assets junction "{}" ==> "{}"'.format(layout_node.abspath(), asset_junction.abspath()))
        junction_directory(layout_node.abspath(), asset_junction.abspath())
    except:
        ctx.fatal("[ERROR] Could not create junction for asset folder {}".format(layout_node.abspath()))

    # add the assets to the APK
    command = command_buffer([ 
        '"{}"'.format(ctx.env['AAPT']), 
        'add', 
        '"{}"'.format(raw_apk_with_asset_node.abspath())
    ])
    command_len_max = get_command_line_limit()

    with push_dir(android_cache.abspath()):
        ctx.user_message('Packing assets into the APK...')
        Logs.debug('android_deploy: -> from {}'.format(os.getcwd()))

        assets = asset_junction.ant_glob('**/*')
        for asset in assets:
            file_path = asset.path_from(android_cache)

            file_path = '"{}"'.format(file_path.replace('\\', '/'))

            path_len = len(file_path) + 1 # 1 for the space
            if (command.len + path_len) >= command_len_max:
                command.flush()

            command.len += path_len
            command.args.append(file_path)

        # flush the command buffer one more time
        command.flush()

    return apk_cache_node, raw_apk_with_asset_node


class adb_copy_output(Task):
    '''
    Class to handle copying of a single file in the layout to the android
    device.
    '''

    def __init__(self, *k, **kw):
        Task.__init__(self, *k, **kw)
        self.device = ''
        self.target = ''

    def set_device(self, device):
        '''Sets the android device (serial number from adb devices command) to copy the file to'''
        self.device = device

    def set_target(self, target):
        '''Sets the target file directory (absolute path) and file name on the device'''
        self.target = target

    def run(self):
        # Embed quotes in src/target so that we can correctly handle spaces
        src = '"{}"'.format(self.inputs[0].abspath())
        tgt = '"{}"'.format(self.target)

        Logs.debug('adb_copy_output: performing copy - {} to {} on device {}'.format(src, tgt, self.device))
        adb_call('push', src, tgt, device = self.device)

        return 0

    def runnable_status(self):
        if Task.runnable_status(self) == ASK_LATER:
            return ASK_LATER

        return RUN_ME


@taskgen_method
def adb_copy_task(self, android_device, src_node, output_target):
    '''
    Create a adb_copy_output task to copy the src_node to the ouput_target
    on the specified device. output_target is the absolute path and file name
    of the target file.
    '''
    copy_task = self.create_task('adb_copy_output', src_node)
    copy_task.set_device(android_device)
    copy_task.set_target(output_target)


###############################################################################
# create a deployment context for each build variant
for configuration in ['debug', 'profile', 'release']:
    for compiler in ['clang', 'gcc']:
        class DeployAndroidContext(Build.BuildContext):
            fun = 'deploy'
            variant = 'android_armv7_' + compiler + '_' + configuration
            after = ['build_android_armv7_' + compiler + '_' + configuration]
            cmd = 'deploy_android_armv7_' + compiler + '_' + configuration


            def use_vfs(self):
                if not hasattr(self, 'cached_use_vfs'):
                    self.cached_use_vfs = (self.get_bootstrap_vfs() == '1')
                return self.cached_use_vfs


            def use_obb(self):
                if not hasattr(self, 'cached_use_obb'):
                    game = self.get_bootstrap_game()

                    use_main_obb = (self.get_android_use_main_obb(game).lower() == 'true')
                    use_patch_obb = (self.get_android_use_patch_obb(game).lower() == 'true')

                    self.cached_use_obb = (use_main_obb or use_patch_obb)

                return self.cached_use_obb


            def get_asset_cache(self):
                if not hasattr(self, 'asset_cache'):
                    game = self.get_bootstrap_game()
                    assets = self.get_bootstrap_assets("android")
                    asset_dir = "Cache/{}/{}".format(game, assets).lower()

                    self.asset_cache = self.path.find_dir(asset_dir)

                return self.asset_cache


            def get_layout_node(self):
                if not hasattr(self, 'android_armv7_layout_node'):
                    game = self.get_bootstrap_game()
                    asset_deploy_mode = self.options.deploy_android_asset_mode.lower()

                    if asset_deploy_mode == ASSET_DEPLOY_LOOSE:
                        self.android_armv7_layout_node = self.get_asset_cache()

                    elif asset_deploy_mode == ASSET_DEPLOY_PAKS:
                        self.android_armv7_layout_node = self.path.make_node('AndroidArmv7LayoutPak/{}'.format(game))

                    elif asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS:
                        layout_folder_name = 'AndroidArmv7LayoutObb' if self.use_obb() else 'AndroidArmv7LayoutPak'
                        self.android_armv7_layout_node = self.path.make_node('{}/{}'.format(layout_folder_name, game))

                # just incase get_layout_node is called before deploy_android_asset_mode has been validated, which
                # could mean android_armv7_layout_node never getting set
                try:
                    return self.android_armv7_layout_node
                except:
                    self.fatal('[ERROR] Unable to determine the asset layout node for Android ARMv7.')


            def get_abi(self):
                return 'armeabi-v7a'


            def user_message(self, message):
                Logs.pprint('CYAN', message)


        class DeployAndroid(DeployAndroidContext):
            after = ['build_android_armv7_' + compiler + '_' + configuration]
            cmd = 'deploy_android_armv7_' + compiler + '_' + configuration
            features = ['deploy_android_prepare']


        class DeployAndroidDevices(DeployAndroidContext):
            after = ['deploy_android_armv7_' + compiler + '_' + configuration]
            cmd = 'deploy_android_devices_' + compiler + '_' + configuration
            features = ['deploy_android_devices']


@taskgen_method
@feature('deploy_android_prepare')
def prepare_to_deploy_android(tsk_gen):
    '''
    Prepare the deploy process by generating the necessary asset layout 
    directories, pak / obb files and packing assets in the APK if necessary.
    '''

    bld = tsk_gen.bld
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    # handle a few non-fatal early out cases
    if platform not in ('android_armv7_gcc', 'android_armv7_clang') or not bld.is_option_true('deploy_android') or bld.options.from_android_studio:
        bld.user_message('Skipping Android Deployment...')
        return

    # make sure the adb server is running first before we execute any other commands
    # special case how these error messages are handled to only be truely fatal when 
    # executed from the editor
    log_error = Logs.error
    if bld.options.from_editor_deploy:
        log_error = bld.fatal

    if adb_call('start-server') is None:
        log_error('[ERROR] Failed to start adb server, unable to perform the deploy')
        return

    devices = get_list_of_android_devices()
    if len(devices) == 0:
        adb_call('kill-server')
        log_error('[ERROR] No Android devices detected, unable to deploy')
        return

    # determine the selected asset deploy mode
    asset_deploy_mode = bld.options.deploy_android_asset_mode.lower()
    if configuration == 'release':
        # force release mode to use the project's settings
        asset_deploy_mode = ASSET_DEPLOY_PROJECT_SETTINGS

    if asset_deploy_mode not in ASSET_DEPLOY_MODES:
        bld.fatal('[ERROR] Unable to determine the asset deployment mode.  Valid options for --deploy-android-asset-mode are limited to: {}'.format(ASSET_DEPLOY_MODES))

    setattr(options, 'deploy_android_asset_mode', asset_deploy_mode)
    Logs.debug('android_deploy: Using asset mode - {}'.format(asset_deploy_mode))

    if bld.get_asset_cache() is None:
        bld.fatal('[ERROR] There is no asset cache to read from at {}.  Please run AssetProcessor / AssetProcessorBatch from '
                   'the Bin64vc120 / Bin64vc140 / BinMac64 directory with "es3" assets enabled in the AssetProcessorPlatformConfig.ini'.format(asset_dir))

    game = bld.get_bootstrap_game()
    executable_name = bld.get_executable_name(game)
    assets = bld.get_bootstrap_assets("android")

    # handle the asset pre-processing
    if (asset_deploy_mode == ASSET_DEPLOY_PAKS) or (asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS):
        if bld.use_vfs():
            bld.fatal('[ERROR] Cannot use VFS when the --deploy-android-asset-mode is set to "{}", please set remote_filesystem=0 in bootstrap.cfg'.format(asset_deploy_mode))

        asset_cache = bld.get_asset_cache()
        layout_node = bld.get_layout_node()

        # generate the pak/obb files
        is_obb = (asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS) and bld.use_obb()
        rc_job = bld.get_android_rc_obb_job(game) if is_obb else bld.get_android_rc_pak_job(game)

        bld.user_message('Generating the necessary pak files...')
        run_rc_job(bld, rc_job, asset_cache.relpath(), layout_node.relpath(), game, assets, is_obb)

        # handles the shaders
        if asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS:

            shaders_pak = '{}/shaders.pak'.format(game).lower()
            shaderscache_pak = '{}/shadercache.pak'.format(game).lower()

            if layout_node.find_resource(shaders_pak) and layout_node.find_resource(shaderscache_pak):
                bld.user_message('Using found shaders paks in the layout folder - {}'.format(layout_node.relpath()))

            else: 
                bld.user_message('Searching for cached shaders locally and on connected devices...')

                shader_types = [ 'gles3_0', 'gles3_1' ]
                shaders_source_paths = dict()

                relative_assets_path = construct_assets_path_for_game_project(bld, game)

                for shader_flavor in shader_types:
                    shader_cache_path = 'user/cache/shaders/cache/{}'.format(shader_flavor).lower()

                    # Check if we already have the shader's source files or we need to pull them from the device.
                    shader_user_node = asset_cache.find_dir(shader_cache_path)
                    if shader_user_node:
                        shaders_source_paths[shader_flavor] = shader_user_node
                        Logs.debug('android_deploy: Skipping pulling shaders of type {} from device. Using "user" folder instead.'.format(shader_flavor))

                    # pull the shaders
                    else:
                        pull_shaders_folder = bld.path.make_node("build/temp/{}/{}/{}".format(assets, game, shader_flavor).lower())
                        pull_shaders_folder.mkdir()

                        if os.path.exists(pull_shaders_folder.abspath()):
                            try:
                                shutil.rmtree(pull_shaders_folder.abspath(), ignore_errors = True)
                            except:
                                Logs.warn('[WARN] Failed to delete {} folder to copy shaders'.format(pull_shaders_folder.relpath()))

                        for android_device in devices:
                            storage_path = auto_detect_device_storage_path(android_device)
                            if not storage_path:
                                continue

                            device_folder = '{}/{}/{}'.format(storage_path, relative_assets_path, shader_cache_path)
                            ls_status, _ = adb_ls(device_folder, android_device)
                            if not ls_status:
                                continue

                            command = [
                                'pull',
                                device_folder, 
                                '"{}"'.format(pull_shaders_folder.abspath()), 
                            ]

                            adb_call(*command, device = android_device)
                            shaders_source_paths[shader_flavor] = pull_shaders_folder
                            break

                found_shader_types = shaders_source_paths.keys()
                if found_shader_types != shader_types:
                    message = 'Unable to find all shader types needed for shader pak generation.  Found {}, Expected {}'.format(found_shader_types, shader_types)

                    if configuration == 'release':
                        bld.fatal('[ERROR] android_deploy: {}'.format(message))
                    else:
                        Logs.warn('[WARN] android_deploy: {}'.format(message))

                if shaders_source_paths:
                    bld.user_message('Generating the shader pak files...')
                    build_shader_paks(bld, game, assets, layout_node, shaders_source_paths)

            # get the keystore passwords
            set_key_and_store_pass(bld)

            # generate the new apk with assets in it
            apk_cache_node, raw_apk_with_asset = pack_assets_in_apk(bld, executable_name, layout_node)

            # sign and align the apk
            final_apk_out = bld.get_output_folders(platform, configuration)[0]
            tsk_gen.sign_and_align_apk(
                executable_name, # base_apk_name
                raw_apk_with_asset, # raw_apk
                apk_cache_node, # intermediate_folder
                final_apk_out, # final_output
                APK_WITH_ASSETS_SUFFIX # suffix
            )

    compiler = platform.split('_')[2]
    Options.commands.append('deploy_android_devices_' + compiler + '_' + configuration)


@taskgen_method
@feature('deploy_android_devices')
def deploy_to_devices(ctx):
    '''
    Installs the project APK and copies the layout directory to all the
    android devices that are connected to the host.
    '''
    def should_copy_file(src_file_node, target_time):
        should_copy = False
        try:
            stat_src = os.stat(src_file_node.abspath())
            should_copy = stat_src.st_mtime >= target_time
        except OSError:
            pass

        return should_copy

    bld = ctx.bld
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    game = bld.get_bootstrap_game()
    executable_name = bld.get_executable_name(game)

    asset_deploy_mode = bld.options.deploy_android_asset_mode.lower()

    # get location of APK either from command line option or default build location
    if Options.options.apk_path == '':
        suffix = ''
        if asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS:
            suffix = APK_WITH_ASSETS_SUFFIX

        output_folder = bld.get_output_folders(platform, configuration)[0]
        apk_name = '{}/{}{}.apk'.format(output_folder.abspath(), executable_name, suffix)
    else:
        apk_name = Options.options.apk_path

    do_clean = bld.is_option_true('deploy_android_clean_device')
    deploy_executable = bld.is_option_true('deploy_android_executable')
    deploy_assets = (asset_deploy_mode == ASSET_DEPLOY_LOOSE) or (asset_deploy_mode == ASSET_DEPLOY_PAKS)
    layout_node = bld.get_layout_node()

    Logs.debug('android_deploy: deploy options: do_clean {}, deploy_exec {}, deploy_assets {}'.format(do_clean, deploy_executable, deploy_assets))

    # Some checkings before we start the deploy process.
    if deploy_executable and not os.path.exists(apk_name):
        bld.fatal('[ERROR] Could not find the Android executable (APK) in path - {} - necessary for deployment.  Run the build command for {}_{} to generate it'.format(apk_name, platform, configuration))
        return


    deploy_libs = bld.options.deploy_android_attempt_libs_only and not (do_clean or bld.options.from_editor_deploy) and not (asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS)
    Logs.debug('android_deploy: The option to attempt library only deploy is %s', 'ENABLED' if deploy_libs else 'DISABLED')

    variant = '{}_{}'.format(platform, configuration)
    apk_builder_path = os.path.join( variant, bld.get_android_project_relative_path(), executable_name, 'builder' )
    apk_builder_node = bld.get_bintemp_folder_node().make_node(apk_builder_path)
    stripped_libs_node = apk_builder_node.make_node([ 'lib', bld.get_abi() ])

    game_package = bld.get_android_package_name(game)
    device_install_path = '/data/data/{}'.format(game_package)

    apk_stat = os.stat(apk_name)
    apk_size = apk_stat.st_size


    relative_assets_path = construct_assets_path_for_game_project(bld, game)

    # This is the name of a file that we will use as a marker/timestamp. We
    # will get the timestamp of the file off the device and compare that with
    # asset files on the host machine to determine if the host machine asset
    # file is newer than what the device has, and if so copy it to the device.
    timestamp_file_name = 'engineroot.txt'
    host_timestamp_file = bld.path.find_node(timestamp_file_name)

    deploy_count = 0

    devices = get_list_of_android_devices()
    for android_device in devices:
        bld.user_message('Starting to deploy to android device ' + android_device)

        storage_path = auto_detect_device_storage_path(android_device, log_warnings = True)
        if not storage_path:
            continue

        output_target = '{}/{}'.format(storage_path, relative_assets_path)
        device_timestamp_file = '{}/{}'.format(output_target, timestamp_file_name)

        if do_clean:
            bld.user_message('Cleaning target before deployment...')

            adb_call('shell', 'rm', '-rf', output_target, device = android_device)
            bld.user_message('Target Cleaned...')

            package_name = bld.get_android_package_name(game)
            if len(package_name) != 0 and deploy_executable:
                bld.user_message('Uninstalling package ' + package_name)
                adb_call('uninstall', package_name, device = android_device)

        ################################
        if deploy_libs:
            access_type = get_device_access_type(android_device)
            if access_type == ACCESS_NORMAL:
                Logs.warn('[WARN] android_deploy: Unable to perform the library only copy on device {}'.format(android_device))

            elif access_type in (ACCESS_ROOT_ADBD, ACCESS_SHELL_SU):
                device_file_staging_path = '{}/LY_Staging'.format(storage_path)
                device_lib_timestamp_file = '{}/files/{}'.format(device_install_path, timestamp_file_name)

                def _adb_push(source_node, dest, device_id):
                    adb_call('push', '"{}"'.format(source_node.abspath()), dest, device = device_id)

                def _adb_shell(source_node, dest, device_id):
                    temp_dest = '{}/{}'.format(device_file_staging_path, source_node.name)
                    adb_call('push', '"{}"'.format(source_node.abspath()), temp_dest, device = device_id)
                    adb_call('shell', 'su', '-c', 'cp', temp_dest, dest, device = device_id)

                if access_type == ACCESS_ROOT_ADBD:
                    adb_root_push_func = _adb_push

                elif access_type == ACCESS_SHELL_SU:
                    adb_root_push_func = _adb_shell
                    adb_call('shell', 'mkdir', device_file_staging_path)

                install_check = adb_call('shell', '"pm list packages | grep {}"'.format(game_package), device = android_device)
                if install_check:
                    target_time = get_device_file_timestamp(device_lib_timestamp_file, android_device, True)

                    # cases for early out infavor of re-installing the APK:
                    #       If target_time is zero, the file wasn't found which would indicate we haven't attempt to push just the libs before
                    #       The dalvik executable is newer than the last time we deployed to this device
                    if target_time == 0 or should_copy_file(apk_builder_node.make_node('classes.dex'), target_time):
                        bld.user_message('A new APK needs to be installed instead for device {}'.format(android_device))

                    # otherwise attempt to copy the libs directly
                    else:
                        bld.user_message('Scanning which libraries need to be copied...')

                        libs_to_add = []
                        total_libs_size = 0
                        fallback_to_apk = False

                        libs = stripped_libs_node.ant_glob('**/*.so')
                        for lib in libs:

                            if should_copy_file(lib, target_time):
                                lib_stat = os.stat(lib.abspath())
                                total_libs_size += lib_stat.st_size
                                libs_to_add.append(lib)

                                if total_libs_size >= apk_size:
                                    bld.user_message('Too many libriares changed, falling back to installing a new APK on {}'.format(android_device))
                                    fallback_to_apk = True
                                    break

                        if not fallback_to_apk:
                            for lib in libs_to_add:
                                final_target_dir = '{}/lib/{}'.format(device_install_path, lib.name)

                                adb_root_push_func(lib, final_target_dir, android_device)

                                adb_call('shell', 'su', '-c', 'chown', LIB_OWNER_GROUP, final_target_dir, device = android_device)
                                adb_call('shell', 'su', '-c', 'chmod', LIB_FILE_PERMISSIONS, final_target_dir, device = android_device)

                            deploy_executable = False

                # update the timestamp file
                adb_root_push_func(host_timestamp_file, device_lib_timestamp_file, android_device)
                adb_call('shell', 'su', '-c', 'touch', device_lib_timestamp_file, device = android_device)

                # clean up the staging directory
                if access_type == ACCESS_SHELL_SU:
                    adb_call('shell', 'rm', '-rf', device_file_staging_path)

        ################################
        if deploy_executable:
            install_options = getattr(Options.options, 'deploy_android_install_options')
            replace_package = ''
            if bld.is_option_true('deploy_android_replace_apk'):
                replace_package = '-r'

            bld.user_message('Installing ' + apk_name)
            install_result = adb_call('install', install_options, replace_package, '"{}"'.format(apk_name), device = android_device)
            if not install_result or 'success' not in install_result.lower():
                Logs.warn('[WARN] android deploy: failed to install APK on device %s.' % android_device)
                if install_result:
                    # The error msg is the last non empty line of the output.
                    error_msg = next(error for error in reversed(install_result.splitlines()) if error)
                    Logs.warn('[WARN] android deploy: %s' % error_msg)

                continue

        if deploy_assets:
            target_time = get_device_file_timestamp(device_timestamp_file, android_device)

            if do_clean or target_time == 0:
                bld.user_message('Copying all assets to the device {}. This may take some time...'.format(android_device))

                # There is a chance that if someone ran VFS before this deploy on an empty directory the output_target directory will have
                # already existed when we do the push command. In this case if we execute adb push command it will create an ES3 directory
                # and put everything there, causing the deploy to be 'successful' but the game will crash as it won't be able to find the
                # assets. Since we detected a "clean" build, wipe out the output_target folder if it exists first then do the push and
                # everything will be just fine.
                ls_status, _ = adb_ls(output_target, android_device)
                if ls_status:
                    adb_call('shell', 'rm', '-rf', output_target)

                push_status = adb_call('push', '"{}"'.format(layout_node.abspath()), output_target, device = android_device)
                if not push_status:
                    # Clean up any files we may have pushed to make the next run success rate better
                    adb_call('shell', 'rm', '-rf', output_target, device = android_device)
                    bld.fatal('[ERROR] The ABD command to push all the files to the device failed.')
                    continue

            else:
                layout_files = layout_node.ant_glob('**/*')
                bld.user_message('Scanning {} files to determine which ones need to be copied...'.format(len(layout_files)))
                for src_file in layout_files:
                    # Faster to check if we should copy now rather than in the copy_task
                    if should_copy_file(src_file, target_time):
                        final_target_dir = '{}/{}'.format(output_target, string.replace(src_file.path_from(layout_node), '\\', '/'))
                        ctx.adb_copy_task(android_device, src_file, final_target_dir)

                # Push the timestamp_file_name last so that it has a timestamp that we can use on the next deploy to know which files to
                # upload to the device
                adb_call('push', '"{}"'.format(host_timestamp_file.abspath()), device_timestamp_file, device = android_device)

        deploy_count = deploy_count + 1

    if deploy_count == 0:
        bld.fatal('[ERROR] Failed to deploy the build to any connected devices.')

