# app/models/account.py
from app.models import db
from datetime import datetime


# ============================
# ACCOUNT
# ============================
class Account(db.Model):
    __tablename__ = "accounts"

    id = db.Column(db.Integer, primary_key=True)
    email = db.Column(db.String(100), unique=True, nullable=False)
    password = db.Column(db.String(255))
    provider = db.Column(db.String(20))
    role = db.Column(db.String(20), default="user", nullable=False)
    status = db.Column(db.String(20))
    created_at = db.Column(db.DateTime)
    updated_at = db.Column(db.DateTime)

    # Relationships
    profile = db.relationship("Profile", back_populates="account", uselist=False)
    session = db.relationship("AccountSession", back_populates="account", uselist=False)

    def to_dict(self):
        return {
            "id": self.id,
            "email": self.email,
            "provider": self.provider,
            "role": self.role,
            "status": self.status,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
        }

class Profile(db.Model):
    __tablename__ = "profiles"

    id = db.Column(db.Integer, primary_key=True)
    account_id = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="CASCADE"))
    name = db.Column(db.String(50))
    avatar = db.Column(db.Text)
    bio = db.Column(db.Text)
    matches = db.Column(db.Integer)
    wins = db.Column(db.Integer)
    points = db.Column(db.Integer)
    badges = db.Column(db.JSON)
    created_at = db.Column(db.DateTime)
    updated_at = db.Column(db.DateTime)

    account = db.relationship("Account", back_populates="profile")

    def to_dict(self):
        return {
            "id": self.id,
            "account_id": self.account_id,
            "name": self.name,
            "avatar": self.avatar,
            "bio": self.bio,
            "matches": self.matches,
            "wins": self.wins,
            "points": self.points,
            "badges": self.badges,
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
        }

