from flask import Flask, jsonify, request
from routes.example import example_route
import os
from circle_detection.detect_circle import detect_circles

app = Flask(__name__)

# Register other routes
app.register_blueprint(example_route)
app.config["UPLOAD_FOLDER"] = os.path.abspath("./circle_detection/images")

# Ensure the upload folder exists
if not os.path.exists(app.config["UPLOAD_FOLDER"]):
    os.makedirs(app.config["UPLOAD_FOLDER"])


@app.get("/")
def index():
    return jsonify(message="Hello HiveHive!"), 200


@app.post("/upload")
def upload_image():

    if "image" not in request.files:
        return jsonify({"error": "No image file provided"}), 400

    image = request.files["image"]
    if image.filename == "":
        return jsonify({"error": "No selected file"}), 400

    filename = image.filename
    file_path = os.path.join(app.config["UPLOAD_FOLDER"], filename)
    image.save(file_path)

    detect_circles(file_path)

    return jsonify({"message": f"Image {filename} uploaded successfully"}), 200


# Start server
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=4444, debug=True)
