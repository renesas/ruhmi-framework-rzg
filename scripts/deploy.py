import argparse
import mera
import json

def get_build_cfg_dict(build_cfg):
    # replace quote for json compatibility
    # then turn to dict
    build_cfg = json.loads(str(build_cfg).replace("'", '"'))

    return build_cfg

def main(arg):
    with mera.Deployer(arg.out_dir, overwrite=True) as deployer:
        model = mera.ModelLoader(deployer).from_tflite(arg.model_path)
        deployer.deploy(
            model,
            mera_platform=mera.Platform.ALT3,
            build_config=get_build_cfg_dict(arg.build_cfg),
            target=mera.Target.IP,
            host_arch='arm',
        )

def get_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--model_path",
        default="./source_model_files/mobilenet_v2_1.0_224_INT8.tflite",
        type=str,
    )
    parser.add_argument(
        "--out_dir",
        default="./deploy_mobilenetv2",
        type=str,
    )
    parser.add_argument(
        "--build_cfg",
        default="{'scheduler_config': {'mode': 'Fast'}}",
        type=str,
    )

    return parser.parse_args()


if __name__ == "__main__":
    main(get_args())
