import os
import sqlite3
from datetime import datetime
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
from werkzeug.utils import secure_filename
from werkzeug.security import generate_password_hash, check_password_hash

app = Flask(__name__)
CORS(app)

# Path setup for PythonAnywhere
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'uploads')
DB_FILE = os.path.join(BASE_DIR, 'database.db')

ALLOWED_MOD_EXT = {'zip', 'lua', 'pks', 'rar', '7z'}
ALLOWED_IMG_EXT = {'png', 'jpg', 'jpeg', 'gif', 'webp'}

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# --- DATABASE SETUP ---
def get_db():
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db()
    cursor = conn.cursor()
    
    # Users table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT,
            google_id TEXT,
            role TEXT DEFAULT 'Member'
        )
    ''')
    
    # Mods table (Includes screenshot column)
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS mods (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT NOT NULL,
            author TEXT NOT NULL,
            author_role TEXT NOT NULL,
            filename TEXT NOT NULL,
            screenshot TEXT,
            date TEXT NOT NULL
        )
    ''')
    
    # Forum Threads table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS threads (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            author_role TEXT NOT NULL,
            created_at TEXT NOT NULL
        )
    ''')

    # Forum Posts/Replies table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS posts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            thread_id INTEGER NOT NULL,
            author TEXT NOT NULL,
            author_role TEXT NOT NULL,
            content TEXT NOT NULL,
            created_at TEXT NOT NULL,
            FOREIGN KEY(thread_id) REFERENCES threads(id)
        )
    ''')

    # Create default Owner account if missing
    cursor.execute("SELECT * FROM users WHERE username = 'SCOPIC64'")
    if not cursor.fetchone():
        default_pass = generate_password_hash("admin123")
        cursor.execute("INSERT INTO users (username, email, password_hash, role) VALUES (?, ?, ?, ?)",
                       ('SCOPIC64', 'scopic64@sm64ds.com', default_pass, 'Owner'))

    conn.commit()
    conn.close()

init_db()

# --- HELPERS ---
def allowed_file(filename, allowed_set):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in allowed_set

# --- AUTH ROUTES ---

@app.route('/api/auth/register', methods=['POST'])
def register():
    data = request.json or {}
    username = data.get('username')
    email = data.get('email')
    password = data.get('password')

    if not username or not email or not password:
        return jsonify({'error': 'All fields are required.'}), 400

    hashed_pw = generate_password_hash(password)
    conn = get_db()
    cursor = conn.cursor()
    try:
        cursor.execute("INSERT INTO users (username, email, password_hash, role) VALUES (?, ?, ?, 'Member')",
                       (username, email, hashed_pw))
        conn.commit()
        user = {'username': username, 'email': email, 'role': 'Member'}
        return jsonify({'message': 'User registered successfully!', 'user': user}), 201
    except sqlite3.IntegrityError:
        return jsonify({'error': 'Username or Email already exists.'}), 400
    finally:
        conn.close()

@app.route('/api/auth/login', methods=['POST'])
def login():
    data = request.json or {}
    email = data.get('email')
    password = data.get('password')

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM users WHERE email = ?", (email,))
    user = cursor.fetchone()
    conn.close()

    if user and user['password_hash'] and check_password_hash(user['password_hash'], password):
        return jsonify({
            'message': 'Login successful',
            'user': {
                'username': user['username'],
                'email': user['email'],
                'role': user['role']
            }
        })
    
    return jsonify({'error': 'Invalid credentials.'}), 401

@app.route('/api/auth/google', methods=['POST'])
def google_auth():
    data = request.json or {}
    email = data.get('email')
    name = data.get('name')
    google_id = data.get('google_id')

    if not email or not google_id:
        return jsonify({'error': 'Invalid Google account data.'}), 400

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM users WHERE email = ? OR google_id = ?", (email, google_id))
    user = cursor.fetchone()

    if not user:
        # Auto-create account for new Google user
        username = name.replace(" ", "_") if name else f"User_{google_id[:6]}"
        cursor.execute("INSERT INTO users (username, email, google_id, role) VALUES (?, ?, ?, 'Member')",
                       (username, email, google_id))
        conn.commit()
        user_data = {'username': username, 'email': email, 'role': 'Member'}
    else:
        user_data = {'username': user['username'], 'email': user['email'], 'role': user['role']}

    conn.close()
    return jsonify({'message': 'Google authentication successful', 'user': user_data})


# --- MODS & SCREENSHOT ROUTES ---

@app.route('/api/mods', methods=['GET'])
def get_mods():
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM mods ORDER BY id DESC")
    mods = [dict(row) for row in cursor.fetchall()]
    conn.close()
    return jsonify(mods)

