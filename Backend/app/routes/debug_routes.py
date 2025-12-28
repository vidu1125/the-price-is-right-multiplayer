# app/routes/debug_routes.py
from flask import Blueprint, jsonify
from app.models import db

bp = Blueprint('debug', __name__, url_prefix='/debug')

@bp.route('/schema/<table_name>', methods=['GET'])
def get_schema(table_name):
    """Get table schema from information_schema"""
    query = """
        SELECT 
            column_name,
            data_type,
            character_maximum_length,
            is_nullable,
            column_default
        FROM information_schema.columns
        WHERE table_name = :table_name
        ORDER BY ordinal_position;
    """
    
    try:
        result = db.session.execute(db.text(query), {'table_name': table_name})
        rows = result.fetchall()
        
        columns = []
        for row in rows:
            col_type = row[1]
            if row[2]:  # character_maximum_length
                col_type = f"{col_type}({row[2]})"
            
            columns.append({
                'name': row[0],
                'type': col_type,
                'nullable': row[3],
                'default': row[4]
            })
        
        return jsonify({
            'success': True,
            'table': table_name,
            'columns': columns
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500
