# app/models/room.py
from app.models import db
from datetime import datetime


class Room(db.Model):
    __tablename__ = "rooms"

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100))
    visibility = db.Column(db.String(20))
    host_id = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="SET NULL"))
    status = db.Column(db.String(20))
    created_at = db.Column(db.DateTime)
    updated_at = db.Column(db.DateTime)

    members = db.relationship("RoomMember", back_populates="room",
                              cascade="all, delete-orphan", lazy=True)

    def to_dict(self):
        host_profile = self.host.profile if hasattr(self, "host") and self.host and self.host.profile else None

        return {
            "id": self.id,
            "name": self.name,
            "visibility": self.visibility,
            "host_id": self.host_id,
            "host_name": host_profile.name if host_profile else "Unknown",
            "status": self.status,
            "current_players": len(self.members),
            "created_at": self.created_at.isoformat() if self.created_at else None,
            "updated_at": self.updated_at.isoformat() if self.updated_at else None,
        }


class RoomMember(db.Model):
    __tablename__ = "room_members"

    room_id = db.Column(db.Integer, db.ForeignKey("rooms.id", ondelete="CASCADE"), primary_key=True)
    account_id = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="CASCADE"), primary_key=True)
    
    # Removed:
    # role
    # kicked
    # left_at

    ready = db.Column(db.Boolean, default=False)
    joined_at = db.Column(db.DateTime)

    room = db.relationship("Room", back_populates="members")

    def to_dict(self):
        profile = self.account.profile if hasattr(self, "account") and self.account and self.account.profile else None

        return {
            "room_id": self.room_id,
            "account_id": self.account_id,
            "name": profile.name if profile else "Unknown",
            "avatar": profile.avatar if profile else None,
            "ready": self.ready,
            "joined_at": self.joined_at.isoformat() if self.joined_at else None,
        }
