from app.models import db
from .base import Base
from sqlalchemy.sql import func

class Account(Base):
    __tablename__ = "accounts"

    id = db.Column(db.Integer, primary_key=True)
    email = db.Column(db.String(100), unique=True, nullable=False)
    password = db.Column(db.String(255))
    role = db.Column(db.String(20), nullable=False, default="user")
    # status column does NOT exist in Supabase - removed
    created_at = db.Column(db.DateTime, server_default=func.now())
    updated_at = db.Column(db.DateTime, server_default=func.now(), onupdate=func.now())
