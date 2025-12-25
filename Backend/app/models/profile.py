from app.models import db
from .base import Base
from sqlalchemy.sql import func

class Profile(Base):
    __tablename__ = "profiles"

    id = db.Column(db.Integer, primary_key=True)

    account_id = db.Column(
        db.Integer,
        db.ForeignKey("accounts.id"),
        nullable=False
    )

    name = db.Column(db.String(50))
    avatar = db.Column(db.Text)
    bio = db.Column(db.Text)

    matches = db.Column(db.Integer, default=0)
    wins = db.Column(db.Integer, default=0)
    points = db.Column(db.Integer, default=0)

    badges = db.Column(db.JSON)   # 👈 KHÔNG dùng JSONB

    created_at = db.Column(db.DateTime, server_default=func.now())
    updated_at = db.Column(db.DateTime, onupdate=func.now())
