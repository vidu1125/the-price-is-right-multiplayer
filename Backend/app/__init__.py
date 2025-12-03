from flask import Flask
from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()

def create_app():
    app = Flask(__name__)

    # Config database
    app.config["SQLALCHEMY_DATABASE_URI"] = "postgresql://user:pass@host/dbname"
    app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False

    db.init_app(app)

    # Import blueprints
    from app.routes import room_bp, utils_bp
    app.register_blueprint(room_bp)
    app.register_blueprint(utils_bp)

    return app
