from http.server import BaseHTTPRequestHandler, HTTPServer
import os, io, time, urllib.parse, mimetypes
from email.parser import BytesParser
from email.policy import default as default_policy

UPLOAD_DIR = "uploads"
LAST_IMAGE_NAME = None
os.makedirs(UPLOAD_DIR, exist_ok=True)


def save_bytes_as_file(data: bytes, mime: str, hinted_name: str = None) -> str:
    base = hinted_name or f"upload-{int(time.time())}"
    base = os.path.basename(base)
    ext = mimetypes.guess_extension(mime) or ""
    name = base if base.lower().endswith(ext) else base + ext
    path = os.path.join(UPLOAD_DIR, name)
    i = 1
    root, ext2 = os.path.splitext(path)
    while os.path.exists(path):
        path = f"{root}-{i}{ext2}"
        i += 1
    with open(path, "wb") as f:
        f.write(data)
    return os.path.basename(path)


class SimpleHandler(BaseHTTPRequestHandler):
    def _send_html(self, html: str, status: int = 200):
        self.send_response(status)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(html.encode("utf-8"))

    def do_GET(self):
        global LAST_IMAGE_NAME
        if self.path.startswith("/uploads/"):
            name = os.path.basename(urllib.parse.unquote(self.path[len("/uploads/"):]))
            fpath = os.path.join(UPLOAD_DIR, name)
            if not os.path.isfile(fpath):
                self.send_error(404, "File not found")
                return
            mime, _ = mimetypes.guess_type(fpath)
            mime = mime or "application/octet-stream"
            self.send_response(200)
            self.send_header("Content-Type", mime)
            self.end_headers()
            with open(fpath, "rb") as f:
                self.wfile.write(f.read())
            return

        img_html = ""
        if LAST_IMAGE_NAME:
            img_url = f"/uploads/{urllib.parse.quote(LAST_IMAGE_NAME)}"
            img_html = f"""
            <h2>Last uploaded image</h2>
            <img src="{img_url}" alt="uploaded" style="max-width:100%;border:1px solid #ccc;" />
            <p><a href="{img_url}" download>Download</a></p>
            """

        page = f"""<!doctype html>
<html>
  <head><meta charset="utf-8"><title>Image Upload</title></head>
  <body>
    <h1>Simple Image Upload</h1>
    <form method="POST" action="/upload" enctype="multipart/form-data">
      <input type="file" name="file" accept="image/*" required>
      <button type="submit">Upload</button>
    </form>
    <p>Or via curl:</p>
    <pre><code>curl -F "file=@image.jpg" http://127.0.0.1:8080/upload</code></pre>
    {img_html or "<p>No image uploaded yet.</p>"}
  </body>
</html>"""
        self._send_html(page)

    def do_POST(self):
        global LAST_IMAGE_NAME
        if self.path != "/upload":
            self.send_error(404, "Not Found")
            return

        ctype = self.headers.get("Content-Type", "")
        clen = int(self.headers.get("Content-Length", "0") or 0)
        body = self.rfile.read(clen)

        try:
            if "multipart/form-data" in ctype:
                boundary = ctype.split("boundary=")[-1].encode()
                message = BytesParser(policy=default_policy).parsebytes(
                    b"Content-Type: " + ctype.encode() + b"\r\n\r\n" + body
                )
                part = None
                for p in message.iter_parts():
                    if p.get_filename():
                        part = p
                        break
                if not part:
                    self.send_error(400, "No file part found")
                    return
                data = part.get_payload(decode=True)
                mime = part.get_content_type()
                hinted_name = part.get_filename()
                saved = save_bytes_as_file(data, mime, hinted_name)

            else:
                if not ctype.startswith("image/"):
                    self.send_error(415, "Expecting image/* Content-Type")
                    return
                saved = save_bytes_as_file(body, ctype, "raw-upload")

            LAST_IMAGE_NAME = saved
            self.send_response(303)
            self.send_header("Location", "/")
            self.end_headers()

        except Exception as e:
            self.send_error(500, f"Upload failed: {e}")


if __name__ == "__main__":
    server = HTTPServer(("127.0.0.1", 8080), SimpleHandler)
    print("Listening on http://127.0.0.1:8080")
    server.serve_forever()