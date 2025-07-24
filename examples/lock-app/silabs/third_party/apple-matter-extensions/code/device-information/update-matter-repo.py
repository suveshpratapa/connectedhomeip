#!/usr/bin/env python3

# Copyright (c) 2023 Apple Inc. All rights reserved.
#
# Apple Matter Extensions is licensed under Apple Inc.'s MFi Sample Code
# License Agreement, which is contained in the LICENSE file distributed with
# the Apple Matter Extensions, and only to those who accept that license.

import argparse
import json
import os
import shutil
import subprocess
import xml.etree.ElementTree

def import_modules(matter_repo_path):
    try:
        from matter_idl.zapxml import ParseSource, ParseXmls
    except ImportError:
        import sys
        sys.path.append(os.path.join(matter_repo_path, 'scripts/py_matter_idl/matter_idl'))


DEVICE_INFORMATION_ROOT_DIR = os.path.realpath(os.path.dirname(__file__))
APPLE_EXTENSION_XML = 'apple-device-information-cluster.xml'


class MissingZAPFileException(FileExistsError):
    """Exception for Not finding the ZAP file provided."""


def copy_xml_to_repo(matter_repo_path):
    xml_dir = os.path.join(matter_repo_path, 'src/app/zap-templates/zcl/data-model/apple')
    try:
        os.mkdir(xml_dir)
    except FileExistsError:
        pass
    shutil.copy(os.path.join(DEVICE_INFORMATION_ROOT_DIR, APPLE_EXTENSION_XML),
                xml_dir)


def update_zcl_json(matter_repo_path):
    zcl_config_file = os.path.join(matter_repo_path, 'src/app/zap-templates/zcl/zcl.json')
    with open(zcl_config_file, 'r') as file:
        data = json.load(file)

    data['xmlRoot'].append('./data-model/apple/')
    data['xmlFile'].append(APPLE_EXTENSION_XML)

    with open(zcl_config_file, 'w') as file:
        json.dump(data, file, indent=4, separators=(',', ': '))


def generate_app_common(matter_repo_path):
    subprocess.run([os.path.join(matter_repo_path, 'scripts/tools/zap/generate.py'), 'src/controller/data_model/controller-clusters.zap', '-t', 'src/app/common/templates/templates.json', '-o', 'zzz_generated/app-common/app-common/zap-generated'])


IMPL_DIR = 'apple-device-information-server'


def update_cluster_list(matter_repo_path):
    cluster_config_file = os.path.join(matter_repo_path, 'src/app/zap_cluster_list.json')
    with open(cluster_config_file, 'r') as file:
        data = json.load(file)

    data['ServerDirectories']['APPLE_DEVICE_INFORMATION_CLUSTER'] = [ IMPL_DIR ]

    with open(cluster_config_file, 'w') as file:
        json.dump(data, file, indent=4, separators=(',', ': '))


def copy_device_info_cpp(matter_repo_path):
    impl_dir = os.path.join(matter_repo_path, 'src/app/clusters', IMPL_DIR)
    try:
        os.mkdir(impl_dir)
    except FileExistsError:
        pass
    shutil.copy(os.path.join(DEVICE_INFORMATION_ROOT_DIR, 'apple-device-information-server.cpp'),
                impl_dir)


def xml_to_zap_attr_info(xml_info, zap_info):
    zap_info['name'] = xml_info.text
    zap_info['side'] = xml_info.attrib['side']
    if xml_info.attrib['code'].startswith("0x"):
        zap_info['code'] = int(xml_info.attrib['code'], 16)
    else:
        zap_info['code'] = int(xml_info.attrib['code'], 10)
    zap_info['type'] = xml_info.attrib['type']


