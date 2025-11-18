import os
from concurrent.futures import ThreadPoolExecutor

from flask import Flask, jsonify, request

from routes.preview import preview_route, push_frame
from services.aws import AWSClient
from services.circle_detection.detect_circle import detect_circles

app = Flask(__name__)

# Global variable to store detected circles
circles_array = []

# Register other routes
app.register_blueprint(preview_route)

# Ensure the upload folder exists
app.config["UPLOAD_FOLDER"] = os.path.abspath(
    "backend-api/services/circle_detection/images"
)
os.makedirs(app.config["UPLOAD_FOLDER"], exist_ok=True)

# Init AWS client for image upload
s3 = AWSClient()
executor = ThreadPoolExecutor(max_workers=25)  # limit


# Upload route for ESP
@app.post("/upload")
def upload_image():
    if "image" not in request.files:
        return jsonify({"error": "No image file provided"}), 400

    image = request.files["image"]
    if image.filename == "":
        return jsonify({"error": "No selected file"}), 400

    file_path = os.path.join(app.config["UPLOAD_FOLDER"], image.filename)
    image.save(file_path)

    circles, result_img = detect_circles(file_path)
    circles_array.clear()
    circles_array.append(circles)
    push_frame(result_img)

    # Push image to S3 bucket asynchronously
    executor.submit(s3.upload, "validation", file_path, delete=True)

    return (
        jsonify(
            {
                "message": f"Image {image.filename} uploaded successfully",
                "circles": circles,
            }
        ),
        200,
    )


# Results route for classification result
@app.get("/result")
def get_result():
    return (
        jsonify(
            {
                "circles": circles_array,
            }
        ),
        200,
    )


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=4444, debug=True)
