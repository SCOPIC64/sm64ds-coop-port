import os
import json
from datetime import datetime
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
from werkzeug.utils import secure_filename

app = Flask(__name__)
CORS(app)

UPLOAD_FOLDER = 'uploads'
DATA_FILE = 'mods_db.json'
ALLOWED_EXTENSIONS = {'zip', 'lua', 'pks', 'rar', '7z'}

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

if not os.path.exists(DATA_FILE):
    with open(DATA_FILE, 'w') as f:
        json.dump([], f)

def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

def load_mods():
    with open(DATA_FILE, 'r') as f:
        return json.load(f)

def save_mods(data):
    with open(DATA_FILE, 'w') as f:
        json.dump(data, f, indent=4)

@app.route('/api/mods', methods=['GET'])
def get_mods():
    """Returns list of uploaded mods."""
    return jsonify(load_mods())

@app.route('/api/upload', methods=['POST'])
def upload_mod():
    """Restriction: Only Owner and Modder roles can upload."""
    author = request.form.get('author')
    role = request.form.get('role')
    title = request.form.get('title')
    description = request.form.get('description')

    if role not in ['Owner', 'Modder']:
        return jsonify({'error': 'Unauthorized: Only Owner and Modder roles can upload.'}), 403

    if 'file' not in request.files:
        return jsonify({'error': 'No file uploaded.'}), 400

    file = request.files['file']
    if file and allowed_file(file.filename):
        filename = secure_filename(file.filename)
        timestamped_name = f"{int(datetime.now().timestamp())}_{filename}"
        file.save(os.path.join(app.config['UPLOAD_FOLDER'], timestamped_name))

        mod_entry = {
            "id": len(load_mods()) + 1,
            "title": title,
            "description": description,
            "author": author,
            "author_role": role,
            "filename": timestamped_name,
            "date": datetime.now().strftime("%b %d, %Y")
        }

        mods = load_mods()
        mods.insert(0, mod_entry)
        save_mods(mods)

        return jsonify({"message": "Mod uploaded!", "mod": mod_entry}), 201

    return jsonify({'error': 'Allowed file formats: .zip, .lua, .pks, .rar, .7z'}), 400

@app.route('/api/download/<filename>', methods=['GET'])
def download_mod(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename, as_attachment=True)

if __name__ == '__main__':
    print("SM64DS Coop Hub Backend running on http://127.0.0.1:5000")
    app.run(port=5000, debug=True)