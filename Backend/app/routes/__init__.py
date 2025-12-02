from flask import jsonify

def register_routes(app):

    @app.route("/health")
    def health():
        return jsonify({"status": "healthy"}), 200

    @app.route("/health/db")
    def db_check():
        from app import db
        try:
            db.session.execute(db.text("SELECT 1"))
            return jsonify({"db": "connected"}), 200
        except Exception as e:
            return jsonify({"db": "error", "details": str(e)}), 500

    @app.route("/api/game")
    def game():
        return jsonify({"rounds": 5, "mode": "elimination"})
