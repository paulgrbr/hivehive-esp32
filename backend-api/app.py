import os

from flask import Flask, jsonify, request

from circle_detection.detect_circle import detect_circles
from routes.preview import preview_route, push_frame

app = Flask(__name__)

# Global variable to store detected circles
circles_array = []

# Register other routes
app.register_blueprint(preview_route)

# Ensure the upload folder exists
app.config["UPLOAD_FOLDER"] = os.path.abspath("backend-api/circle_detection/images")
os.makedirs(app.config["UPLOAD_FOLDER"], exist_ok=True)


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
    return (
        jsonify(
            {
                "message": f"Image {image.filename} uploaded successfully",
                "circles": circles,
            }
        ),
        200,
    )


# route to get array of detected circles
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
