from circle_detection.detect_circle import detect_circles
import os
import cv2

INPUT_PATH = "/Users/eliaspfeiffer/Developer/hivehive-esp32/circle_evaluation/input"
OUPUT_PATH = "/Users/eliaspfeiffer/Developer/hivehive-esp32/circle_evaluation/output"


# Helper function to get image paths from a directory
def get_imagae_paths(path):
    files = os.listdir(path)
    paths = []
    for f in files:
        full_path = os.path.join(path, f)
        paths.append(full_path)
    return paths


image_paths = get_imagae_paths(INPUT_PATH)

# Process each image and print results
for path in image_paths:
    circles, result_img = detect_circles(path)

    filename = os.path.basename(path)
    save_path = os.path.join(OUPUT_PATH, filename)
    # Save annotated image
    cv2.imwrite(save_path, result_img)

    print(f"Image: {path}")
    print(f"Circles: {circles}")
    print("-------------------------")