def update_zap_file_with_xml_cluster(matter_repo_path, zap_file_path):
    global_attributes_xml_path = os.path.join(
        matter_repo_path, 'src/app/zap-templates/zcl/data-model/chip/global-attributes.xml')
    global_attributes_tree = xml.etree.ElementTree.parse(global_attributes_xml_path)
    global_attributes = global_attributes_tree.getroot().find("global")
    # Filter out the array-typed attributes, and the non-server attributes, since we don't need to add them.
    global_attributes = [
        attr for attr in global_attributes
        if attr.tag == 'attribute' and attr.attrib['side'] == 'server' and attr.attrib['type'] != 'array'
    ]

    apple_extension_cluster_template = {
        'mfgCode': None,
        'side': 'server',
        'enabled': 1,
        'commands': [],
        'attributes': [],
        'events': []
        }

    attr_template = {
        'mfgCode': None,
        'included': 1,
        'storageOption': 'RAM',
        'singleton': 0,
        'bounded': 0,
        'reportable': 1,
        'minInterval': 0,
        'maxInterval': 1,
        'reportableChange': 0
        }

    xml_path = os.path.join(DEVICE_INFORMATION_ROOT_DIR, APPLE_EXTENSION_XML)
    tree = xml.etree.ElementTree.parse(xml_path)
    configurator = tree.getroot()

    # Parse Apple Device Information XML and store values
    for element in configurator:
        if element.tag == 'cluster':
            apple_extension_cluster_template['name'] = element.find('name').text.strip()
            apple_extension_cluster_template['code'] = int(element.find('code').text, 16)
            apple_extension_cluster_template['define'] = element.find('define').text

            # Grab any default overrides for global attributes.
            for our_global_attr in element.findall('globalAttribute'):
                for global_attr in global_attributes:
                    if our_global_attr.attrib['code'] == global_attr.attrib['code']:
                        global_attr.attrib['default'] = our_global_attr.attrib['value']

            for attr in element.findall('attribute'):
                temp_attr = attr_template.copy()
                xml_to_zap_attr_info(attr, temp_attr)
                temp_attr['defaultValue'] = '1'
                apple_extension_cluster_template['attributes'].append(temp_attr)

            for attr in global_attributes:
                temp_attr = attr_template.copy()
                xml_to_zap_attr_info(attr, temp_attr)
                temp_attr['defaultValue'] = attr.attrib['default']
                apple_extension_cluster_template['attributes'].append(temp_attr)

    # Parse ZAP files and update Apple Extesnsion cluster.
    with open(zap_file_path, 'r') as file:
        zap_file_data = json.load(file)
        endpoints = zap_file_data['endpoints']
        endpoint_types = zap_file_data['endpointTypes']
        root_endpoint_type_index = None
        root_endpoint_type = {}

        # Iterate through the defined endpoints in the ZAP file and file the root endpoint.
        for endpoint in endpoints:
            if endpoint['endpointId'] == 0:
                root_endpoint_type_index = endpoint['endpointTypeIndex']

        # Get the Root EndPoint type by index
        root_endpoint_type = endpoint_types[root_endpoint_type_index]

        for cluster in root_endpoint_type['clusters']:
            if apple_extension_cluster_template['name'] == cluster['name']:
                break
        else:
            root_endpoint_type['clusters'].append(apple_extension_cluster_template)

    # Write updated Apple Extension Cluster to the ZAP file.
    with open(zap_file_path, 'w') as file:
        json.dump(zap_file_data, file, indent=2, separators=(',', ': '))


def generate_lock_app_zap_file(matter_repo_path, zap_file_path):
    subprocess.run([os.path.join(matter_repo_path, 'scripts/tools/zap/generate.py'),
                    # generate.py expects a ZAP file path relative to the Matter root.
                    os.path.relpath(zap_file_path, matter_repo_path)])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Update the Matter Repo with the Apple Extension files')

    parser.add_argument(
        '--zap-file-path',
        help='The lock-app zap file path to update with the apple extension cluster',
        required=True,
    )

    parser.add_argument(
        '--matter-repo',
        help='The location of the Matter repository.  Defaults to current working directory.',
        required=False,
        default=os.getcwd())

    args = parser.parse_args()
    zap_file_path = args.zap_file_path
    if not os.path.isfile(zap_file_path):
        raise MissingZAPFileException('ZAP file does not exist: {}'.format(zap_file_path))

    matter_repo_path = args.matter_repo

    # Ensure the right modules are imported
    import_modules(matter_repo_path)

    # Step 1 of the instructions.
    copy_xml_to_repo(matter_repo_path)

    # Steps 2 and 3 of the instructions.
    update_zcl_json(matter_repo_path)

    # Step 4 of the instructions.
    generate_app_common(matter_repo_path)

    # Step 5 of the instructions.
    update_cluster_list(matter_repo_path)

    # Step 6 of the instructions.
    copy_device_info_cpp(matter_repo_path)

    # Step 7 of the instructions.
    update_zap_file_with_xml_cluster(matter_repo_path, zap_file_path)

    # Step 8 of the instructions.
    generate_lock_app_zap_file(matter_repo_path, zap_file_path)