@app.route('/api/upload', methods=['POST'])
def upload_mod():
    author = request.form.get('author')
    role = request.form.get('role')
    title = request.form.get('title')
    description = request.form.get('description')

    if role not in ['Owner', 'Modder']:
        return jsonify({'error': 'Unauthorized: Only Owner and Modder roles can upload.'}), 403

    if 'file' not in request.files:
        return jsonify({'error': 'Mod file is required.'}), 400

    mod_file = request.files['file']
    if not mod_file or not allowed_file(mod_file.filename, ALLOWED_MOD_EXT):
        return jsonify({'error': 'Invalid mod format (.zip, .lua, .pks, .rar, .7z allowed).'}), 400

    mod_filename = secure_filename(f"{int(datetime.now().timestamp())}_{mod_file.filename}")
    mod_file.save(os.path.join(app.config['UPLOAD_FOLDER'], mod_filename))

    # Optional Screenshot upload
    screenshot_filename = None
    if 'screenshot' in request.files:
        img_file = request.files['screenshot']
        if img_file and allowed_file(img_file.filename, ALLOWED_IMG_EXT):
            screenshot_filename = secure_filename(f"ss_{int(datetime.now().timestamp())}_{img_file.filename}")
            img_file.save(os.path.join(app.config['UPLOAD_FOLDER'], screenshot_filename))

    date_str = datetime.now().strftime("%b %d, %Y")

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute('''
        INSERT INTO mods (title, description, author, author_role, filename, screenshot, date)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    ''', (title, description, author, role, mod_filename, screenshot_filename, date_str))
    conn.commit()
    conn.close()

    return jsonify({"message": "Mod uploaded successfully!"}), 201

@app.route('/api/download/<filename>', methods=['GET'])
def download_file(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename, as_attachment=True)


# --- FORUM ROUTES ---

@app.route('/api/forum/threads', methods=['GET'])
def get_threads():
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute('''
        SELECT threads.*, COUNT(posts.id) as reply_count 
        FROM threads LEFT JOIN posts ON threads.id = posts.thread_id
        GROUP BY threads.id ORDER BY threads.id DESC
    ''')
    threads = [dict(row) for row in cursor.fetchall()]
    conn.close()
    return jsonify(threads)

@app.route('/api/forum/threads', methods=['POST'])
def create_thread():
    data = request.json or {}
    title = data.get('title')
    author = data.get('author')
    author_role = data.get('author_role', 'Member')
    content = data.get('content')

    if not title or not content or not author:
        return jsonify({'error': 'Title and content are required.'}), 400

    now_str = datetime.now().strftime("%b %d, %Y %H:%M")
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("INSERT INTO threads (title, author, author_role, created_at) VALUES (?, ?, ?, ?)",
                   (title, author, author_role, now_str))
    thread_id = cursor.lastrowid

    # Create initial post inside thread
    cursor.execute("INSERT INTO posts (thread_id, author, author_role, content, created_at) VALUES (?, ?, ?, ?, ?)",
                   (thread_id, author, author_role, content, now_str))
    conn.commit()
    conn.close()

    return jsonify({'message': 'Thread created successfully!', 'thread_id': thread_id}), 201

@app.route('/api/forum/threads/<int:thread_id>', methods=['GET'])
def get_thread_detail(thread_id):
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM threads WHERE id = ?", (thread_id,))
    thread = cursor.fetchone()
    if not thread:
        conn.close()
        return jsonify({'error': 'Thread not found'}), 404

    cursor.execute("SELECT * FROM posts WHERE thread_id = ? ORDER BY id ASC", (thread_id,))
    posts = [dict(row) for row in cursor.fetchall()]
    conn.close()

    return jsonify({'thread': dict(thread), 'posts': posts})

@app.route('/api/forum/posts', methods=['POST'])
def create_post():
    data = request.json or {}
    thread_id = data.get('thread_id')
    author = data.get('author')
    author_role = data.get('author_role', 'Member')
    content = data.get('content')

    if not thread_id or not content or not author:
        return jsonify({'error': 'Content is required.'}), 400

    now_str = datetime.now().strftime("%b %d, %Y %H:%M")
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("INSERT INTO posts (thread_id, author, author_role, content, created_at) VALUES (?, ?, ?, ?, ?)",
                   (thread_id, author, author_role, content, now_str))
    conn.commit()
    conn.close()

    return jsonify({'message': 'Reply posted!'}), 201


if __name__ == '__main__':
    app.run(port=5000, debug=True)
