from flask import Flask, jsonify
from routes.example import example_route

app = Flask(__name__)

# Register other routes
app.register_blueprint(example_route)


@app.get("/")
def index():
    return jsonify(message="Hello HiveHive!"), 200


# Start server
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=4444, debug=True)
