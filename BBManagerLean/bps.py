#!/usr/bin/env python
"""
bps: Build, Package, Ship

The tiny tool to complete all the steps related to build, packaging and shipping
the BeatBuddy Manager Lean.

Integrated building with version bumping and publishing facilities tailored to
the distribution mechanisms in BBML. Version bumping should update source and
docs wherever needed.

Help messages should document all usage patterns. Code should work in Python 2
and Python 3 at least until 2020, when Python 2 is slated for EOL.
"""
from __future__ import print_function

import argparse, subprocess, os

APP_NAME = 'BBManagerLean'

"""Version Information representation for the project, includes the mechanisms
for manipulating it."""


class VersionInfo:
    MAJOR = 'MAJOR'
    MINOR = 'MINOR'
    PATCH = 'PATCH'
    BUILD = 'BUILD'

    def __init__(self, project_root='.', version_file_name='version.csv'):
        self.version_number_dict = {VersionInfo.MAJOR: 0, VersionInfo.MINOR: 0, VersionInfo.PATCH: 0,
                                    VersionInfo.BUILD: 0}
        self.version_file_path = project_root + '/' + version_file_name

        if os.path.isfile(self.version_file_path):
            for line in open(self.version_file_path, 'r').readlines():
                k = line.split(',')[0]
                v = int(line.split(',')[1])
                self.version_number_dict[k] = v
        else:
            raise Exception('Version file not found in path: ' + self.version_file_path)

        if len(self.version_number_dict.keys()) != 4:
            raise Exception('Version file appears malformed.')

    def __str__(self):
        return "%(MAJOR)d.%(MINOR)d.%(PATCH)d.%(BUILD).d" % self.version_number_dict

    def version_fragment(self, fragment):
        return self.version_number_dict[fragment]

    def bump(self, kind):
        if kind not in [VersionInfo.BUILD, VersionInfo.PATCH, VersionInfo.MINOR, VersionInfo.MAJOR]:
            raise Exception('Trying to bump a non-existing version fragment.')
        if kind == VersionInfo.BUILD:
            self.version_number_dict[VersionInfo.BUILD] = int(self.version_number_dict[VersionInfo.BUILD]) + 1
        elif kind == VersionInfo.PATCH:
            self.version_number_dict[VersionInfo.PATCH] = int(self.version_number_dict[VersionInfo.PATCH]) + 1
            self.version_number_dict[VersionInfo.BUILD] = 0
        elif kind == VersionInfo.MINOR:
            self.version_number_dict[VersionInfo.MINOR] = int(self.version_number_dict[VersionInfo.MINOR]) + 1
            self.version_number_dict[VersionInfo.BUILD] = 0
            self.version_number_dict[VersionInfo.PATCH] = 0
        elif kind == MAJOR:
            self.version_number_dict[MAJOR] = int(self.version_number_dict[MAJOR]) + 1
            self.version_number_dict[MINOR] = 0
            self.version_number_dict[BUILD] = 0
            self.version_number_dict[PATCH] = 0
        self._write_version_file()
        self._write_version_constants_h()

    def _write_version_file(self):
        version_info_file_template = """\
MAJOR,%(MAJOR)d
MINOR,%(MINOR)d
PATCH,%(PATCH)d
BUILD,%(BUILD)d\
"""

        vf = open(self.version_file_path, 'w')
        vf.write(version_info_file_template % self.version_number_dict)
        vf.close()

    def _write_version_constants_h(self):
        version_constants_template = """\
#ifndef VERSION_CONSTANTS_H
#define VERSION_CONSTANTS_H

#define VERSION_MAJOR %(MAJOR)d
#define VERSION_MINOR %(MINOR)d
#define VERSION_PATCH %(PATCH)d
#define VERSION_BUILD %(BUILD)d

#endif // VERSION_CONSTANTS_H\
"""
        version_constants_file = './src/version_constants.h'

        vcf = open(version_constants_file, 'w')
        vcf.write(version_constants_template % self.version_number_dict)
        vcf.close()


class Builder:
    def __init__(self):
        pass

    @staticmethod
    def _mac_deployment_specifics():
        subprocess.call('PRJ_ROOT=. ./mac_deployment/deploy.sh ./BBManagerLean.app/', shell=True)

    @staticmethod
    def partial_build():
        subprocess.call('qmake && make -j4', shell=True)
        Builder._mac_deployment_specifics()

    @staticmethod
    def full_build():
        subprocess.call('make clean', shell=True)
        Builder.partial_build()


class Shipper:
    def __init__(self, version_info):
        self.version_info = version_info
        self.local_artifact_name = APP_NAME + '-' + str(self.version_info) + '.dmg'
        pass

    def package(self):
        subprocess.call('mv ' + APP_NAME + '.app mac_deployment/', shell=True)
        subprocess.call('VERSION="' + str(self.version_info) +
                        '" APP_NAME=' + APP_NAME +
                        ' DMG_FINAL="' + self.local_artifact_name + '"' +
                        ' ./mac_deployment/dmg_gen.sh',
                        shell=True)

    def release(self, kind='latest'):
        print('Releasing...')
        release_xml_template = """\
<?xml version="1.0" encoding="utf-8"?>
<BBManagerLean>
    <version major="%(MAJOR)d" minor="%(MINOR)d" patch="%(PATCH)d" build="%(BUILD)d" announcement="%(ANNOUNCEMENT)s" download="%(DOWNLOAD)s" changelog="%(CHANGELOG)s"/>
</BBManagerLean>\
"""
        remote_artifact_name = 'BBManagerLean-' + kind + '.dmg'

        release_info = {'ANNOUNCEMENT': 'https://www.singularsound.com',
                        'DOWNLOAD': 'https://www.singularsound.com/downloadable/' + remote_artifact_name,
                        'CHANGELOG': 'https://www.singularsound.com'}
        release_info.update(self.version_info.version_number_dict)
        current_release_xml = release_xml_template % release_info
        release_file_name = kind + '-macx.xml'
        latest = open(release_file_name, 'w')
        latest.write(current_release_xml)
        latest.close()
        subprocess.call('echo "put ' + release_file_name + ' /var/www/html/downloadable/' + release_file_name
                        + '" | sftp root@159.203.168.69 -b', shell=True)
        cmd = 'echo "put ./mac_deployment/' + self.local_artifact_name + ' /var/www/html/downloadable/' + remote_artifact_name + '" | sftp root@159.203.168.69 -b'
        print(cmd)
        subprocess.call(cmd, shell=True)


parser = argparse.ArgumentParser()

parser.add_argument('--bump', help='Bumps the fragment of the version of the project, zeroing less significant digits',
                    choices=['major', 'minor', 'patch', 'build'])
parser.add_argument('--current', help='Prints the current version of the project', action='store_true')
parser.add_argument('--build', help='Produces a project buikd, either incremental or full', choices=['inc', 'full'])
parser.add_argument('--pack', help='Creates a distributable dmg from the existing build', action='store_true')
parser.add_argument('--release',
                    help='Uploads the DMG, and generates and updates the appropriate release version xml file',
                    choices=['latest', 'stable'])

args = parser.parse_args()

vi = VersionInfo()

if args.bump or args.build:
    vi.bump(args.bump.upper())

if args.build == 'inc':
    Builder.partial_build()
elif args.build == 'full':
    Builder.full_build()

if args.pack:
    Shipper(vi).package()

if args.release:
    Shipper(vi).release(args.release)

if args.current:
    print(str(vi))
