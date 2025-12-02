from app.models import db
from datetime import datetime
import uuid

class Round(db.Model):
    __tablename__ = 'rounds'

    id = db.Column(db.Uuid, primary_key=True, default=uuid.uuid4)

    match_id = db.Column(db.Integer, db.ForeignKey('matches.id', ondelete="CASCADE"), nullable=False)

    number = db.Column(db.Integer, nullable=False)

    type = db.Column(db.String(20), nullable=False)

    question = db.Column(db.Uuid)

    created_at = db.Column(db.DateTime, default=datetime.utcnow)

    def to_dict(self):
        return {
            "id": str(self.id),
            "match_id": self.match_id,
            "number": self.number,
            "type": self.type,
            "question": str(self.question) if self.question else None,
            "created_at": self.created_at.isoformat() if self.created_at else None,
        }
