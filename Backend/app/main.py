from flask import Flask, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

@app.route('/health')
def health():
    return jsonify({'status': 'healthy'}), 200

@app.route('/api/game')
def game():
    return jsonify({'rounds': 5, 'mode': 'elimination'}), 200

if __name__ == '__main__':
    print("Backend API starting on port 5000...")
    app.run(host='0.0.0.0', port=5000, debug=True)
