import tensorflow as tf
import numpy as np
import argparse

def main(arg):
    interpreter = tf.lite.Interpreter(model_path=arg.model_path)
    interpreter.allocate_tensors()
    in_info = interpreter.get_input_details()[0]
    out_info = interpreter.get_output_details()[0]
    input_data = np.random.randint(low=-128, high=128, size=(1, 224, 224, 3), dtype=np.int8)
    with open(arg.out_dir + '/input_0.bin', 'wb') as f:
      f.write(input_data.tobytes())
    interpreter.set_tensor(in_info['index'], input_data)
    interpreter.invoke()
    output_data = interpreter.get_tensor(out_info['index'])
    with open(arg.out_dir + '/ref_result_0.bin', 'wb') as f:
      f.write(output_data.tobytes())

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
    return parser.parse_args()

if __name__ == "__main__":
    main(get_args())
