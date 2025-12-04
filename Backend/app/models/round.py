# app/models/rounds.py
from app.models import db

from datetime import datetime

class Round(db.Model):
    __tablename__ = "rounds"

    id = db.Column(db.Integer, primary_key=True)
    match_id = db.Column(db.Integer, db.ForeignKey("matches.id", ondelete="CASCADE"))
    number = db.Column(db.Integer)
    type = db.Column(db.String(20))
    created_at = db.Column(db.DateTime)

    def to_dict(self):
        return {
            "id": self.id,
            "match_id": self.match_id,
            "number": self.number,
            "type": self.type,
            "created_at": self.created_at.isoformat() if self.created_at else None,
        }
