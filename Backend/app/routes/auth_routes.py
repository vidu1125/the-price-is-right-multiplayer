from flask import Blueprint, request, jsonify
from app.services.auth_service import AuthService

auth_bp = Blueprint('auth', __name__, url_prefix='/api/auth')

@auth_bp.route('/register', methods=['POST'])
def register():
    """
    POST /api/auth/register
    Body: {
        "email": "user@example.com",
        "password": "password123",
        "name": "Username"
    }
    """
    data = request.get_json()
    
    if not data:
        return jsonify({'error': 'Request body is required'}), 400
    
    email = data.get('email')
    password = data.get('password')
    name = data.get('name')
    
    if not email or not password or not name:
        return jsonify({'error': 'Email, password, and name are required'}), 400
    
    result = AuthService.register_user(email, password, name)
    
    if result.get('success'):
        return jsonify(result), result.get('code', 201)
    
    return jsonify(result), result.get('code', 400)


@auth_bp.route('/login', methods=['POST'])
def login():
    """
    POST /api/auth/login
    Body: {
        "email": "user@example.com",
        "password": "password123"
    }
    """
    data = request.get_json()
    
    if not data:
        return jsonify({'error': 'Request body is required'}), 400
    
    email = data.get('email')
    password = data.get('password')
    
    if not email or not password:
        return jsonify({'error': 'Email and password are required'}), 400
    
    result = AuthService.login_user(email, password)
    
    if result.get('success'):
        return jsonify(result), 200
    
    return jsonify(result), result.get('code', 400)


@auth_bp.route('/logout', methods=['POST'])
def logout():
    """
    POST /api/auth/logout
    """
    return jsonify({'success': True, 'message': 'Logged out successfully'}), 200
