#! /usr/bin/env python3
'''
Copyright (C) 2026 Renesas Electronics Corp.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

'''

import argparse
import json
import logging
import mera
import numpy as np
import os
import shutil
import sys
import yaml

from ai_edge_litert.interpreter import Interpreter
from pathlib import Path

logger = logging.getLogger()

def get_args() -> argparse.Namespace:
    """
    Get command-line arguments and verify.

    Returns:
        argparse.Namspace: Command-line arguments set.
    """
    parser = argparse.ArgumentParser(prog="Renesas Ethos-U Model Data Generator", description="Generates the model data used by the EdgeCortix MERA library, including sample input/output files")
    parser.add_argument("-c", "--build_config", type=str,
                        help="MERA build configuration", default="{'scheduler_config': {'mode': 'Fast'}}")
    parser.add_argument("-d", "--output_dir", type=str, required=True,
                        help="Output directory where files will be stored")
    parser.add_argument("-m", "--model_paths", nargs='+', type=str, required=True,
                        help="Space separated list of TFLite models to generate the files for")
    args = parser.parse_args()

    for model_path in args.model_paths:
        if not os.path.exists(model_path):
            parser.error(f"{model_path} does not exist!")

    return args

def get_build_config(build_config: str) -> dict:
    """
    Gets MERA build configuration settings from command-line argument.

    Args:
        build_config: Quoted JSON containing MERA build configuration.

    Returns:
        dict: Dictionary of MERA build configuration.
    """
    build_config = json.loads(str(build_config).replace("'", '"'))

    return build_config

def is_vela_model_size_supported(model_deploy_dir: str) -> bool:
    """
    Checks that Vela model size does not exceed 32MB model size limit.

    Args:
        model_deploy_dir: RUHMI compilation deployment directory.

    Returns:
        bool: True if model size is supported, False otherwise.
    """
    model_path = next(Path(model_deploy_dir).rglob("model_vela.tflite"), None)
    
    if model_path is None:
        logger.error(f"Unable to find model_vela.tflite inside of {model_name}")
        sys.exit(1)

    model_size = model_path.stat().st_size

    # Check if model size exceeds 32MB (33554432 bytes)
    if model_size > 33554432:
        return False
    elif model_size == 0:
        logger.error("Vela compiled Model file size is 0!")
        sys.exit(1)
    else:
        return True

def generate_model_test_data(model_name: str, model_deploy_dir: str) -> dict:
    """
    Generates model test data (inputs/outputs) and the entry for the
    configuration YAML.

    Args:
        model_name: Name of the input model (without TFLite extension).
        model_deploy_dir: RUHMI complication deployment directory.

    Returns:
        dict: Dictionary containing the model entry for configuration YAML.
    """
    model_path = next(Path(model_deploy_dir).rglob("model.tflite"), None)

    if model_path is None:
        logger.error(f"Unable to find model.tflite inside of {model_name}")
        sys.exit(1)

    # Load MERA tflite model
    interpreter = Interpreter(model_path=model_path)
    interpreter.allocate_tensors()

    # Get input information
    input_details = interpreter.get_input_details()

    inputs = []

    for index, input_detail in enumerate(input_details):
        input_file_name = f"input-{index}.bin"

        name = input_detail['name']
        shape = input_detail['shape'].tolist()
        dtype = input_detail['dtype']

        # Generate randomized input
        if dtype == np.int8:
            data_type = "int8"
            input_data = np.random.randint(low=-128, high=128, size=shape, dtype=dtype)
        elif dtype == np.uint8:
            data_type = "uint8"
            input_data = np.random.randint(low=0, high=256, size=shape, dtype=dtype)
        elif dtype == np.float32:
            data_type = "float32"
            input_data = np.random.random(size=shape).astype(dtype)
        else:
            logger.error(f"Unsupported input data type ({dtype})")
            sys.exit(1)

        input_file_path = os.path.join(model_deploy_dir, input_file_name)

        # Save generated input to model output directory
        with open(input_file_path, 'wb') as f:
            f.write(input_data.tobytes())

        logger.info(f"Input data for tensor #{index} has been saved to: {input_file_path}")

        # Define YAML entry for input
        input_yaml_entry = {
            'name': name,
            'data_type': data_type,
            'file_name': input_file_name,
            'shape': shape
        }

        # Store YAML entry into a list containing entries for each input
        inputs.append(input_yaml_entry)

        interpreter.set_tensor(index, input_data)

    # Run inference and save expected output to model output directory
    interpreter.invoke()

    output_info = interpreter.get_output_details()

    for index, output_tensor in enumerate(output_info):
        output_file = os.path.join(model_deploy_dir, f"expected-output-{index}.bin")

        output_data = interpreter.get_tensor(output_tensor['index'])

        # Save expected input to model output directory
        with open(output_file, 'wb') as f:
            f.write(output_data.tobytes())

        logger.info(f"Expected output data for tensor #{index} has been saved to: {output_file}")

    return {
        'name': model_name,
        'data_directory': f"{model_name}/",
        'inputs': inputs
    }

if __name__ == '__main__':
    format = "%(asctime)s %(levelname)s -  %(message)s"
    logging.basicConfig(format=format, level=logging.INFO)

    args = get_args()

    output_dir = args.output_dir.rstrip('/')

    # Save backup if directory already exists
    if os.path.exists(output_dir):
        output_dir_backup = f"{output_dir}_backup"

        logger.warning(f"{output_dir} already exists! The directory has now been moved to {output_dir_backup}")

        # Remove backup directory if it already exists
        if os.path.exists(output_dir_backup):
            logger.warning(f"Removing previous backup directory ({output_dir})!")

            shutil.rmtree(output_dir_backup)

        os.rename(output_dir, f"{output_dir}_backup")

    os.mkdir(output_dir)

    build_config = get_build_config(args.build_config)
    model_paths = args.model_paths

    model_config_list = []

    # Generate data for each model defined
    for model_path in model_paths:
        model_name = os.path.basename(model_path)
        model_name = model_name.replace(".tflite", "")

        model_deploy_dir = os.path.join(output_dir, model_name)

        logger.info("------------------------------------------------------------")
        logger.info(f"Model being compiled with RUHMI: {model_name} ({model_path})")

        # Compile model with RUHMI
        with mera.Deployer(model_deploy_dir, overwrite=True) as deployer:
            model = mera.ModelLoader(deployer).from_tflite(model_path)
            deployer.deploy(
                model,
                mera_platform=mera.Platform.ALT3,
                build_config=build_config,
                target=mera.Target.IP,
                host_arch='arm',
            )

        # Check if model size exceeds 32MB limit before generating test data
        if not is_vela_model_size_supported(model_deploy_dir):
            logger.error(f"Model ({model_name}) exceeds 32MB supported model size limit! This model cannot run on RZ/G3E!")
            sys.exit(1)

        model_config_list.append(generate_model_test_data(model_name, model_deploy_dir))

        logger.info("------------------------------------------------------------")

    config_yaml = {'models': model_config_list}
    config_yaml_path = os.path.join(output_dir, "config.yaml")

    # Save the configuration YAML to output directory
    with open(config_yaml_path, 'w') as f:
        yaml.dump(config_yaml, f, default_flow_style=None, sort_keys=False)

    logger.info(f"The configuration YAML has been saved to: {config_yaml_path}")
    logger.info(f"Please copy the {output_dir} directory to the board!")

    sys.exit()
