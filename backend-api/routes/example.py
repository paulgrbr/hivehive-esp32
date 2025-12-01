from flask import Blueprint, jsonify

# Create a blueprint named 'example'
example_route = Blueprint("example", __name__, url_prefix="/example")


# Define routes under this blueprint
@example_route.get("/")
def hello_example():
    return jsonify(message="Hello from the example route!"), 200
